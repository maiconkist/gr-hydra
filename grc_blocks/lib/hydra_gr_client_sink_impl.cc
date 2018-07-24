#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "hydra_gr_client_sink_impl.h"

namespace gr {
  namespace hydra {

hydra_gr_client_sink::sptr
hydra_gr_client_sink::make(double center_frequency,
             double samp_rate,
             unsigned vr_id,
             const std::string &host,
             unsigned int port,
             unsigned int payload)
{
  return gnuradio::get_initial_sptr
    (new hydra_gr_client_sink_impl(center_frequency,
                              samp_rate,
                              vr_id,
                              host,
                              port,
                              payload));
}

/*
 * The private constructor
 */
hydra_gr_client_sink_impl::hydra_gr_client_sink_impl(double d_center_frequency,
                         double d_samp_rate,
                         unsigned int u_id,
                         const std::string &s_host,
                         unsigned int u_port,
                         unsigned int u_payload)
  : gr::hier_block2("sink",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(0, 0, 0))
{
  client = new hydra_client(s_host, u_port, u_id, true);
  client->check_connection();

  int i_tx_port = client->request_tx_resources(d_center_frequency, d_samp_rate);

  if (i_tx_port)
  {
    d_udp_sink = gr::blocks::udp_sink::make(sizeof(gr_complex),
                                                s_host,
                                                i_tx_port,
                                                u_payload,
                                                true);

    connect(self(), 0, d_udp_sink, 0);
  }
  else
  {
    std::cerr << "Not able to reserve resources." << std::endl;
    exit(1);
  }
}

/*
 * Our virtual destructor.
 */
hydra_gr_client_sink_impl::~hydra_gr_client_sink_impl()
{
  // client->free_resources();
}

void
hydra_gr_client_sink_impl::start_reception(const std::string s_host, int i_port)
{
  d_udp_sink->connect(s_host, i_port);
}

void
hydra_gr_client_sink_impl::stop_reception(void)
{
  d_udp_sink->disconnect();
}

  } /* namespace hydra */
} /* namespace gr */
