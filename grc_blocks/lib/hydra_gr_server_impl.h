#ifndef INCLUDED_HYDRA_GR_SERVER_IMPL_H
#define INCLUDED_HYDRA_GR_SERVER_IMPL_H

#include "hydra/hydra_gr_server.h"

#include <hydra/hydra_main.h>
#include <gnuradio/blocks/udp_source.h>

using namespace hydra;

namespace gr {
  namespace hydra {

class hydra_gr_server_impl : public hydra_gr_server
{
 private:

  HydraMain *main;

 public:
  hydra_gr_server_impl(double d_center_frequency,
                       double d_samp_rate,
                       unsigned int u_port,
                       size_t d_tx_fft_size);

  ~hydra_gr_server_impl();
};
  } // namespace hydra
} // namespace gr

#endif /* INCLUDED_HYDRA_SOURCE_IMPL_H */
