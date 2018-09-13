#include "hydra/hydra_client.h"

namespace hydra {

hydra_client::hydra_client(std::string client_ip,
                           std::string server_ip,
                           unsigned int u_port,
                           unsigned int u_client_id,
                           bool b_debug)
{
  s_client_host = client_ip;
  s_server_host = server_ip;
  s_server_port = std::to_string(u_port);
  u_id = u_client_id;
  b_debug_flag = b_debug;
}

hydra_client::~hydra_client()
{
  free_resources();
}

int
hydra_client::request_rx_resources(double d_centre_freq,
                                   double d_bandwidth,
                                   bool bpad)
{
  // If ill defined one of the parameters
  if (not bool(d_centre_freq) or not bool(d_bandwidth))
  {
    std::cerr << "Missing RX information!" << std::endl;
  }


  // Set message type
  std::string message = "{\"xvl_rrx\":{\"id\":" + std::to_string(u_id) + "," +
    "\"centre_freq\":" + std::to_string(d_centre_freq) + "," +
    "\"bandwidth\":" + std::to_string(d_bandwidth) + ", " +
    "\"ip\":\"" + s_client_host + "\", " +
    "\"padding\":" + std::to_string(bpad) + "}}";

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
    return 0;
  }

  bool success = root.get("xvl_rep.status", false);

  if (success)
  {
    return root.get("xvl_rep.udp_port", 0);
  }
  return 0;
}

int
hydra_client::request_tx_resources(double d_centre_freq,
                                   double d_bandwidth,
                                   bool bpad)
{
  // If ill defined one of the parameters
  if (not bool(d_centre_freq) or not bool(d_bandwidth))
    {
      std::cerr << "Missing TX information!" << std::endl;
    }

  // Set message type
  std::string message = "{\"xvl_rtx\":{\"id\":" + std::to_string(u_id) + "," +
    "\"centre_freq\":" + std::to_string(d_centre_freq) + "," +
    "\"padding\":" + std::to_string(bpad) + "," +
    "\"bandwidth\":" + std::to_string(d_bandwidth) + "}}";

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
      return root.get("xvl_rep.udp_port", 0);
    }


  return 0;
}

std::string
hydra_client::check_connection()
{
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
  //  Prepare our context and socket
  zmq::context_t context(1);
  zmq::socket_t socket (context, ZMQ_REQ);

  // If printing debug messages
  if (b_debug_flag)
  {
    std::cout << "Connecting to XVL server..." << std::endl;
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
