#define BOOST_TEST_MODULE hydra_hypervisor_tests
#include <boost/test/included/unit_test.hpp>

#include "hydra/hydra_core.h"
#include "hydra/hydra_hypervisor.h"

#include <memory>
#include <chrono>
#include <thread>

namespace {

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
