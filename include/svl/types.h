#ifndef INCLUDED_SVL_TYPES_H
#define INCLUDED_SVL_TYPES_H

#include <vector>
#include <queue>
#include <boost/shared_ptr.hpp>

#include <gnuradio/types.h>

namespace gr {
	namespace svl {
		typedef boost::shared_ptr<gr_complex[]> samples_ptr;
		typedef std::vector<gr_complex> samples_vec;
		typedef std::queue<samples_vec> samples_vec_vec;

		typedef std::vector<int> iq_map_vec;
} /* namespace svl */

} /* namespace gr */


#endif /* ifndef INCLUDED_SVL_TYPES_H */
