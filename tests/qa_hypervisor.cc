#define BOOST_TEST_MODULE hydra_hypervisor_tests
#include <boost/test/included/unit_test.hpp>
#include <boost/log/core.hpp>

#include "hydra/hydra_core.h"
#include "hydra/hydra_hypervisor.h"

#include <memory>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

namespace {

struct QuietLogs
{
  bool previously_enabled = boost::log::core::get()->set_logging_enabled(false);
  ~QuietLogs()
  {
    boost::log::core::get()->set_logging_enabled(previously_enabled);
  }
};

class IdleDevice : public hydra::abstract_device
{
public:
  void send(const hydra::iq_window&, size_t) override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  size_t receive(hydra::iq_window&, size_t) override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return 0;
  }
};

}

BOOST_AUTO_TEST_CASE(unknown_and_repeated_detach_are_safe)
{
  hydra::Hypervisor hypervisor;
  BOOST_CHECK(not hypervisor.detach_virtual_radio(42));

  auto radio = std::make_shared<hydra::VirtualRadio>(42, &hypervisor);
  hypervisor.attach_virtual_radio(radio);
  BOOST_REQUIRE(hypervisor.detach_virtual_radio(42));
  BOOST_CHECK(not hypervisor.detach_virtual_radio(42));
}

BOOST_AUTO_TEST_CASE(unconfigured_core_cleanup_is_safe)
{
  hydra::HydraCore core;
  BOOST_CHECK_EQUAL(core.request_rx_resources(1, 100, 10, "127.0.0.1", "127.0.0.1"), 0);
  BOOST_CHECK_EQUAL(core.request_tx_resources(1, 100, 10, "127.0.0.1", "127.0.0.1"), 0);
  core.stop();
  BOOST_CHECK_EQUAL(core.free_resources(1), 1);
}

BOOST_AUTO_TEST_CASE(fft_roundtrips_release_owned_resources)
{
  for (const size_t size : {8, 16, 32})
  {
    hydra::fft_complex forward(size);
    hydra::fft_complex inverse(size, false);
    hydra::iq_window input(size);
    for (size_t i = 0; i < size; ++i)
      input[i] = hydra::iq_sample(float(i) / size, -0.25f);
    forward.set_data(input.data(), size);
    forward.execute();
    inverse.set_data(forward.get_outbuf(), size);
    inverse.execute();
    for (size_t i = 0; i < size; ++i)
      BOOST_CHECK_SMALL(std::abs(inverse.get_outbuf()[i] - input[i]), 1e-5f);
  }
}

BOOST_AUTO_TEST_CASE(framing_workers_stop_on_destruction_and_repeated_stop)
{
  auto samples = std::make_shared<hydra::hydra_buffer<hydra::iq_sample>>(32);
  auto windows = std::make_shared<hydra::hydra_buffer<hydra::iq_window>>(4);
  hydra::resampler<hydra::iq_sample, hydra::iq_window> pack(samples, 8, 8);
  hydra::resampler<hydra::iq_window, hydra::iq_sample> unpack(windows, 8, 8);
  BOOST_CHECK_NO_THROW(pack.stop());
  BOOST_CHECK_NO_THROW(pack.stop());
  BOOST_CHECK_NO_THROW(unpack.stop());
  BOOST_CHECK_NO_THROW(unpack.stop());
  hydra::resampler<hydra::iq_sample, hydra::iq_window> automatic(samples, 8, 8);
}

BOOST_AUTO_TEST_CASE(socket_workers_stop_on_destruction_and_repeated_stop)
{
  boost::asio::io_service io;
  boost::asio::ip::tcp::acceptor peer(
      io, {boost::asio::ip::address_v4::loopback(), 0});
  const auto port = std::to_string(peer.local_endpoint().port());
  auto samples = std::make_shared<hydra::hydra_buffer<hydra::iq_sample>>(32);
  auto source = hydra::zmq_source::make("127.0.0.1", "127.0.0.1", port, 32);
  auto sink = hydra::zmq_sink::make(samples, "127.0.0.1", "127.0.0.1", "*");
  BOOST_CHECK_NO_THROW(source->stop());
  BOOST_CHECK_NO_THROW(source->stop());
  BOOST_CHECK_NO_THROW(sink->stop());
  BOOST_CHECK_NO_THROW(sink->stop());
  auto automatic = hydra::zmq_source::make("127.0.0.1", "127.0.0.1", port, 32);
}

