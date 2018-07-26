#ifndef INCLUDED_HYDRA_GR_CLIENT_SINK_IMPL_H
#define INCLUDED_HYDRA_GR_CLIENT_SINK_IMPL_H

#include "hydra/hydra_gr_client_sink.h"

#include <hydra/hydra_client.h>
#include <gnuradio/blocks/udp_sink.h>

using namespace hydra;

namespace gr {
  namespace hydra {

class hydra_gr_client_sink_impl : public hydra_gr_client_sink
{
public:
  /* CTOR
   */
  hydra_gr_client_sink_impl(unsigned int u_id,
                            const std::string &host,
                            unsigned int u_port);

  virtual void start_client(double d_center_frequency,
                            double d_samp_rate,
                            size_t u_payload);

  ~hydra_gr_client_sink_impl();

private:
  gr::blocks::udp_sink::sptr d_udp_sink;
  hydra_client *client;
  std::string g_host;
};

  } // namespace hydra
} // namespace gr

#endif /* INCLUDED_HYDRA_CLIENT_SINK_IMPL_H */
