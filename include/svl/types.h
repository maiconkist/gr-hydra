#ifndef INCLUDED_SVL_TYPES_H
#define INCLUDED_SVL_TYPES_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include <queue>

#include <gnuradio/types.h>

namespace gr {
		  namespace svl {

      typedef std::vector<gr_complex> samples_vec;
      typedef std::queue<samples_vec> samples_vec_vec;

      typedef std::vector<size_t> iq_map_vec;

} /* namespace svl */

} /* namespace gr */


#endif /* ifndef INCLUDED_SVL_TYPES_H */
