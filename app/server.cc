#include "hydra/hydra_main.h"
#include "hydra/hydra_uhd_interface.h"

int main()
{
   /* TRANSMITTER */
   double d_tx_centre_freq = 2.0e9;
   double d_tx_samp_rate   = 2e6;
   unsigned int u_tx_fft_size = 1024;

   /* RECEIVER */
   double d_rx_centre_freq = 2.0e9 + 200e3;
   double d_rx_samp_rate   = 200e3;
   unsigned int u_rx_fft_size = 64;

   /* Control port */
   unsigned int u_port = 5000;

   /* Instantiate XVL */
   hydra::HydraMain main = hydra::HydraMain(u_port);

   /* Configure the TX radio */
   hydra::uhd_hydra_sptr usrp = std::make_shared<hydra::device_image_gen>();

#if 0
   main.set_tx_config(
      usrp,
      d_tx_centre_freq,
      d_tx_samp_rate,
      u_tx_fft_size);
#endif

   main.set_rx_config(
     usrp,
     d_rx_centre_freq,
     d_rx_samp_rate,
     u_rx_fft_size);

   /* Run server */
   main.run();

   /* Run server */
   return 0;
}
