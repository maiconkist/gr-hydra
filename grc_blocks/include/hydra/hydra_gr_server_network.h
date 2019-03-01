#ifndef INCLUDED_HYDRA_SERVER_NETWORK_H
#define INCLUDED_HYDRA_SERVER_NETWORK_H

#include <hydra/api.h>

#include <hydra/hydra_main.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace hydra {

    class HYDRA_API hydra_gr_server_network : virtual public gr::hier_block2
    {
      public:
        typedef boost::shared_ptr<hydra_gr_server_network> sptr;

        static sptr make(std::string server_addr);

        virtual void set_tx_config(double d_center_frequency,
                                   double d_samp_rate,
                                   size_t d_tx_fft_size,
                                   std::string host_addr,
                                   std::string remote_addr) = 0;

        virtual void set_rx_config(double d_center_frequency,
                                   double d_samp_rate,
                                   size_t d_tx_fft_size,
                                   std::string host_addr,
                                   std::string remote_addr) = 0;

        virtual void start_server() = 0;
    };
  } // namespace hydra
}   // namespace gr

#endif /* INCLUDED_HYDRA_SERVER_H */
