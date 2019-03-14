#include "hydra/hydra_server.h"
#include "hydra/util/udp.h"

#include <boost/algorithm/string.hpp>

namespace hydra {

HydraServer::HydraServer(std::string server_addr,
                         std::shared_ptr<HydraCore> core)
{
  // Get the server port
  s_server_addr = server_addr;

  // Pointer to the XVL Core
  p_core = core;

  // Change the server status
  server_info.s_status = "Idle";
}

int
HydraServer::auto_discovery()
{
   const int MAX_MSG = 1000;
   char msg[MAX_MSG];

   while (1)
   {
      recv_udp(msg, MAX_MSG, true, 5001);
      send_udp(msg, s_server_addr, false, 5002);
   }
}

// Run the server
int
HydraServer::run()
{
  //  Prepare our context and socket
  zmq::context_t context;
  zmq::socket_t socket (context, ZMQ_REP);
  socket.bind (("tcp://" + s_server_addr).c_str());

  // Change the server status
  server_info.s_status = "Enabled";

  std::thread autod = std::thread(&HydraServer::auto_discovery, this);

  // Message type object
  zmq::message_t request;

  std::string server_addr_no_port;
  {
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, s_server_addr, boost::is_any_of(":"));
    server_addr_no_port = tokens[0];
  }

  // Even loop
  while (true)
  {
    //  Wait for next request from client
    socket.recv (&request);

    // Unpack string from message_t and push it to a string stream
    std::stringstream ss; ss << std::string(static_cast<char*>(request.data()),
                                         request.size());

    std::cout << std::string(static_cast<char*>(request.data()), request.size()) << std::endl;

    // Create output JSON tree
    boost::property_tree::ptree output_tree;
    // Inner tree, holding the message's content
    boost::property_tree::ptree content;

    // Property Tree Object
    boost::property_tree::ptree root;

    // Try to load the input stream as JSON
    try
    {
      boost::property_tree::read_json(ss, root);
    }
    // Return message if it failed
    catch (const boost::property_tree::json_parser::json_parser_error &e)
    {
      // Add the content
      content.put("status", false);
      content.put("message","Unable to parse string as a JSON.");
      // Append the content to the output tree
      output_tree.add_child("xvl_err", content);
    }

    // Extract key from the JSON tree
    std::string key = root.front().first;
    std::cout << "key: " << key << std::endl;

    // Sync message, reply with status
    if (boost::iequals(key, "xvl_syn"))
    {
      std::cout << "XVL Sync Message" << std::endl;

      // Add the content
      content.put("status", true);
      content.put_child("message",this->server_info.output());
      // Append the content to the output tree
      output_tree.add_child("xvl_ack", content);
    }
    // Query message, reply with the current allocation
    else if (boost::iequals(key , "xvl_que"))
    {
      std::cout << "XVL Query Message" << std::endl;

      // Add the content
      content.put("status", true);
      content.put_child("message", p_core->query_resources());
      // Append the content to the output tree
      output_tree.add_child("xvl_ans", content);
    }
    // Request message, try to allocate and reply the result
    else if (boost::iequals(key, "xvl_rrx") or
             boost::iequals(key, "xvl_rtx"))
    {
      std::cout << "XVL Request Message" << std::endl;
      std::cout << key << std::endl;

      // Extract the request arguments
      double d_cf = root.get(key + ".centre_freq", 0.0);
      double d_bw = root.get(key + ".bandwidth", 0.0);
      unsigned int u_id = root.get(key + ".id", 0.0);

      // If there's no CF, BW or ID
      if (not bool(d_cf) or not bool(d_bw) or not u_id)
      {
        // Add the content
        content.put("status", false);
        content.put("message","Missing or invalid parameters.");
        // Append the content to the output tree
        output_tree.add_child("xvl_rep", content);
      }
      // Otherwise, continue
      else
      {
        // If the reservation succeeds, it will hold the UDP port
        unsigned int u_reserved = 0;
        std::string remote_addr = root.get(key + ".ip", "0.0.0.0");

        // If it is a receive request
        if (boost::iequals(key, "xvl_rrx"))
        {
          // Try to reserve RX resources
          u_reserved = p_core->request_rx_resources(u_id, d_cf, d_bw, server_addr_no_port, remote_addr);
        }
        else // key == "xvl_rtx"
        {
          bool bpad = root.get(key + ".padding", false);
          // Try to reserve TX resources
          u_reserved = p_core->request_tx_resources(u_id, d_cf, d_bw, server_addr_no_port, remote_addr, bpad);
        }

        // If not able to reserve resources
        if (not u_reserved)
        {
          // Add the content
          content.put("status", false);
          content.put("message","Reservation failed.");
          // Append the content to the output tree
          output_tree.add_child("xvl_rep", content);
        }
        else
        {
          // Add the content
          content.put("status", true);
          content.put("message","Reservation succeeded.");
          content.put("udp_port", std::to_string(u_reserved));
          // Append the content to the output tree
          output_tree.add_child("xvl_rep", content);
        }
      } // End has arguments
    }  // End request handler

    // Free message, try to free resources and reply the result
    else if (boost::iequals(key, "xvl_fre"))
    {
      std::cout << "XVL Free Message" << std::endl;

      // Extract the request arguments
      unsigned int u_id = root.get("xvl_fre.id", 0);

      std::cout << "SERVER: u_id:" << u_id << std::endl;

      // Check if they are invalid
      if (not u_id)
      {
        std::cout << "SERVER: not u_id:" << u_id << std::endl;
        // Add the content
        content.put("status", false);
        content.put("message","Missing or invalid parameters.");
        // Append the content to the output tree
        output_tree.add_child("xvl_cls", content);
      }
      // Otherwise, continue
      else
      {
        // Try to reserve resources
        std::cout << "SERVER: calling p_core->free_resources" << std::endl;
        if ( not p_core->free_resources(u_id))
        {
          // Add the content
          content.put("status", false);
          content.put("message","Free resources failed.");
          // Append the content to the output tree
          output_tree.add_child("xvl_cls", content);
        }
        else
        {
          // Add the content
          content.put("status", true);
          content.put("message","Free resources succeeded.");
          // Append the content to the output tree
          output_tree.add_child("xvl_cls", content);
        }
      }
    }
    // Otherwise, unknown message
    else
    {
      std::cout << "Unknown Message. Rejecting" << std::endl;
      // Add the content
      content.put("status", false);
      content.put("message","Unknown message: " + key);
      // Append the content to the output tree
      output_tree.add_child("xvl_err", content);
    }

    // Create a string stream
    std::stringstream es;
    // Write the tree as a JSON string
    boost::property_tree::json_parser::write_json(es, output_tree);
    // Return it as a string
    std::string response_message = es.str();

    //  Send reply back to client
    zmq::message_t reply(response_message.size());
    memcpy (reply.data (), response_message.data(), response_message.size());
    socket.send (reply);

  }
  return 0;
}


} // namespace hydra
