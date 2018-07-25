#include "hydra/hydra_main.h"


namespace hydra {

// Real radio centre frequency, bandwidth; control port; hypervisor's sampling rate, FFT size
HydraMain::HydraMain(unsigned int u_control_port,
                     unsigned int u_monitor_port)
{
   // Set the control port
   u_port = u_control_port;

   // Initialise the stats reporter
   monitor = std::make_shared<xvl_monitor>(u_monitor_port);
   // Initialise the core XVL
   core = std::make_shared<HydraCore>();
}

void
HydraMain::set_rx_config(double d_cf,
                         double d_bw,
                         unsigned int u_fft_size)
{
   // Configure receiver resources
   core->set_rx_resources(d_cf, d_bw, u_fft_size);
}

void HydraMain::set_tx_config(uhd_hydra_sptr usrp,
                              double d_cf,
                              double d_bw,
                              unsigned int u_fft_size)
{
   // Configure transmitter resources
   core->set_tx_resources(usrp, d_cf, d_bw, u_fft_size);
}

// Run server
void HydraMain::run()
{
   // Initialise the server
   server = std::make_shared<HydraServer>(u_port, core);

   // Run the statistics reporting server
   monitor->run();

   // Run the XVL server
   server->run();
}


} // namespace hydra
