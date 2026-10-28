#define BOOST_TEST_MODULE hydra_hypervisor_tests
#include <boost/test/included/unit_test.hpp>

#include "hydra/hydra_core.h"
#include "hydra/hydra_hypervisor.h"
#include "qa_helpers.h"

#include <memory>
#include <atomic>
#include <chrono>
#include <future>
#include <limits>
#include <thread>

using hydra_test::IdleDevice;
using hydra_test::QuietLogs;

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
      radio->set_rx_bandwidth(i % 2 ? 16 : 32);
      radio->set_tx_bandwidth(i % 2 ? 32 : 16);
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

BOOST_AUTO_TEST_CASE(invalid_and_sub_bin_requests_leave_resources_unchanged)
{
  QuietLogs quiet;
  BOOST_CHECK_THROW(hydra::fft_complex(0), std::invalid_argument);
  BOOST_CHECK_THROW(hydra::fft_complex(std::numeric_limits<size_t>::max()),
                    std::invalid_argument);

  auto device = std::make_shared<IdleDevice>();
  hydra::HydraCore core;
  BOOST_CHECK_THROW(core.set_rx_resources(device, 1000, 64, 0, 0),
                    std::invalid_argument);
  core.set_rx_resources(device, 1000, 64, 0, 64);
  core.set_tx_resources(device, 1000, 64, 0, 64);
  const auto before = core.query_resources();
  for (const double bandwidth : {0.0, -1.0, 0.5,
                                std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::infinity()})
  {
    BOOST_CHECK_EQUAL(core.request_rx_resources(
        1, 1000, bandwidth, "127.0.0.1", "127.0.0.1"), 0);
    BOOST_CHECK_EQUAL(core.request_tx_resources(
        1, 1000, bandwidth, "127.0.0.1", "127.0.0.1"), 0);
    BOOST_CHECK(core.query_resources() == before);
  }
}

BOOST_AUTO_TEST_CASE(one_bin_radio_rejects_smaller_reconfiguration)
{
  QuietLogs quiet;
  boost::asio::io_service io;
  boost::asio::ip::tcp::acceptor reservation(
      io, {boost::asio::ip::address_v4::loopback(), 0});
  const auto port = reservation.local_endpoint().port();
  reservation.close();
  hydra::Hypervisor hypervisor(64, 1000, 64);
  auto radio = std::make_shared<hydra::VirtualRadio>(1, &hypervisor);
  BOOST_CHECK_EQUAL(radio->set_rx_chain(port, 1000, 0.5, "127.0.0.1", "127.0.0.1"), -1);
  BOOST_CHECK_EQUAL(radio->set_tx_chain(port, 1000, 0.5, "127.0.0.1", "127.0.0.1"), -1);
  BOOST_CHECK(!radio->get_rx_enabled());
  BOOST_CHECK(!radio->get_tx_enabled());
  BOOST_REQUIRE_EQUAL(radio->set_rx_chain(port, 1000, 1, "127.0.0.1", "127.0.0.1"), 0);
  BOOST_REQUIRE_EQUAL(radio->set_tx_chain(port, 1000, 1, "127.0.0.1", "127.0.0.1"), 0);
  hypervisor.attach_virtual_radio(radio);
  radio->set_rx_bandwidth(0.5);
  radio->set_tx_bandwidth(0.5);
  BOOST_CHECK_EQUAL(radio->get_rx_bandwidth(), 1);
  BOOST_CHECK_EQUAL(radio->get_tx_bandwidth(), 1);
  BOOST_CHECK_EQUAL(radio->get_rx_fft(), 1);
  BOOST_CHECK_EQUAL(radio->get_tx_fft(), 1);
}

BOOST_AUTO_TEST_CASE(core_retunes_preserve_allocations_and_apply_frequency_and_width_together)
{
  QuietLogs quiet;
  for (const bool receiver : {false, true})
  {
    hydra::HydraCore core;
    auto device = std::make_shared<IdleDevice>();
    if (receiver) core.set_rx_resources(device, 1000, 64, 0, 64);
    else core.set_tx_resources(device, 1000, 64, 0, 64);
    auto request = [&](unsigned int id, double cf, double bw) {
      return receiver ? core.request_rx_resources(id, cf, bw, "127.0.0.1", "127.0.0.1")
                      : core.request_tx_resources(id, cf, bw, "127.0.0.1", "127.0.0.1");
    };
    const auto port = request(1, 984, 32);
    BOOST_REQUIRE(port > 1);
    BOOST_REQUIRE(request(2, 1008, 16) > 1);
    const auto before = core.query_resources();
    BOOST_CHECK_EQUAL(request(1, 1008, 16), 0);
    BOOST_CHECK_EQUAL(request(1, 1000, 128), 0);
    BOOST_CHECK(core.query_resources() == before);
    BOOST_REQUIRE_EQUAL(request(1, 1024, 16), port);
    const auto after = core.query_resources();
    const auto& chunks = after.get_child(receiver ? "receiver" : "transmitter");
    BOOST_CHECK_EQUAL(chunks.back().second.get<unsigned int>("id"), 1);
    BOOST_CHECK_EQUAL(chunks.back().second.get<double>("centre_freq"), 1024);
    BOOST_CHECK_EQUAL(chunks.back().second.get<double>("bandwidth"), 16);
    BOOST_CHECK_EQUAL(chunks.back().second.get<int>(
        receiver ? "rx_udp_port" : "tx_udp_port"), port);
    BOOST_REQUIRE(request(3, 984, 32) > 1);
  }
}

