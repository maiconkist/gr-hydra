#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hydra_gr_server_network_impl.h"

#include <gnuradio/io_signature.h>
#include <hydra/types.h>
#include <hydra/hydra_uhd_interface.h>

namespace gr {
  namespace hydra {

hydra_gr_server_network::sptr
hydra_gr_server_network::make(std::string host_addr)
{
  return gnuradio::get_initial_sptr(new hydra_gr_server_network_impl(host_addr));
}

hydra_gr_server_network_impl::hydra_gr_server_network_impl(std::string host_addr)
  :gr::hier_block2("server",
                   gr::io_signature::make(0, 0, 0),
                   gr::io_signature::make(0, 0, 0))
{
  std::cout << "CTOR server " << std::endl;
  main = new HydraMain(host_addr);
}

hydra_gr_server_network_impl::~hydra_gr_server_network_impl()
{
}

void
hydra_gr_server_network_impl::init_usrp(std::string host_addr, std::string remote_addr)
{
  if (g_usrp.get() == nullptr)
  {
       g_usrp = std::make_shared<device_network>(host_addr, remote_addr);
  }
}


void
hydra_gr_server_network_impl::set_tx_config(double d_center_frequency,
                                    double d_samp_rate,
                                    size_t d_fft_size,
                                    std::string host_addr,
                                    std::string remote_addr)
{
  init_usrp(host_addr, remote_addr);

  main->set_tx_config(g_usrp,
                      d_center_frequency,
                      d_samp_rate,
                      d_fft_size);
}


void
hydra_gr_server_network_impl::set_rx_config(double d_center_frequency,
                                    double d_samp_rate,
                                    size_t d_fft_size,
                                    std::string host_addr,
                                    std::string remote_addr)
{
  init_usrp(host_addr, remote_addr);

  main->set_rx_config(g_usrp,
                      d_center_frequency,
                      d_samp_rate,
                      d_fft_size);
}

void
hydra_gr_server_network_impl::start_server()
{
  std::cout << "Running server " << std::endl;
  main->run();
}

} /* namespace hydra */
} /* namespace gr */
