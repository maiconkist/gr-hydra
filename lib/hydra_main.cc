#include "hydra/hydra_main.h"

namespace hydra {

// Real radio centre frequency, bandwidth; control port; hypervisor's sampling rate, FFT size
HydraMain::HydraMain(std::string server_addr,
                     std::string group_name,
                     unsigned int u_monitor_port)
{
  /* Set the control port */
   s_server_addr = server_addr;

  // Set the group name
   s_group = group_name;

  // Initialise the stats reporter
  // monitor = std::make_shared<xvl_monitor>(u_monitor_port);
  // Initialise the core XVL
  core = std::make_shared<HydraCore>();

  logger = hydra_log("main");
}

void
HydraMain::set_rx_config(uhd_hydra_sptr usrp,
                         const double &d_cf,
                         const double &d_bw,
                         const double &d_ng,
                         const unsigned int &u_fft_size)
{
  core->set_rx_resources(usrp, d_cf, d_bw, d_ng, u_fft_size);
}

void HydraMain::set_tx_config(uhd_hydra_sptr usrp,
                              const double &d_cf,
                              const double &d_bw,
                              const double &d_ng,
                              const unsigned int &u_fft_size)
{
  // Configure transmitter resources
  core->set_tx_resources(usrp, d_cf, d_bw, d_ng, u_fft_size);
}

// Run server
void HydraMain::run()
{
   // Initialise the server
   server = std::make_shared<HydraServer>(s_server_addr, s_group, core);

   // Run the statistics reporting server
   // monitor->run();

   // Run the XVL server
   server->run();
}


void
HydraMain::stop()
{
  // Stop the HyDRA Core
  core->stop();

  // Stop the HyDRA Server
  server->stop();

  // Print
  // logger.info("Stopped all services");
}

} // namespace hydra
