#ifndef INCLUDED_HYDRA_CLIENT_H
#define INCLUDED_HYDRA_CLIENT_H

#include <hydra/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace hydra {
    /*!
     * \brief <+description of block+>
     * \ingroup hydra
     *
     */
    class HYDRA_API hydra_gr_client : virtual public gr::hier_block2
    {
      public:

        typedef boost::shared_ptr<hydra_gr_client> sptr;

        /*!
         * \brief Return a shared_ptr to a new instance of hydra::hydra_sink.
         *
         * To avoid accidental use of raw pointers, hydra::hydra_sink's
         * constructor is in a private implementation
         * class. hydra::hydra_sink::make is the public interface for
         * creating new instances.
         */
        static sptr make(double             center_frequency,
                         double             bandwidth,
                         unsigned int       vr_id,
                         const std::string &host,
                         unsigned int       port,
                         unsigned int       payload);
    };
  } // namespace hydra
}   // namespace gr

#endif /* INCLUDED_HYDRA_SINK_H */
