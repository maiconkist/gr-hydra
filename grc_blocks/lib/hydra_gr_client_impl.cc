#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "hydra_gr_client_impl.h"

namespace gr {
  namespace hydra {

hydra_gr_client::sptr
hydra_gr_client::make(double center_frequency,
             double samp_rate,
             unsigned vr_id,
             const std::string &host,
             unsigned int port,
             unsigned int payload)
{
  return gnuradio::get_initial_sptr
    (new hydra_gr_client_impl(center_frequency,
                              samp_rate,
                              vr_id,
                              host,
                              port,
                              payload));
}

/*
 * The private constructor
 */
hydra_gr_client_impl::hydra_gr_client_impl(double d_center_frequency,
                         double d_samp_rate,
                         unsigned int u_id,
                         const std::string &s_host,
                         unsigned int u_port,
                         unsigned int u_payload)
  : gr::hier_block2("source",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
  client = new hydra_client(s_host, u_port, u_id, true);
  client->check_connection();

  int i_rx_port = client->request_rx_resources(d_center_frequency, d_samp_rate);

  if (i_rx_port)
  {
    d_udp_source = gr::blocks::udp_source::make(sizeof(gr_complex),
                                                s_host,
                                                i_rx_port,
                                                u_payload,
                                                true);

    connect(d_udp_source, 0, self(), 0);
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
hydra_gr_client_impl::~hydra_gr_client_impl()
{
  // client->free_resources();
}

void
hydra_gr_client_impl::start_reception(const std::string s_host, int i_port)
{
  d_udp_source->connect(s_host, i_port);
}

void
hydra_gr_client_impl::stop_reception(void)
{
  d_udp_source->disconnect();
}

} /* namespace hydra */
} /* namespace gr */
