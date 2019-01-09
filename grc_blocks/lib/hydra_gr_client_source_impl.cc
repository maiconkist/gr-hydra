#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "hydra_gr_client_source_impl.h"

namespace gr {
  namespace hydra {

hydra_gr_client_source::sptr
hydra_gr_client_source::make(unsigned           u_id,
                             const std::string &c_host,
                             const std::string &s_host,
                             unsigned int       port)
{
  return gnuradio::get_initial_sptr(new hydra_gr_client_source_impl(u_id, c_host, s_host, port));
}

/* CTOR
 */
hydra_gr_client_source_impl::hydra_gr_client_source_impl(unsigned int u_id,
                                                         const std::string &c_host,
                                                         const std::string &s_host,
                                                         unsigned int u_port):
  gr::hier_block2("gr_client_source",
                  gr::io_signature::make(0, 0, 0),
                  gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
  client = std::make_unique<hydra_client>(c_host, u_port, u_id, true);
  client->check_connection();
}

hydra_gr_client_source_impl::~hydra_gr_client_source_impl()
{
  client->free_resources();
}

bool
hydra_gr_client_source_impl::stop()
{
  client->free_resources();
}

void hydra_gr_client_source_impl::start_client(double d_center_frequency,
                                               double d_samp_rate,
                                               size_t u_payload)
{

  rx_configuration rx_config{d_center_frequency, d_samp_rate, false};
  int err = client->request_rx_resources(rx_config);

  if (!err)
  {
    std::cout << boost::format("Creating GNURadio UDP source block: (%1%: %2%)") % "0.0.0.0" % rx_config.server_port << std::endl;
    d_udp_source = gr::blocks::udp_source::make(sizeof(gr_complex),
                                                "0.0.0.0",
                                                rx_config.server_port,
                                                u_payload,
                                                false);

    connect(d_udp_source, 0, self(), 0);
  }
  else
  {
    std::cerr << "Not able to reserve resources." << std::endl;
    exit(1);
  }
}

} /* namespace hydra */
} /* namespace gr */
