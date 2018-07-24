#ifndef INCLUDED_HYDRA_SERVER_H
#define INCLUDED_HYDRA_SERVER_H

#include <hydra/api.h>

#include <hydra/hydra_main.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace hydra {
    /*!
     * \brief <+description of block+>
     * \ingroup hydra
     *
     */
    class HYDRA_API hydra_gr_server : virtual public gr::hier_block2
    {
      public:

        typedef boost::shared_ptr<hydra_gr_server> sptr;

        /*!
         * \brief Return a shared_ptr to a new instance of hydra::hydra_gr_server.
         *
         * To avoid accidental use of raw pointers, hydra::hydra_sink's
         * constructor is in a private implementation
         * class. hydra::hydra_sink::make is the public interface for
         * creating new instances.
         */
        static sptr make(double center_frequency,
                         double d_samp_rate,
                         unsigned int u_port,
                         size_t d_tx_fft_size);
    };
  } // namespace hydra
}   // namespace gr

#endif /* INCLUDED_HYDRA_SERVER_H */
