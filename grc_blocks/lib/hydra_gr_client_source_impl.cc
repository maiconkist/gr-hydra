#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hydra_gr_client_source_impl.h"

#include <gnuradio/io_signature.h>
#include <gnuradio/zeromq/pull_source.h>

namespace gr {
  namespace hydra {

hydra_gr_client_source::sptr
hydra_gr_client_source::make(unsigned           u_id,
                             const std::string &s_host,
                             unsigned int       port,
                             const std::string &s_group)
{
  return gnuradio::get_initial_sptr(new hydra_gr_client_source_impl(u_id, s_host, port, s_group));
}

/* CTOR
 */
hydra_gr_client_source_impl::hydra_gr_client_source_impl(unsigned int u_id,
                                                         const std::string &s_host,
                                                         unsigned int u_port,
                                                         const std::string &s_group):
  gr::hier_block2("gr_client_source",
                  gr::io_signature::make(0, 0, 0),
                  gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
  client = std::make_unique<hydra_client>(s_host, u_port, u_id, s_group, true);

  if (client->check_connection() == std::string(""))
  {
    std::cout << "Could not connect to server. Aborting" << std::endl;
    assert(1 == 0);
  }
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
  rx_configuration rx_config{d_center_frequency, d_samp_rate};
  int err = client->request_rx_resources(rx_config);

  if (!err)
  {
    std::string addr = "tcp://" + rx_config.server_ip + ":" + std::to_string(rx_config.server_port);
    gr::zeromq::pull_source::sptr d_source = gr::zeromq::pull_source::make(sizeof(iq_sample),
                                             1,
                                             const_cast<char *>(addr.c_str()));
    connect(d_source, 0, self(), 0);
  }
  else
  {
    std::cerr << "Not able to reserve resources." << std::endl;
    exit(1);
  }
}

  } /* namespace hydra */
} /* namespace gr */
