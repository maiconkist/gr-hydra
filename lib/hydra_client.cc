#include "hydra/hydra_client.h"
#include "hydra/util/udp.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <zmq.hpp>

namespace hydra {

hydra_client::hydra_client(std::string client_ip,
                           unsigned int u_port,
                           unsigned int u_client_id,
                           bool b_debug)
{
  s_client_host = client_ip;
  s_server_port = std::to_string(u_port);
  u_id = u_client_id;
  b_debug_flag = b_debug;

  std::cout << boost::format("s_client_host: %s -  s_server_host: %s") % s_client_host % s_server_host << std::endl;
}

hydra_client::~hydra_client()
{
  free_resources();
}

int
hydra_client::request_rx_resources(rx_configuration &rx_conf)
{
  // If ill defined one of the parameters
   if (not bool(rx_conf.center_freq) or not bool(rx_conf.bandwidth))
  {
    std::cerr << "Missing RX information!" << std::endl;
  }

  // Set message type
  std::string message = "{\"xvl_rrx\":{\"id\":" + std::to_string(u_id) + "," +
    "\"centre_freq\":" + std::to_string(rx_conf.center_freq) + "," +
    "\"bandwidth\":" + std::to_string(rx_conf.bandwidth) + ", " +
    "\"ip\":\"" + s_client_host + "\", " +
    "\"padding\":" + std::to_string(rx_conf.bpad) + "}}";

  std::stringstream ss;
  // Return the result of the request message
  ss << factory(message);

  // Property Tree Object
  boost::property_tree::ptree root;

  // Try to load the input stream as JSON
  try
  {
    boost::property_tree::read_json(ss, root);
  }
  catch (const boost::property_tree::json_parser::json_parser_error &e)
  {
    return -1;
  }

  bool success = root.get("xvl_rep.status", false);

  if (success)
  {
    rx_conf.server_port = root.get("xvl_rep.udp_port", 0);
    rx_conf.server_ip = s_server_host;

    return 0;
  }

  return -1;
}


int
hydra_client::discover_server(std::string client_ip,
                std::string &server_ip)
{
   const int MAX_MSG = 1000;
   send_udp(client_ip, client_ip, true, 5001);

   char msg[MAX_MSG];
   if (recv_udp(msg, MAX_MSG, false, 5002, {4, 0}))
   {
      std::cout << "Error occurred. Timeout Exceeded" << std::endl;
      return -1;
   }
   else
   {
      std::cout << "Received: " << msg << std::endl;

      std::vector<std::string> sp;
      boost::split(sp, msg, [](char c){return c == ':';});
      server_ip = sp[0];
   }

   return 0;
}


int
hydra_client::request_tx_resources(rx_configuration &tx_conf)
{
  // If ill defined one of the parameters
  if (not bool(tx_conf.center_freq) or not bool(tx_conf.bandwidth))
  {
    std::cerr << "Missing TX information!" << std::endl;
  }

  // Set message type
  std::string message = "{\"xvl_rtx\":{\"id\":" + std::to_string(u_id) + "," +
    "\"centre_freq\":" + std::to_string(tx_conf.center_freq) + "," +
    "\"padding\":" + std::to_string(tx_conf.bpad) + "," +
    "\"ip\":\"" + s_client_host + "\", " +
    "\"bandwidth\":" + std::to_string(tx_conf.bandwidth) + "}}";

  // Return the result of the request message
  std::stringstream ss;
  ss << factory(message);

  // Property Tree Object
  boost::property_tree::ptree root;

  // Try to load the input stream as JSON
  try
  {
    boost::property_tree::read_json(ss, root);
  }
  catch (const boost::property_tree::json_parser::json_parser_error &e)
  {
    return 0;
  }

  bool success = root.get("xvl_rep.status", false);

  if (success)
  {
     tx_conf.server_port = root.get("xvl_rep.udp_port", 0);
     tx_conf.server_ip = s_server_host;
     return 0;
  }

  return -1;
}

std::string
hydra_client::check_connection(size_t max_tries)
{
   size_t tries = 0;
   int status;

   while ( (status = discover_server(s_client_host, s_server_host)) < 0 &&
           (tries++ < max_tries))
   {
     if (status < 0 && tries >= max_tries) return std::string("");

     sleep(1);
   }

   // Set message type
   std::string message = "{\"xvl_syn\":\"\"}";
   // Send message and return acknowledgement
   return factory(message);
}

std::string
hydra_client::query_resources()
{
  // Set message type
  std::string message = "{\"xvl_que\":\"\"}";
  // Send message and return acknowledgement
  return factory(message);
}

std::string
hydra_client::free_resources()
{
  // Set message type
  std::string message = "{\"xvl_fre\":{\"id\":" + std::to_string(u_id) + "}}";
  // Send message and return acknowledgement
  return factory(message);
}

std::string
hydra_client::factory(const std::string &s_message)
{
  std::cout << s_message << std::endl;

  //  Prepare our context and socket
  zmq::context_t context(1);
  zmq::socket_t socket (context, ZMQ_REQ);

  // If printing debug messages
  if (b_debug_flag)
  {
     std::cout << boost::format("Connecting to XVL server: %s:%s") % s_server_host % s_server_port << std::endl;
  }
  // Connect to the XVL Server
  socket.connect (("tcp://" + s_server_host + ":" + s_server_port).c_str());

  // If printing debug messages
  if (b_debug_flag)
  {
    std::cout << "Sending:\t" << s_message.data() << std::endl;
  }

  // Create ZMQ message type and copy the message to it
  zmq::message_t request (s_message.size());
  memcpy (request.data(), s_message.data(), s_message.size());
  // Send message
  socket.send (request);

  //  Get the reply.
  zmq::message_t reply;
  socket.recv (&reply);

  // Extract the text from the reply message
  std::string s_response = std::string(static_cast<char*>(reply.data()),
                                       reply.size());

  // If printing debug messages
  if (b_debug_flag)
  {
    // Print the response data
    std::cout << s_response << std::endl;
  }

  // Return the reply data
  return s_response.data();
}

} /* namespace hydra */
