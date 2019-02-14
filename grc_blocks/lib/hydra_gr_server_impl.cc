#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hydra_gr_server_impl.h"

#include <gnuradio/io_signature.h>
#include <hydra/types.h>
#include <hydra/hydra_uhd_interface.h>

namespace gr {
  namespace hydra {

hydra_gr_server::sptr
hydra_gr_server::make(std::string server_addr)
{
  return gnuradio::get_initial_sptr(new hydra_gr_server_impl(server_addr));
}

hydra_gr_server_impl::hydra_gr_server_impl(std::string server_addr)
  :gr::hier_block2("server",
                   gr::io_signature::make(0, 0, 0),
                   gr::io_signature::make(0, 0, 0))
{
  std::cout << "CTOR server " << std::endl;
  main = new HydraMain(server_addr);
}

hydra_gr_server_impl::~hydra_gr_server_impl()
{
}

void
hydra_gr_server_impl::init_usrp(std::string mode)
{
  if (g_usrp.get() == nullptr)
  {
    if (mode == "USRP")
      g_usrp = std::make_shared<device_uhd>();
    else if (mode == "IMG_GEN")
      g_usrp = std::make_shared<device_image_gen>();
    else if (mode == "LOOPBACK")
       g_usrp = std::make_shared<device_loopback>();
    else
      std::cout << "Unknown device type: " << __PRETTY_FUNCTION__ << std::endl;
  }
}


void
hydra_gr_server_impl::set_tx_config(double d_center_frequency,
                                    double d_samp_rate,
                                    size_t d_fft_size,
                                    std::string mode)
{
  init_usrp(mode);

  main->set_tx_config(g_usrp,
                      d_center_frequency,
                      d_samp_rate,
                      d_fft_size);
}


void
hydra_gr_server_impl::set_rx_config(double d_center_frequency,
                                    double d_samp_rate,
                                    size_t d_fft_size,
                                    std::string mode)
{
  init_usrp(mode);

  main->set_rx_config(g_usrp,
                      d_center_frequency,
                      d_samp_rate,
                      d_fft_size);
}

void
hydra_gr_server_impl::start_server()
{
  std::cout << "Running server " << std::endl;
  main->run();
}

} /* namespace hydra */
} /* namespace gr */
