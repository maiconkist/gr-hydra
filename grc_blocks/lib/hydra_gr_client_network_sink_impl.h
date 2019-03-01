#ifndef INCLUDED_HYDRA_GR_CLIENT_NETWORK_SINK_NETWORKIMPL_H
#define INCLUDED_HYDRA_GR_CLIENT_NETWORK_SINK_IMPL_H

#include "hydra/hydra_gr_client_network_sink.h"

#include "hydra/hydra_client.h"
#include <gnuradio/blocks/tcp_server_sink.h>
#include <chrono>

using namespace hydra;

namespace gr {
  namespace hydra {

class hydra_gr_client_network_sink_impl : public hydra_gr_client_network_sink
{
 public:
   /* CTOR
    */
   hydra_gr_client_network_sink_impl(unsigned int u_id,
                             const std::string &host_addr,
                             unsigned int u_port,
                             const std::string &server_addr);

   virtual void start_client(double d_center_frequency,
                             double d_samp_rate,
                             size_t u_payload);

   virtual int request_tx_resources(double d_center_frequency,
                                    double d_samp_rate,
                                    size_t u_payload);

   ~hydra_gr_client_network_sink_impl();

   virtual bool stop();

 private:
  gr::blocks::tcp_server_sink::sptr d_tcp_sink;
  std::unique_ptr<hydra_client> client;
  std::string g_host;
};

  } /* namespace hydra */
} /* namespace gr */

#endif /* INCLUDED_HYDRA_GR_CLIENT_NETWORK_SINK_IMPL_H */
