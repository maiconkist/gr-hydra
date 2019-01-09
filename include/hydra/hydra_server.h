#ifndef HYDRA_SERVER_INCLUDE_H
#define HYDRA_SERVER_INCLUDE_H

#include <string>
#include <iostream>
#include <unistd.h>
#include <zmq.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "hydra/hydra_core.h"

namespace hydra {

//TODO Perhaps I should use Protobuffers
struct xvl_info
{
   // Default values
   std::string s_status = "Disabled";
   std::string s_name = "XVL Hypervisor Server";
   std::string s_version = "0.1";

   // Output the struct content as a string
   boost::property_tree::ptree output()
   {
      // Create a tree object
      boost::property_tree::ptree tree;
      // Insert the class parameters
      tree.put("condition", this->s_status);
      tree.put("name", this->s_name);
      tree.put("version", this->s_version);
      // Return the tree object
      return tree;
   }
};

class HydraServer
{
public:
   /* CTOR */
   HydraServer(std::string server_addr, std::shared_ptr<HydraCore> core);

   /* Run the server */
   int run();

   /* Run auto discovery service */
   int auto_discovery();

private:
   // Struct with the server info
   xvl_info server_info;
   // Server ip:port
   std::string s_server_addr;
   // Pointer to ther XVL Core
   std::shared_ptr<HydraCore> p_core;

};


}; /* namespace hydra */

#endif
