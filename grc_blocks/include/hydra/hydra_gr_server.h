#ifndef INCLUDED_HYDRA_SERVER_H
#define INCLUDED_HYDRA_SERVER_H

#include <hydra/api.h>

#include <hydra/hydra_main.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace hydra {

    class HYDRA_API hydra_gr_server : virtual public gr::hier_block2
    {
      public:
        typedef boost::shared_ptr<hydra_gr_server> sptr;

        static sptr make(unsigned int u_port);

        virtual void set_tx_config(double d_center_frequency,
                           double d_samp_rate,
                           size_t d_tx_fft_size) = 0;

        virtual void start_server() = 0;
    };
  } // namespace hydra
}   // namespace gr

#endif /* INCLUDED_HYDRA_SERVER_H */
