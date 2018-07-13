#include "hydra/hydra_client.h"

xvl_client::xvl_client(const std::string s_host,
                       const unsigned int u_port,
                       const unsigned int u_client_id,
                       const bool b_debug)
{
  s_server_host = s_host;
  s_server_port = std::to_string(u_port);
  u_id = u_client_id;
  b_debug_flag = b_debug;
}

std::string xvl_client::factory(const std::string &s_message)
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

std::string xvl_client::check_connection(void)
{
  // Set message type
  std::string message = "{\"xvl_syn\":\"\"}";
  // Send message and return acknowledgement
  return xvl_client::factory(message);
}

std::string xvl_client::query_resources(void)
{
  // Set message type
  std::string message = "{\"xvl_que\":\"\"}";
  // Send message and return acknowledgement
  return xvl_client::factory(message);
}

int xvl_client::request_rx_resources(const double d_centre_freq,
                                     const double d_bandwidth)
{
  // If ill defined one of the parameters
  if (not bool(d_centre_freq) or not bool(d_bandwidth))
    {
      std::cerr << "Missing RX information!" << std::endl;
    }

  // Set message type
  std::string message = "{\"xvl_rrx\":{\"id\":" + std::to_string(u_id) + "," +
    ("\"centre_freq\":" + std::to_string(d_centre_freq) + ",") +
    ("\"bandwidth\":" + std::to_string(d_bandwidth) + "}}");

  std::stringstream ss;
  // Return the result of the request message
  ss << xvl_client::factory(message);

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

int xvl_client::request_tx_resources(const double d_centre_freq,
                                     const double d_bandwidth)
{
  // If ill defined one of the parameters
  if (not bool(d_centre_freq) or not bool(d_bandwidth))
    {
      std::cerr << "Missing TX information!" << std::endl;
    }

  // Set message type
  std::string message = "{\"xvl_rtx\":{\"id\":" + std::to_string(u_id) + "," +
    ("\"centre_freq\":" + std::to_string(d_centre_freq) + ",") +
    ("\"bandwidth\":" + std::to_string(d_bandwidth) + "}}");

  // Return the result of the request message
  std::stringstream ss;
  ss << xvl_client::factory(message);

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

std::string xvl_client::free_resources ()
{
  // Set message type
  std::string message = "{\"xvl_fre\":{\"id\":" + std::to_string(u_id) + "}}";
  // Send message and return acknowledgement
  return xvl_client::factory(message);
}
