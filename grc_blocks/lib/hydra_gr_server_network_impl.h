#ifndef INCLUDED_HYDRA_GR_SERVER_NETWORK_IMPL_H
#define INCLUDED_HYDRA_GR_SERVER_NETWORK_IMPL_H

#include "hydra/hydra_gr_server_network.h"

#include <string>
#include <hydra/types.h>
#include <hydra/hydra_main.h>
#include <gnuradio/blocks/udp_source.h>

using namespace hydra;

namespace gr {
  namespace hydra {

class hydra_gr_server_network_impl : public hydra_gr_server_network
{
 public:
  /** CTOR
   */
   hydra_gr_server_network_impl(std::string server_addr);

  /** DTOR
   */
  ~hydra_gr_server_network_impl();


  virtual void set_tx_config(double d_center_frequency,
                             double d_samp_rate,
                             size_t d_tx_fft_size,
                             std::string host_addr,
                             std::string remote_addr);

  virtual void set_rx_config(double d_center_frequency,
                             double d_samp_rate,
                             size_t d_tx_fft_size,
                             std::string host_addr,
                             std::string remote_addr);

  virtual void start_server();

 private:
  HydraMain *main;
  uhd_hydra_sptr g_usrp;

  void init_usrp(std::string host_addr, std::string remote_addr);
};


  } // namespace hydra
} // namespace gr

#endif /* INCLUDED_HYDRA_SOURCE_IMPL_NETWORK_H */