BOOST_AUTO_TEST_CASE(mapping_conflicts_leave_frequency_and_candidate_maps_unchanged)
{
  QuietLogs quiet;
  hydra::Hypervisor hypervisor(64, 1000, 64);
  auto first = std::make_shared<hydra::VirtualRadio>(1, &hypervisor);
  auto second = std::make_shared<hydra::VirtualRadio>(2, &hypervisor);
  for (const auto& radio : {first, second})
  {
    const auto port = hydra_test::unused_tcp_port();
    const double cf = radio == first ? 984 : 1016;
    BOOST_REQUIRE_EQUAL(radio->set_rx_chain(port, cf, 16, "127.0.0.1", "127.0.0.1"), 0);
    BOOST_REQUIRE_EQUAL(radio->set_tx_chain(port, cf, 16, "127.0.0.1", "127.0.0.1"), 0);
    hypervisor.attach_virtual_radio(radio);
  }
  BOOST_CHECK_LT(first->set_rx_freq(1016), 0);
  BOOST_CHECK_LT(first->set_tx_freq(1016), 0);
  BOOST_CHECK_EQUAL(first->get_rx_freq(), 984);
  BOOST_CHECK_EQUAL(first->get_tx_freq(), 984);
  BOOST_CHECK_EQUAL(second->get_rx_freq(), 1016);
  BOOST_CHECK_EQUAL(second->get_tx_freq(), 1016);
  hydra::iq_map_vec map(64, -1);
  map[10] = 2;
  const auto before = map;
  BOOST_CHECK_LT(hypervisor.set_rx_mapping(*first, map), 0);
  BOOST_CHECK(map == before);
  BOOST_CHECK_LT(hypervisor.set_tx_mapping(*first, map), 0);
  BOOST_CHECK(map == before);
}

BOOST_AUTO_TEST_CASE(framing_reconfiguration_discards_old_windows_and_keeps_buffer_connections)
{
  QuietLogs quiet;
  auto input = std::make_shared<hydra::hydra_buffer<hydra::iq_sample>>(8000);
  hydra::resampler<hydra::iq_sample, hydra::iq_window> pack(input, 64, 8);
  auto output = pack.buffer();
  hydra::iq_window old_samples(16, {9, 9});
  input->write(old_samples.begin(), old_samples.end());
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (output->size() != 2 && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  BOOST_REQUIRE_EQUAL(output->size(), 2);
  BOOST_CHECK_THROW(pack.reconfigure(0), std::invalid_argument);
  BOOST_CHECK_THROW(pack.reconfigure(std::numeric_limits<size_t>::max()),
                    std::invalid_argument);
  BOOST_CHECK_EQUAL(output->size(), 2);
  for (const size_t size : {16, 4})
  {
    pack.reconfigure(size);
    BOOST_CHECK_EQUAL(output->size(), 0);
    BOOST_CHECK_EQUAL(input->capacity(), 1000 * size);
    hydra::iq_window expected(size, {0.25f, -0.5f});
    input->write(expected.begin(), expected.end());
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    hydra::iq_window actual;
    while (actual.empty() && std::chrono::steady_clock::now() < limit)
    {
      actual = output->read_one();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
  }

  auto windows = std::make_shared<hydra::hydra_buffer<hydra::iq_window>>(1000);
  hydra::resampler<hydra::iq_window, hydra::iq_sample> unpack(windows, 64, 8);
  auto samples = unpack.buffer();
  unpack.reconfigure(32);
  BOOST_CHECK_EQUAL(samples->capacity(), 32000);
  hydra::iq_window expected(32, {0.5f, 0.25f});
  windows->write(expected);
  const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (samples->size() != expected.size() && std::chrono::steady_clock::now() < limit)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  const auto actual = samples->read(expected.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}
