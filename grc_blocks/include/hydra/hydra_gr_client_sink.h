#ifndef INCLUDED_HYDRA_CLIENT_SINK_H
#define INCLUDED_HYDRA_CLIENT_SINK_H

#include <hydra/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace hydra {

class HYDRA_API hydra_gr_client_sink : virtual public gr::hier_block2
{
  public:

    typedef boost::shared_ptr<hydra_gr_client_sink> sptr;

    static sptr make(unsigned int       u_id,
                     const std::string &s_host,
                     unsigned int       u_port);

    virtual void start_client(double d_center_frequency,
                              double d_samp_rate,
                              size_t u_payload) = 0;
};


} /* namespace hydra */
} /* namespace gr */

#endif /* INCLUDED_HYDRA_CLIENT_SINK_H */
