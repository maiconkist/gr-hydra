#ifndef HYDRA_MAIN_INCLUDE_H
#define HYDRA_MAIN_INCLUDE_H

#include <memory>

#include "hydra/hydra_server.h"
#include "hydra/hydra_core.h"
#include "hydra/hydra_stats.h"
#include "hydra/types.h"


namespace hydra {

class HydraMain
{
public:
   // Pointer to the XVL Server
   std::unique_ptr<HydraServer> server;
   /// Pointer to the XVL Core
   std::shared_ptr<HydraCore> core;
   // Pointer to the XVL Statistics monitor
   std::shared_ptr<xvl_monitor> monitor;

   unsigned int u_port;

   // CTOR
   HydraMain(unsigned int u_control_port, unsigned int u_monitor_port = 4996);

   void set_rx_config(double d_cf, double d_bw, unsigned int u_fft_size);
   void set_tx_config(uhd_hydra_sptr usrp, double d_cf, double d_bw, unsigned int u_fft_size);

   // Run method
   void run();
};

}; // namespace hydra

#endif
