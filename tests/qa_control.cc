#define BOOST_TEST_MODULE hydra_control_tests
#include <boost/test/included/unit_test.hpp>

#include "hydra/hydra_server.h"
#include "hydra/hydra_client.h"
#include "qa_helpers.h"

#include <exception>
#include <sstream>
#include <thread>

namespace {

class RunningServer
{
public:
  const unsigned short port = hydra_test::unused_tcp_port();
  std::shared_ptr<hydra::HydraCore> core = std::make_shared<hydra::HydraCore>();

  RunningServer()
      : server("127.0.0.1:" + std::to_string(port), "hydra-qa", core),
        worker([this] {
          try { server.run(); }
          catch (...) { failure = std::current_exception(); }
        })
  {
  }

  ~RunningServer()
  {
    server.stop();
    if (worker.joinable()) worker.join();
  }

  void stop()
  {
    server.stop();
    if (worker.joinable()) worker.join();
    if (failure) std::rethrow_exception(failure);
  }

private:
  hydra::HydraServer server;
  std::exception_ptr failure;
  std::thread worker;
};

class ScriptedServer
{
public:
  unsigned short port;

  explicit ScriptedServer(std::vector<std::string> replies) : socket(context, ZMQ_REP)
  {
    socket.set(zmq::sockopt::rcvtimeo, 3000);
    socket.set(zmq::sockopt::sndtimeo, 3000);
    socket.set(zmq::sockopt::linger, 0);
    socket.bind("tcp://127.0.0.1:*");
    const auto endpoint = socket.get(zmq::sockopt::last_endpoint);
    port = std::stoi(endpoint.substr(endpoint.rfind(':') + 1));
    worker = std::thread([this, replies = std::move(replies)] {
      try
      {
        for (const auto& reply : replies)
        {
          zmq::message_t request;
          if (!socket.recv(request, zmq::recv_flags::none))
            throw std::runtime_error("Missing native client request");
          zmq::message_t response(reply.data(), reply.size());
          if (!socket.send(response, zmq::send_flags::none))
            throw std::runtime_error("Native client reply timed out");
        }
      }
      catch (...) { failure = std::current_exception(); }
    });
  }

  ~ScriptedServer() { if (worker.joinable()) worker.join(); }

  void finish()
  {
    if (worker.joinable()) worker.join();
    if (failure) std::rethrow_exception(failure);
  }

private:
  zmq::context_t context;
  zmq::socket_t socket;
  std::exception_ptr failure;
  std::thread worker;
};

boost::property_tree::ptree request_reply(zmq::socket_t& socket, const std::string& payload)
{
  zmq::message_t request(payload.data(), payload.size());
  BOOST_REQUIRE(socket.send(request, zmq::send_flags::none).has_value());
  zmq::message_t reply;
  BOOST_REQUIRE(socket.recv(reply, zmq::recv_flags::none).has_value());
  std::istringstream input(std::string(static_cast<const char*>(reply.data()), reply.size()));
  boost::property_tree::ptree response;
  boost::property_tree::read_json(input, response);
  return response;
}

}

BOOST_AUTO_TEST_CASE(malformed_requests_receive_errors_without_breaking_rep_state)
{
  hydra_test::QuietLogs quiet;
  RunningServer server;
  zmq::context_t context;
  zmq::socket_t socket(context, ZMQ_REQ);
  socket.set(zmq::sockopt::rcvtimeo, 3000);
  socket.set(zmq::sockopt::sndtimeo, 3000);
  socket.set(zmq::sockopt::linger, 0);
  socket.connect("tcp://127.0.0.1:" + std::to_string(server.port));

  for (const std::string payload : {
      "", "{", "{\"xvl_syn\":{}", "{}", "[]", "\"not-an-object\"",
      "{\"\":{}}", "{\"xvl_syn\":{},\"xvl_que\":{}}"})
  {
    BOOST_TEST_CONTEXT("Request: " << payload)
    {
      const auto error = request_reply(socket, payload);
      BOOST_REQUIRE_EQUAL(error.size(), 1);
      BOOST_CHECK(!error.get<bool>("xvl_err.status"));
      const auto valid = request_reply(socket, "{\"xvl_syn\":{}}");
      BOOST_CHECK(valid.get<bool>("xvl_ack.status"));
    }
  }
  BOOST_CHECK_NO_THROW(server.stop());
}

