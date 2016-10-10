/* -*- c++ -*- */
/* 
 * Copyright 2016 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_SVL_SVL_SINK_IMPL_H
#define INCLUDED_SVL_SVL_SINK_IMPL_H

#include <svl/api.h>
#include <svl/svl_block.h>

namespace gr {
   namespace svl {

class SVL_API svl_sink: public svl_block
{
   public:
      typedef boost::shared_ptr<svl_sink> svl_sink_ptr;

      /**
       * @param _n_inputs
       * @param _fft_m_len
       * @param _fft_n_len
       */
      static svl_sink_ptr make(size_t _n_ports,
            size_t _fft_m_len,
            const std::vector<int> _fft_n_len);

      /** CTOR
       */
      svl_sink(size_t _n_inputs,
            size_t _fft_m_len,
            const std::vector<int> _fft_n_len);

      /** DTOR
       */
      ~svl_sink();

      // Where all the action really happens
		int general_work(int noutput_items,
              gr_vector_int &ninput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items);

		/*
		void forecast(int noutput_items, gr_vector_int &ninput_items_required)
		{
				  return g_hypervisor->forecast(noutput_items, ninput_items_required);
		}
		*/
};

} // namespace svl
} // namespace gr

#endif /* INCLUDED_SVL_SVL-SINK_IMPL_H */
