#ifndef HYDRA_CLIENT_INCLUDE_H
#define HYDRA_CLIENT_INCLUDE_H


#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/property_tree/json_parser.hpp>

namespace hydra
{

class hydra_client
{
public:

   /* CTOR
    */
   hydra_client(std::string client_ip = "localhost",
                std::string server_ip = "localhost",
                unsigned int u_port = 5000,
                unsigned int u_client_id = 10,
                bool b_debug = false);

   /* DTOR
    */
   ~hydra_client();

   /* Request RX resources */
   int request_rx_resources(double d_centre_freq,
                            double d_bandwidth,
                            bool bpad = false);

   /* Request TX resources */
   int request_tx_resources(double d_centre_freq,
                            double d_bandwidth,
                            bool bpad = false);


   /* Check whether the hypervisor is alive */
   std::string check_connection();

   /* Query the available resources */
   std::string query_resources();

   /* Free resources */
   std::string free_resources();

private:
   /* Base message methods */
   std::string factory(const std::string &s_message);
   int discover_server(std::string client, std::string &server_ip);


   std::string s_client_host;
   std::string s_server_host;
   std::string s_server_port;

   /* Client ID -- TODO need a better way to define it */
   int u_id;

   /* Debug flag */
   bool b_debug_flag;
};


}; /* namespace hydra */

#endif
