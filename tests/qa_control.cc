#define BOOST_TEST_MODULE hydra_control_tests
#include <boost/test/included/unit_test.hpp>

#include "hydra/hydra_server.h"
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