BOOST_AUTO_TEST_CASE(hypervisor_shutdown_stops_attached_radio_workers)
{
  boost::asio::io_service io;
  boost::asio::ip::tcp::acceptor peer(
      io, {boost::asio::ip::address_v4::loopback(), 0});
  auto device = std::make_shared<IdleDevice>();
  hydra::VirtualRadioPtr radio;
  {
    hydra::Hypervisor hypervisor;
    hypervisor.set_rx_resources(device, 1000, 64, 64);
    hypervisor.set_tx_resources(device, 1000, 64, 64);
    radio = std::make_shared<hydra::VirtualRadio>(1, &hypervisor);
    BOOST_REQUIRE_EQUAL(radio->set_tx_chain(
        peer.local_endpoint().port(), 1000, 16, "127.0.0.1", "127.0.0.1"), 0);
    hypervisor.attach_virtual_radio(radio);
    BOOST_CHECK_NO_THROW(hypervisor.stop());
    BOOST_CHECK_NO_THROW(hypervisor.stop());
  }
  BOOST_CHECK_NO_THROW(radio->stop());
}

BOOST_AUTO_TEST_CASE(concurrent_retuning_streaming_membership_and_shutdown)
{
  QuietLogs quiet;
  boost::asio::io_service io;
  boost::asio::ip::tcp::acceptor reservation(
      io, {boost::asio::ip::address_v4::loopback(), 0});
  const auto port = reservation.local_endpoint().port();
  reservation.close();

  hydra::Hypervisor hypervisor(64, 1000, 64);
  auto radio = std::make_shared<hydra::VirtualRadio>(1, &hypervisor);
  BOOST_REQUIRE_EQUAL(radio->set_rx_chain(port, 1000, 16, "127.0.0.1", "127.0.0.1"), 0);
  BOOST_REQUIRE_EQUAL(radio->set_tx_chain(port, 1000, 16, "127.0.0.1", "127.0.0.1"), 0);
  hypervisor.attach_virtual_radio(radio);

  std::atomic<bool> valid{true};
  std::promise<void> start;
  auto ready = start.get_future().share();
  std::thread stream([&] {
    ready.wait();
    hydra::iq_window input(64, hydra::iq_sample(0.25f, 0));
    hydra::iq_window output(64);
    for (int i = 0; i < 200; ++i)
    {
      hypervisor.forward_rx_window(input, input.size());
      hypervisor.get_tx_window(output, output.size());
    }
  });
  std::thread configure([&] {
    ready.wait();
    for (int i = 0; i < 200; ++i)
    {
      if (radio->set_rx_freq(i % 2 ? 992 : 1008) < 0 ||
          radio->set_tx_freq(i % 2 ? 992 : 1008) < 0)
        valid = false;
    }
  });
  std::thread membership([&] {
    ready.wait();
    for (int i = 0; i < 32; ++i)
    {
      auto temporary = std::make_shared<hydra::VirtualRadio>(2, &hypervisor);
      hypervisor.attach_virtual_radio(temporary);
      if (hypervisor.get_vradio(2) != temporary ||
          !hypervisor.detach_virtual_radio(2))
        valid = false;
    }
  });
  std::thread stop_first([&] { ready.wait(); hypervisor.stop(); });
  std::thread stop_second([&] { ready.wait(); hypervisor.stop(); });
  start.set_value();
  stream.join();
  configure.join();
  membership.join();
  stop_first.join();
  stop_second.join();
  BOOST_CHECK(valid.load());
}
