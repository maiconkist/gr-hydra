#ifndef HYDRA_CLIENT_INCLUDE_H
#define HYDRA_CLIENT_INCLUDE_H


#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/property_tree/json_parser.hpp>

namespace hydra {

class hydra_client
{
  // Server host and port
  std::string s_server_host;
  std::string s_server_port;

  // Client ID -- TODO need a better way to define it
  int u_id;
  // Debug flag
  bool b_debug_flag;

public:
    // Constructor
  hydra_client(std::string s_host = "localhost",
             const unsigned int u_port = 5000,
             const unsigned int u_client_id = 10,
             const bool b_debug = false);

  // Request RX resources
  int request_rx_resources(const double d_centre_freq = 0,
                           const double d_bandwidth = 0);

  // Request TX resources
  int request_tx_resources(const double d_centre_freq = 0,
                           const double d_bandwidth = 0);

  // Check whether the hypervisor is alive
  std::string check_connection();

  // Query the available resources
  std::string query_resources();

  // Free resources
  std::string free_resources();

private:
  // Base message methods
  std::string factory(const std::string &s_message);
};


}; // namespace hydra

#endif
