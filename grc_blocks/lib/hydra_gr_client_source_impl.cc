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

  if (client->check_connection(3) == std::string(""))
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
  rx_configuration rx_config{d_center_frequency, d_samp_rate, false};
  int err = client->request_rx_resources(rx_config);

  if (!err)
  {
#if 1
    std::string addr = "tcp://" + rx_config.server_ip + ":" + std::to_string(rx_config.server_port);
    std::cout << "addr: " << addr << std::endl;
    gr::zeromq::pull_source::sptr d_source = gr::zeromq::pull_source::make(sizeof(gr_complex),
                                             1,
                                             const_cast<char *>(addr.c_str()));
#endif
    connect(d_source, 0, self(), 0);
  }
  else
  {
    std::cerr << "Not able to reserve resources." << std::endl;
    exit(1);
  }
}

void hydra_gr_client_source_impl::test()
{
  std::string addr = "tcp://192.168.5.77:33000";
  zmq::context_t context;
  zmq::message_t message;

  std::cout << "addr: " << addr << std::endl;

  zmq::socket_t socket(context, ZMQ_PULL);
  socket.connect(addr.c_str());

  while (1)
  {
    socket.recv(&message);
    std::cout << "received: " << message.size() << std::endl;
    message.rebuild();
  }
}

  } /* namespace hydra */
} /* namespace gr */
