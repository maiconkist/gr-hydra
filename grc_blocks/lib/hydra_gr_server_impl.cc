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
hydra_gr_server::make(unsigned int port)
{
  return gnuradio::get_initial_sptr(new hydra_gr_server_impl(port));
}

/*
 * The private constructor
 */
hydra_gr_server_impl::hydra_gr_server_impl(unsigned int u_port)
  :gr::hier_block2("server",
                   gr::io_signature::make(0, 0, 0),
                   gr::io_signature::make(0, 0, 0))
{
  main = new HydraMain(u_port);
}


hydra_gr_server_impl::~hydra_gr_server_impl()
{
}

void
hydra_gr_server_impl::set_tx_config(double d_center_frequency,
                                    double d_samp_rate,
                                    size_t d_tx_fft_size)
{
  uhd_hydra_sptr usrp = std::make_shared<device_uhd>(d_center_frequency, d_samp_rate, 0.1);

  main->set_tx_config(usrp,
                      d_center_frequency,
                      d_samp_rate,
                      d_tx_fft_size);
}

void
hydra_gr_server_impl::start_server()
{
  main->run();
}

} /* namespace hydra */
} /* namespace gr */
