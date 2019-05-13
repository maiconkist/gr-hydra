#include "hydra/hydra_server.h"


namespace hydra {

HydraServer::HydraServer(std::string server_addr,
                         std::string group_name,
                         std::shared_ptr<HydraCore> core)
{
  // Get the server port
  s_server_addr = server_addr;
  // Get the group name
  s_group = group_name;

  // Pointer to the XVL Core
  p_core = core;

  thr_stop = false;

  logger = hydra_log("server");

  // Change the server status
  server_info.s_status = "Idle";
}

int
HydraServer::auto_discovery()
{
  const int MAX_MSG = 100;
  char msg[MAX_MSG];

  std::vector<std::string> client_info;

  int rx_port = 5001;
  int tx_port = 5002;

  hydra_log ad_logger("server|audodiscovery");

  ad_logger.info("Waiting for data on port UDP " + std::to_string(rx_port));

  // Check receive status
  int rcv = 0;
  // Event loop stop condition
  while (not thr_stop)
  {
    rcv = recv_udp(msg, MAX_MSG, true, rx_port, {1, 0}, &ad_logger);

    if (rcv != -1)
    {
      // Split the received message
      boost::split(client_info, msg, boost::is_any_of(":"));

      // If the message was not formatted properly
      if (client_info.size() != 2)
      {
        ad_logger.info("Received malformed message: " + std::string(msg));
      }
      // If we received a message from the same group
      else if (s_group == client_info[0])
      {
        // Return message
        send_udp(client_info[1], s_server_addr, false, tx_port, &ad_logger);
      }
      else
      {
        ad_logger.info("Received message from wrong group: " + std::string(client_info[0]));
      }

    } // End RECV checl
  } // End loop

  // Output debug information
  // ad_logger.info("Stopped autodiscovery");
}

// Run the server
int
HydraServer::run()
{
  //  Prepare our context and socket
  zmq::context_t context;
  zmq::socket_t socket (context, ZMQ_REP);
  socket.bind (("tcp://" + s_server_addr).c_str());

  // Timeout to get out of the while loop since recv is blocking
  int timeout = 1000;
  socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
  // int linger = 0; // Proper shutdown ZeroMQ
  // socket.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));

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

  // Received message flag
  int rc;

  // Event loop stop condition
  while (not thr_stop)
  {
    //  Wait for next request from client
    try
    {
      // Try receive messages from clinets
      rc = socket.recv(&request);
    }
    // Catch ZMQ errors
    catch (zmq::error_t& e)
    {
      // If it is a "I am ZMQ and I am a prick" error
      if (e.num() == EINTR)
      {
        // ZMQ throws this error when we want to exit
        break;
      }
      else
      {
        // Output real error mesage
        logger.error("ZMQ Error. " + std::string(e.what()));
      }
    }

    // If received a message within the timeout period
    if (rc && not thr_stop)
    {
      // Unpack string from message_t and push it to a string stream
      std::stringstream ss; ss << std::string(static_cast<char*>(request.data()),
                                           request.size());

      logger.debug("Request: " + std::string(static_cast<char*>(request.data()), request.size()));

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
      // logger.debug("key: " + key);

      // Sync message, reply with status
      if (boost::iequals(key, "xvl_syn"))
      {
        logger.debug("XVL Sync Message");

        // Add the content
        content.put("status", true);
        content.put_child("message",this->server_info.output());
        // Append the content to the output tree
        output_tree.add_child("xvl_ack", content);
      }
      // Query message, reply with the current allocation
      else if (boost::iequals(key , "xvl_que"))
      {
        logger.debug("XVL Query Message");

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
        logger.debug("XVL Request Message");

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
            // Try to reserve TX resources
            u_reserved = p_core->request_tx_resources(u_id, d_cf, d_bw, server_addr_no_port, remote_addr);
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
        logger.debug("XVL Free Message");

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
        logger.warning("Unknown Message. Rejecting");
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

      boost::erase_all(response_message, "\n");
      boost::erase_all(response_message, " ");

      logger.debug("Reply: " + response_message);

      //  Send reply back to client
      zmq::message_t reply(response_message.size());
      memcpy (reply.data (), response_message.data(), response_message.size());
      socket.send (reply);

    } // RC check
  } // While loop

  // Close the ZMQ primitives
  socket.close();
  // context.close();

  // Join the autodiscovery thread
  autod.join();

  // Output message when server stops
  logger.info("Stopped XVL Server");

  return 0;
}


} // namespace hydra