BOOST_AUTO_TEST_CASE(repeated_allocations_keep_existing_endpoints)
{
  hydra_test::QuietLogs quiet;
  RunningServer server;
  auto device = std::make_shared<hydra_test::IdleDevice>();
  server.core->set_rx_resources(device, 1000, 64, 0, 64);
  server.core->set_tx_resources(device, 1000, 64, 0, 64);
  {
    hydra::hydra_client client("127.0.0.1", server.port, 1, "hydra-qa");
    client.override_server_host("127.0.0.1");
    hydra::rx_configuration rx(1000, 16), tx(1000, 16);
    BOOST_REQUIRE_EQUAL(client.request_rx_resources(rx), 0);
    BOOST_REQUIRE_EQUAL(client.request_tx_resources(tx), 0);
    const auto rx_port = rx.server_port;
    const auto tx_port = tx.server_port;
    BOOST_REQUIRE(rx_port > 1);
    BOOST_REQUIRE(tx_port > 1);
    rx.center_freq = tx.center_freq = 1008;
    BOOST_REQUIRE_EQUAL(client.request_rx_resources(rx), 0);
    BOOST_REQUIRE_EQUAL(client.request_tx_resources(tx), 0);
    BOOST_CHECK_EQUAL(rx.server_port, rx_port);
    BOOST_CHECK_EQUAL(tx.server_port, tx_port);
    const auto resources = server.core->query_resources();
    for (const auto& chunk : resources.get_child("receiver"))
      if (chunk.second.get<unsigned int>("id") == 1)
        BOOST_CHECK_EQUAL(chunk.second.get<int>("rx_udp_port"), rx_port);
  }
  BOOST_CHECK_NO_THROW(server.stop());
}

BOOST_AUTO_TEST_CASE(native_clients_reject_malformed_or_unusable_allocation_replies)
{
  hydra_test::QuietLogs quiet;
  const std::string valid = R"({"xvl_rep":{"status":true,"udp_port":33000}})";
  const std::vector<std::string> invalid{
      "", "{", "{}", "[]",
      R"({"xvl_rep":{"status":true}})",
      R"({"xvl_rep":{"status":false,"udp_port":33000}})",
      R"({"xvl_rep":{"status":true,"udp_port":0}})",
      R"({"xvl_rep":{"status":true,"udp_port":-1}})",
      R"({"xvl_rep":{"status":true,"udp_port":65536}})",
      R"({"xvl_rep":{"status":true,"udp_port":"invalid"}})",
      R"({"xvl_rep":{"status":true,"udp_port":33000},"extra":{}})",
      valid + std::string("\0garbage", 8)};
  std::vector<std::string> replies;
  for (const auto& reply : invalid)
  {
    replies.push_back(reply);
    replies.push_back(reply);
  }
  replies.insert(replies.end(), {valid, valid, R"({"xvl_cls":{"status":true}})"});
  ScriptedServer server(std::move(replies));
  {
    hydra::hydra_client client("127.0.0.1", server.port);
    client.override_server_host("127.0.0.1");
    hydra::rx_configuration rx(1000, 16), tx(1000, 16);
    rx.server_port = tx.server_port = 12345;
    rx.server_ip = tx.server_ip = "previous-endpoint";
    for (const auto& reply : invalid)
    {
      BOOST_TEST_CONTEXT("Reply: " << reply)
      {
        BOOST_CHECK_EQUAL(client.request_rx_resources(rx), -1);
        BOOST_CHECK_EQUAL(client.request_tx_resources(tx), -1);
        BOOST_CHECK_EQUAL(rx.server_port, 12345);
        BOOST_CHECK_EQUAL(tx.server_port, 12345);
        BOOST_CHECK_EQUAL(rx.server_ip, "previous-endpoint");
        BOOST_CHECK_EQUAL(tx.server_ip, "previous-endpoint");
      }
    }
    BOOST_CHECK_EQUAL(client.request_rx_resources(rx), 0);
    BOOST_CHECK_EQUAL(client.request_tx_resources(tx), 0);
    BOOST_CHECK_EQUAL(rx.server_port, 33000);
    BOOST_CHECK_EQUAL(tx.server_port, 33000);
    BOOST_CHECK_EQUAL(rx.server_ip, "127.0.0.1");
    BOOST_CHECK_EQUAL(tx.server_ip, "127.0.0.1");
  }
  BOOST_CHECK_NO_THROW(server.finish());
}
