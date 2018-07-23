#include "hydra/hydra_server.h"

#include <boost/algorithm/string.hpp>


namespace hydra {

HydraServer::HydraServer(unsigned int u_port,
                       std::shared_ptr<HydraCore> core)
{
  // Get the server port
  s_server_port = std::to_string(u_port);

  // Pointer to the XVL Core
  p_core = core;

  // Change the server status
  server_info.s_status = "Idle";
}

// Run the server
int
HydraServer::run()
{  //  Prepare our context and socket
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);
  socket.bind (("tcp://0.0.0.0:" + s_server_port).c_str());

  // Change the server status
  server_info.s_status = "Enabled";

  // Message type object
  zmq::message_t request;

  // Even loop
  while (true)
  {
    //  Wait for next request from client
    socket.recv (&request);

    // Unpack string from message_t and push it to a string stream
    std::stringstream ss; ss << std::string(static_cast<char*>(request.data()),
                                         request.size());

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

        // If it is a receive request
        if (boost::iequals(key, "xvl_rrx"))
        {
          // Try to reserve RX resources
          u_reserved = p_core->request_rx_resources(u_id, d_cf, d_bw);
        }
        else
        {
          // Try to reserve TX resources
          u_reserved = p_core->request_tx_resources(u_id, d_cf, d_bw, true);
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

      // Check if they are invalid
      if (not u_id)
      {
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
