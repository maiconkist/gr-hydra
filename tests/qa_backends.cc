#define BOOST_TEST_MODULE hydra_backend_tests
#include <boost/test/included/unit_test.hpp>

#include "hydra/hydra_hypervisor.h"
#include "qa_helpers.h"

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

namespace {

struct SampleFile
{
  std::string path;

  explicit SampleFile(const hydra::iq_window& samples)
  {
    char name[] = "/tmp/hydra-qa-XXXXXX";
    const int fd = mkstemp(name);
    if (fd < 0) throw std::runtime_error("Unable to create sample fixture");
    path = name;
    close(fd);
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(samples.data()),
                 samples.size() * sizeof(hydra::iq_sample));
    if (!output)
    {
      std::remove(path.c_str());
      throw std::runtime_error("Unable to write sample fixture");
    }
  }

  ~SampleFile() { std::remove(path.c_str()); }
};

std::unique_ptr<zmq::socket_t> handshake_monitor(zmq::socket_t& socket,
                                                zmq::context_t& context)
{
  const char* endpoint = "inproc://hydra-qa-handshake";
  BOOST_REQUIRE_EQUAL(zmq_socket_monitor(
      socket.handle(), endpoint, ZMQ_EVENT_HANDSHAKE_SUCCEEDED), 0);
  auto monitor = std::make_unique<zmq::socket_t>(context, ZMQ_PAIR);
  monitor->set(zmq::sockopt::rcvtimeo, 3000);
  monitor->set(zmq::sockopt::linger, 0);
  monitor->connect(endpoint);
  return monitor;
}

void await_handshake(zmq::socket_t& monitor)
{
  zmq::message_t event, endpoint;
  BOOST_REQUIRE(monitor.recv(event, zmq::recv_flags::none).has_value());
  BOOST_REQUIRE(monitor.recv(endpoint, zmq::recv_flags::none).has_value());
}

class ChunkedDevice : public hydra::abstract_device
{
public:
  std::atomic<bool> enabled{false};

  explicit ChunkedDevice(hydra::iq_window samples) : samples(std::move(samples)) {}

  size_t receive(hydra::iq_window& output, size_t len) override
  {
    if (!enabled || chunk_index == chunks.size()) return 0;
    std::fill(output.begin(), output.end(), hydra::iq_sample(9999, 9999));
    const auto count = chunks[chunk_index++];
    if (count > len) return count;
    std::copy_n(samples.begin() + position, count, output.begin());
    position += count;
    return count;
  }

private:
  const std::array<size_t, 8> chunks{{3, 0, 2, 3, 17, 5, 0, 3}};
  hydra::iq_window samples;
  size_t position = 0;
  size_t chunk_index = 0;
};

}

