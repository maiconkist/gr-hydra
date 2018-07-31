#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "hydra_gr_client_sink_impl.h"

namespace gr {
  namespace hydra {

hydra_gr_client_sink::sptr
hydra_gr_client_sink::make(unsigned u_id,
                           const std::string &host,
                           unsigned int port)
{
  return gnuradio::get_initial_sptr(
      new hydra_gr_client_sink_impl(u_id, host, port)
  );
}

/*
 * The private constructor
 */
hydra_gr_client_sink_impl::hydra_gr_client_sink_impl(
                   unsigned int u_id,
                   const std::string &s_host,
                   unsigned int u_port)
  :gr::hier_block2("gr_client_sink",
                   gr::io_signature::make(1, 1, sizeof(gr_complex)),
                   gr::io_signature::make(0, 0, 0))

{
  g_host = s_host;
  client = new hydra_client(s_host, u_port, u_id, true);
  client->check_connection();
}

void
hydra_gr_client_sink_impl::start_client(double d_center_frequency,
                                        double d_samp_rate,
                                        size_t u_payload)
{
  int i_tx_port = client->request_tx_resources(d_center_frequency, d_samp_rate, false);

  if (i_tx_port)
  {
    d_udp_sink = gr::blocks::udp_sink::make(sizeof(gr_complex),
                                                g_host,
                                                i_tx_port,
                                                u_payload,
                                                true);

    d_udp_sink->connect(g_host, i_tx_port);

    connect(self(), 0, d_udp_sink, 0);
    std::cout << "Client initialized successfully." << std::endl;
  }
  else
  {
    std::cerr << "Not able to reserve resources." << std::endl;
    exit(1);
  }
}

hydra_gr_client_sink_impl::~hydra_gr_client_sink_impl()
{
  // client->free_resources();
}

  } /* namespace hydra */
} /* namespace gr */
