#include "hydra/hydra_client.h"

namespace hydra {

hydra_client::hydra_client(std::string client_ip,
                           unsigned int u_port,
                           unsigned int u_client_id,
                           std::string s_group_name,
                           bool b_debug):
  b_debug_flag(b_debug),
  s_client_host(client_ip),
  s_group(s_group_name)
{
  s_server_port = std::to_string(u_port);

  std::cout << boost::format("s_group: %s - s_client_host: %s -  s_server_host: %s") % s_group % s_client_host % s_server_host << std::endl;
}

hydra_client::~hydra_client()
{
  free_resources();
}

int
hydra_client::request_rx_resources(rx_configuration &rx_conf)
{
  // If ill defined one of the parameters
  if (not bool(rx_conf.center_freq) or not bool(rx_conf.bandwidth) or s_server_host == "")
  {
    std::cerr << "Missing RX information!" << std::endl;
    return -1;
  }

  // Set message type
  std::string message = "{\"xvl_rrx\":{\"id\":" + std::to_string(u_id) + "," +
    "\"centre_freq\":" + std::to_string(rx_conf.center_freq) + "," +
    "\"bandwidth\":" + std::to_string(rx_conf.bandwidth) + ", " +
    "\"ip\":\"" + s_client_host + "\"" + "}}";

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
hydra_client::discover_server(
    std::string client_ip,
    std::string &server_ip)
{
    // If printing debug messages
    if (b_debug_flag)
    {
      // Print the response data
      std::cout <<  "<client> Discovering server" << std::endl;;
    }

   const int MAX_MSG = 100;
   send_udp(client_ip, s_group + ":" + client_ip, true, 5001);

   char msg[MAX_MSG];
   if (recv_udp(msg, MAX_MSG, false, 5002, {5, 0}))
   {
      std::cout << "<client> Error occurred. Timeout Exceeded" << std::endl;
      return -1;
   }
   else
   {
     // If printing debug messages
     if (b_debug_flag)
     {
       // Print the response data
      std::cout << "<client> Received: " << msg << std::endl;
     }

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
  if (not bool(tx_conf.center_freq) or not bool(tx_conf.bandwidth) or s_server_host == "")
  {
    std::cerr << "Missing TX information!" << std::endl;
    return -1;
  }

  // Set message type
  std::string message = "{\"xvl_rtx\":{\"id\":" + std::to_string(u_id) + "," +
    "\"centre_freq\":" + std::to_string(tx_conf.center_freq) + "," +
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
     std::cout << boost::format("status: %d -- tries: %d") % status % tries << std::endl;
     if (status < 0 && tries >= max_tries) return std::string("");

     sleep(1);
   }

   std::cout << boost::format("status: %d -- tries: %d") % status % tries << std::endl;

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
  if (s_server_host == "")
    return "";

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
  // Timeout to get out of the while loop since recv is blocking
  int timeout = 5000;
  socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

  // If printing debug messages
  if (b_debug_flag)
  {
     std::cout << boost::format("<client> Connecting to XVL server: %s:%s") % s_server_host % s_server_port << std::endl;
  }
  // Connect to the XVL Server
  socket.connect (("tcp://" + s_server_host + ":" + s_server_port).c_str());

  // If printing debug messages
  if (b_debug_flag)
  {
    std::cout << "<client> Sending: " << s_message.data() << std::endl;
  }

  // Create ZMQ message type and copy the message to it
  zmq::message_t request (s_message.size());
  memcpy (request.data(), s_message.data(), s_message.size());
  // Send message
  socket.send (request);

  //  Get the reply.
  zmq::message_t reply;

  int rc = 0;
  rc = socket.recv (&reply);

  if (rc)
  {
    // Extract the text from the reply message
    std::string s_response = std::string(static_cast<char*>(reply.data()),
                                         reply.size());

    // If printing debug messages
    if (b_debug_flag)
    {
      // Print the response data
      std::cout << "<client> Received message: " << s_response << std::endl;
    }

    // Return the reply data
    return s_response.data();
  }
  else
  {
    std::cerr << "<client> Server timeout." << std::endl;
    exit(20);
  }
}

} /* namespace hydra */
