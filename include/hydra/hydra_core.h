#ifndef HYDRA_CORE_INCLUDE_H
#define HYDRA_CORE_INCLUDE_H

#include "hydra/types.h"
#include "hydra/hydra_resource.h"
#include "hydra/hydra_hypervisor.h"
#include "hydra/hydra_uhd_interface.h"

#include <set>
#include <iostream>
#include <list>

using namespace std;


namespace hydra {

class HydraCore
{
private:
   // Pointer to the resource manager object
   std::unique_ptr<xvl_resource_manager> p_resource_manager;
   std::unique_ptr<Hypervisor> p_hypervisor;

   // Save the receiver info
   bool b_receiver;

   // Save the transmitter info
   bool b_transmitter;

   // Set of occupied UDP ports
   std::set<unsigned int> used_ports;

   std::mutex g_mutex;

public:
   // Constructor
   HydraCore();

   // Destructor
   ~HydraCore(){};

   // Set RX resources
   void set_rx_resources(uhd_hydra_sptr usrp,
                         double d_centre_freq,
                         double d_bandwidth,
                         unsigned int u_fft_size);

   // Set TX resources
   void set_tx_resources(uhd_hydra_sptr usrp,
                         double d_centre_freq,
                         double d_bandwidth,
                         unsigned int u_fft_size);

   // Request RX resources
   int request_rx_resources(unsigned int u_id,
                            double d_centre_freq,
                            double d_bandwidth,
                            const std::string &remote_addr);

   // Request TX resources
   int request_tx_resources(unsigned int u_id,
                            double d_centre_freq,
                            double d_bandwidth,
                            bool bpad);

   // Query resources
   boost::property_tree::ptree query_resources();

   // Free resource
   int free_resources(size_t u_id);

};

}; // namespace hydar

#endif
