#include "hydra/hydra_main.h"

namespace hydra {

// Real radio centre frequency, bandwidth; control port; hypervisor's sampling rate, FFT size
HydraMain::HydraMain(std::string server_addr,
                     unsigned int u_monitor_port)
{
   /* Set the control port */
   s_server_addr = server_addr;

   // Initialise the stats reporter
   monitor = std::make_shared<xvl_monitor>(u_monitor_port);
   // Initialise the core XVL
   core = std::make_shared<HydraCore>();
}

void
HydraMain::set_rx_config(uhd_hydra_sptr usrp,
                         double d_cf,
                         double d_bw,
                         unsigned int u_fft_size)
{
  // Configure receiver resources
  usrp->set_rx_config(d_cf, d_bw, 0);
  core->set_rx_resources(usrp, d_cf, d_bw, u_fft_size);
}

void HydraMain::set_tx_config(uhd_hydra_sptr usrp,
                              double d_cf,
                              double d_bw,
                              unsigned int u_fft_size)

{
  // Configure transmitter resources
  usrp->set_tx_config(d_cf, d_bw, 0.6);
  core->set_tx_resources(usrp, d_cf, d_bw, u_fft_size);
}

// Run server
void HydraMain::run()
{
   // Initialise the server
   server = std::make_shared<HydraServer>(s_server_addr, core);

   // Run the statistics reporting server
   monitor->run();

   // Run the XVL server
   server->run();
}


} // namespace hydra
