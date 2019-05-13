#ifndef HYDRA_MAIN_INCLUDE_H
#define HYDRA_MAIN_INCLUDE_H

#include <memory>

#include "hydra/hydra_server.h"
#include "hydra/hydra_core.h"
#include "hydra/hydra_log.h"
#include "hydra/hydra_stats.h"
#include "hydra/types.h"

namespace hydra {

class HydraMain
{
public:
   /* CTOR
    */
   HydraMain();

   HydraMain(
       std::string server_addr,
       std::string group_name,
       unsigned int u_monitor_port = 4996);

   void set_rx_config(uhd_hydra_sptr usrp,
                      const double &d_cf,
                      const double &d_bw,
                      const double &d_ng,
                      const unsigned int &u_fft_size);

   void set_tx_config(uhd_hydra_sptr usrp,
                      const double &d_cf,
                      const double &d_bw,
                      const double &d_ng,
                      const unsigned int &u_fft_size);

   // Run method
   void run();

   void stop();

private:
   std::string s_server_addr;
   std::string s_group;

   // Pointer to the XVL Server
   std::shared_ptr<HydraServer> server;
   /// Pointer to the XVL Core
   std::shared_ptr<HydraCore> core;
   // Pointer to the XVL Statistics monitor
   std::shared_ptr<xvl_monitor> monitor;

  hydra_log logger;

};

}; // namespace hydra

#endif
