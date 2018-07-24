#ifndef INCLUDED_HYDRA_GR_CLIENT_IMPL_H
#define INCLUDED_HYDRA_GR_CLIENT_IMPL_H

#include "hydra/hydra_gr_client.h"

#include <hydra/hydra_client.h>
#include <gnuradio/blocks/udp_source.h>

using namespace hydra;

namespace gr {
  namespace hydra {
    class hydra_gr_client_impl : public hydra_gr_client
    {
     private:

      gr::blocks::udp_source::sptr d_udp_source;
      hydra_client *client;

     public:
      hydra_gr_client_impl(double d_center_frequency,
                           double d_samp_rate,
                           unsigned int u_id,
                           const std::string &host,
                           unsigned int u_port,
                           unsigned int u_payload);
      ~hydra_gr_client_impl();

      // Where all the action really happens
      // Where all the action really happens
      void start_reception(const std::string s_host,
                           int i_port);

      void stop_reception(void);
    };

  } // namespace hydra
} // namespace gr

#endif /* INCLUDED_HYDRA_SOURCE_IMPL_H */