BOOST_AUTO_TEST_CASE(file_reads_report_complete_samples_and_eof)
{
  hydra_test::QuietLogs quiet;
  const hydra::iq_window input{{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}};
  SampleFile file(input);
  hydra::device_file device(file.path);
  device.set_rx_config(1000, 1e6, 0);
  hydra::iq_window output(8, {99, 99});
  BOOST_REQUIRE_EQUAL(device.receive(output, 3), 3);
  BOOST_CHECK_EQUAL_COLLECTIONS(output.begin(), output.begin() + 3,
                                input.begin(), input.begin() + 3);
  BOOST_REQUIRE_EQUAL(device.receive(output, 8), 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(output.begin(), output.begin() + 2,
                                input.begin() + 3, input.end());
  BOOST_CHECK_EQUAL(device.receive(output, 8), 0);
  BOOST_CHECK_EQUAL(device.receive(output, 0), 0);
}

BOOST_AUTO_TEST_CASE(image_input_reads_are_counted_and_rewind_per_instance)
{
  hydra_test::QuietLogs quiet;
  const hydra::iq_window input{{1, 0}, {2, 0}, {3, 0}};
  SampleFile file(input);
  hydra::device_image_gen first(file.path), second(file.path);
  hydra::iq_window output(4);
  BOOST_REQUIRE_EQUAL(first.receive(output, 2), 2);
  BOOST_REQUIRE_EQUAL(first.receive(output, 4), 1);
  BOOST_CHECK_EQUAL(output.front(), input.back());
  BOOST_REQUIRE_EQUAL(first.receive(output, 4), 3);
  BOOST_REQUIRE_EQUAL(second.receive(output, 4), 3);
  BOOST_CHECK_EQUAL_COLLECTIONS(output.begin(), output.begin() + 3,
                                input.begin(), input.end());
  hydra::iq_window empty;
  BOOST_CHECK_EQUAL(first.receive(empty, 4), 0);
}

BOOST_AUTO_TEST_CASE(loopback_preserves_remainders_and_respects_buffer_bounds)
{
  hydra::device_loopback device;
  const hydra::iq_window input{{1, 0}, {2, 0}, {3, 0}, {4, 0}};
  device.send(input, 3);
  hydra::iq_window output(2);
  BOOST_REQUIRE_EQUAL(device.receive(output, 1), 1);
  BOOST_CHECK_EQUAL(output.front(), input.front());
  BOOST_REQUIRE_EQUAL(device.receive(output, 8), 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(output.begin(), output.end(),
                                input.begin() + 1, input.begin() + 3);
  BOOST_CHECK_EQUAL(device.receive(output, 2), 0);
}

BOOST_AUTO_TEST_CASE(network_reads_return_samples_and_preserve_message_remainders)
{
  hydra_test::QuietLogs quiet;
  zmq::context_t context;
  zmq::socket_t peer(context, ZMQ_PUSH);
  peer.set(zmq::sockopt::sndtimeo, 3000);
  peer.set(zmq::sockopt::linger, 0);
  auto monitor = handshake_monitor(peer, context);
  peer.bind("tcp://127.0.0.2:5556");
  hydra::device_network device("127.0.0.2", "127.0.0.2");
  hydra::iq_window output(3);
  BOOST_CHECK_EQUAL(device.receive(output, 3), 0);
  await_handshake(*monitor);
  const hydra::iq_window input{{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}};
  zmq::message_t message(input.data(), input.size() * sizeof(hydra::iq_sample));
  BOOST_REQUIRE(peer.send(message, zmq::send_flags::none).has_value());
  BOOST_REQUIRE_EQUAL(device.receive(output, 2), 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(output.begin(), output.begin() + 2,
                                input.begin(), input.begin() + 2);
  BOOST_REQUIRE_EQUAL(device.receive(output, 8), 3);
  BOOST_CHECK_EQUAL_COLLECTIONS(output.begin(), output.end(),
                                input.begin() + 2, input.end());
  zmq::message_t malformed(3);
  std::memset(malformed.data(), 0, malformed.size());
  BOOST_REQUIRE(peer.send(malformed, zmq::send_flags::none).has_value());
  BOOST_CHECK_EQUAL(device.receive(output, 3), 0);
}

BOOST_AUTO_TEST_CASE(iq_source_decodes_small_messages_without_unaligned_access)
{
  hydra_test::QuietLogs quiet;
  zmq::context_t context;
  zmq::socket_t peer(context, ZMQ_PUSH);
  peer.set(zmq::sockopt::sndtimeo, 3000);
  peer.set(zmq::sockopt::linger, 0);
  auto monitor = handshake_monitor(peer, context);
  peer.bind("tcp://127.0.0.1:*");
  const auto endpoint = peer.get(zmq::sockopt::last_endpoint);
  auto source = hydra::zmq_source::make(
      "127.0.0.1", "127.0.0.1", endpoint.substr(endpoint.rfind(':') + 1), 8);
  await_handshake(*monitor);
  zmq::message_t malformed(11);
  std::memset(malformed.data(), 0, malformed.size());
  BOOST_REQUIRE(peer.send(malformed, zmq::send_flags::none).has_value());
  const hydra::iq_window input{{1, 2}, {3, 4}, {5, 6}};
  zmq::message_t message(input.data(), input.size() * sizeof(hydra::iq_sample));
  BOOST_REQUIRE(peer.send(message, zmq::send_flags::none).has_value());
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  hydra::iq_window output;
  while (output.empty() && std::chrono::steady_clock::now() < deadline)
  {
    output = source->buffer()->read(input.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  source->stop();
  BOOST_REQUIRE_EQUAL(output.size(), input.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(output.begin(), output.end(), input.begin(), input.end());
}

BOOST_AUTO_TEST_CASE(rx_accumulates_short_reads_without_forwarding_stale_samples)
{
  hydra_test::QuietLogs quiet;
  hydra::iq_window input(16);
  for (size_t i = 0; i < input.size(); ++i)
    input[i] = hydra::iq_sample(float(i) / 100, -float(i) / 200);
  auto device = std::make_shared<ChunkedDevice>(input);
  hydra::Hypervisor hypervisor;
  hypervisor.set_rx_resources(device, 1000, 8, 8);
  const auto port = hydra_test::unused_tcp_port();
  auto radio = std::make_shared<hydra::VirtualRadio>(1, &hypervisor);
  BOOST_REQUIRE_EQUAL(radio->set_rx_chain(
      port, 1000, 8, "127.0.0.1", "127.0.0.1"), 0);
  hypervisor.attach_virtual_radio(radio);

  zmq::context_t context;
  zmq::socket_t receiver(context, ZMQ_PULL);
  receiver.set(zmq::sockopt::rcvtimeo, 3000);
  receiver.set(zmq::sockopt::linger, 0);
  auto monitor = handshake_monitor(receiver, context);
  receiver.connect("tcp://127.0.0.1:" + std::to_string(port));
  await_handshake(*monitor);
  device->enabled = true;
  hydra::iq_window received;
  while (received.size() < input.size())
  {
    zmq::message_t message;
    BOOST_REQUIRE(receiver.recv(message, zmq::recv_flags::none).has_value());
    BOOST_REQUIRE_EQUAL(message.size() % sizeof(hydra::iq_sample), 0);
    const auto old_size = received.size();
    received.resize(old_size + message.size() / sizeof(hydra::iq_sample));
    std::memcpy(received.data() + old_size, message.data(), message.size());
  }
  hypervisor.stop();
  BOOST_REQUIRE_EQUAL(received.size(), input.size());
  for (size_t i = 0; i < input.size(); ++i)
    BOOST_CHECK_SMALL(std::abs(received[i] - input[i]), 1e-5f);
}

BOOST_AUTO_TEST_CASE(rejected_retunes_keep_the_existing_rx_stream_alive)
{
  hydra_test::QuietLogs quiet;
  hydra::Hypervisor hypervisor(64, 1000, 64);
  const auto port = hydra_test::unused_tcp_port();
  auto first = std::make_shared<hydra::VirtualRadio>(1, &hypervisor);
  auto neighbor = std::make_shared<hydra::VirtualRadio>(2, &hypervisor);
  BOOST_REQUIRE_EQUAL(first->set_rx_chain(port, 1000, 8, "127.0.0.1", "127.0.0.1"), 0);
  BOOST_REQUIRE_EQUAL(neighbor->set_rx_chain(
      hydra_test::unused_tcp_port(), 1016, 16, "127.0.0.1", "127.0.0.1"), 0);
  hypervisor.attach_virtual_radio(first);
  hypervisor.attach_virtual_radio(neighbor);
  zmq::context_t context;
  zmq::socket_t receiver(context, ZMQ_PULL);
  receiver.set(zmq::sockopt::rcvtimeo, 3000);
  receiver.set(zmq::sockopt::linger, 0);
  auto monitor = handshake_monitor(receiver, context);
  receiver.connect("tcp://127.0.0.1:" + std::to_string(port));
  await_handshake(*monitor);
  hydra::iq_window input(64, {0.25f, 0});
  for (int attempt = 0; attempt < 3; ++attempt)
  {
    if (attempt == 1) BOOST_CHECK_LT(first->set_rx_freq(1016), 0);
    if (attempt == 2) first->set_rx_bandwidth(48);
    BOOST_CHECK_EQUAL(first->get_rx_freq(), 1000);
    BOOST_CHECK_EQUAL(first->get_rx_bandwidth(), 8);
    BOOST_CHECK_EQUAL(first->get_rx_udp_port(), port);
    hypervisor.forward_rx_window(input, input.size());
    zmq::message_t message;
    BOOST_REQUIRE(receiver.recv(message, zmq::recv_flags::none).has_value());
    hydra::iq_window output(8);
    BOOST_REQUIRE_EQUAL(message.size(), output.size() * sizeof(hydra::iq_sample));
    std::memcpy(output.data(), message.data(), message.size());
    for (const auto sample : output)
      BOOST_CHECK_SMALL(std::abs(sample - hydra::iq_sample(0.25f, 0)), 1e-5f);
  }
}

BOOST_AUTO_TEST_CASE(bandwidth_changes_rebuild_rx_and_tx_dsp_without_changing_endpoints)
{
  hydra_test::QuietLogs quiet;
  hydra::Hypervisor hypervisor(64, 1000, 64);
  zmq::context_t rx_context, tx_context;
  zmq::socket_t receiver(rx_context, ZMQ_PULL), sender(tx_context, ZMQ_PUSH);
  receiver.set(zmq::sockopt::rcvtimeo, 3000);
  receiver.set(zmq::sockopt::linger, 0);
  sender.set(zmq::sockopt::sndtimeo, 3000);
  sender.set(zmq::sockopt::linger, 0);
  auto rx_monitor = handshake_monitor(receiver, rx_context);
  auto tx_monitor = handshake_monitor(sender, tx_context);
  sender.bind("tcp://127.0.0.1:*");
  const auto tx_endpoint = sender.get(zmq::sockopt::last_endpoint);
  const auto tx_port = std::stoi(tx_endpoint.substr(tx_endpoint.rfind(':') + 1));
  const auto rx_port = hydra_test::unused_tcp_port();
  auto radio = std::make_shared<hydra::VirtualRadio>(1, &hypervisor);
  BOOST_REQUIRE_EQUAL(radio->set_rx_chain(rx_port, 1000, 8, "127.0.0.1", "127.0.0.1"), 0);
  BOOST_REQUIRE_EQUAL(radio->set_tx_chain(tx_port, 1000, 16, "127.0.0.1", "127.0.0.1"), 0);
  hypervisor.attach_virtual_radio(radio);
  receiver.connect("tcp://127.0.0.1:" + std::to_string(rx_port));
  await_handshake(*rx_monitor);
  await_handshake(*tx_monitor);

  const std::array<std::pair<size_t, size_t>, 4> widths{{{8, 16}, {32, 8}, {16, 32}, {8, 16}}};
  for (const auto [rx_size, tx_size] : widths)
  {
    radio->set_rx_bandwidth(rx_size);
    radio->set_tx_bandwidth(tx_size);
    BOOST_REQUIRE_EQUAL(radio->get_rx_fft(), rx_size);
    BOOST_REQUIRE_EQUAL(radio->get_tx_fft(), tx_size);
    BOOST_CHECK_EQUAL(radio->get_rx_udp_port(), rx_port);
    BOOST_CHECK_EQUAL(radio->get_tx_udp_port(), tx_port);

    const hydra::iq_sample rx_value(float(rx_size) / 100, -0.25f);
    hydra::iq_window wideband(64, rx_value);
    hypervisor.forward_rx_window(wideband, wideband.size());
    hydra::iq_window received;
    while (received.size() < rx_size)
    {
      zmq::message_t message;
      BOOST_REQUIRE(receiver.recv(message, zmq::recv_flags::none).has_value());
      BOOST_REQUIRE_GT(message.size(), 0);
      BOOST_REQUIRE_EQUAL(message.size() % sizeof(hydra::iq_sample), 0);
      const auto offset = received.size();
      received.resize(offset + message.size() / sizeof(hydra::iq_sample));
      std::memcpy(received.data() + offset, message.data(), message.size());
    }
    BOOST_REQUIRE_EQUAL(received.size(), rx_size);
    for (const auto sample : received)
      BOOST_CHECK_SMALL(std::abs(sample - rx_value), 1e-5f);

    const hydra::iq_sample tx_value(float(tx_size) / 100, 0.125f);
    hydra::iq_window narrowband(tx_size + tx_size / 2, {9, 9});
    std::fill_n(narrowband.begin(), tx_size, tx_value);
    zmq::message_t message(narrowband.data(), narrowband.size() * sizeof(hydra::iq_sample));
    BOOST_REQUIRE(sender.send(message, zmq::send_flags::none).has_value());
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool produced = false;
    do
    {
      hypervisor.get_tx_window(wideband, 64);
      produced = std::any_of(wideband.begin(), wideband.end(),
                            [](auto sample) { return std::abs(sample) > 1e-6f; });
      if (!produced) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (!produced && std::chrono::steady_clock::now() < deadline);
    BOOST_REQUIRE(produced);
    for (const auto sample : wideband)
      BOOST_CHECK_SMALL(std::abs(sample - tx_value), 1e-5f);
  }
  hypervisor.stop();
}
