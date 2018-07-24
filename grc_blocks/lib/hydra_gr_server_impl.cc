#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hydra_gr_server_impl.h"

#include <gnuradio/io_signature.h>
#include <hydra/hydra_uhd_interface.h>

namespace gr {
  namespace hydra {

hydra_gr_server::sptr
hydra_gr_server::make(double center_frequency,
             double samp_rate,
             unsigned int port,
             size_t fft_size)
{
  return gnuradio::get_initial_sptr
    (new hydra_gr_server_impl(center_frequency,
                              samp_rate,
                              port,
                              fft_size));
}

/*
 * The private constructor
 */
hydra_gr_server_impl::hydra_gr_server_impl(double d_center_frequency,
                                           double d_samp_rate,
                                           unsigned int u_port,
                                           size_t d_tx_fft_size)
  :gr::hier_block2("server",
                   gr::io_signature::make(0, 0, 0),
                   gr::io_signature::make(0, 0, 0))
{
  main = new HydraMain(u_port);

  uhd_hydra_sptr usrp = std::make_shared<device_image_gen>(d_center_frequency, d_samp_rate, 0.1);

  main->set_tx_config(usrp,
                        d_center_frequency,
                        d_samp_rate,
                        d_tx_fft_size);

  main->run();
}

/*
 * Our virtual destructor.
 */
hydra_gr_server_impl::~hydra_gr_server_impl()
{
  // server->free_resources();
}

} /* namespace hydra */
} /* namespace gr */
