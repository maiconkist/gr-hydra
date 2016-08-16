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

#include <svl/svl_sink.h>

namespace gr {
   namespace svl {

      class Hypervisor;

      class svl_sink_impl : public svl_sink
      {
         private:
            typedef boost::shared_ptr<Hypervisor> hypervisor_ptr;
            hypervisor_ptr g_hypervisor;

         public:
				/**
				 * @param _n_inputs
				 * @param _fft_m_len
				 * @param _fft_n_len
				 */
            svl_sink_impl(size_t _n_inputs,
                            size_t _fft_m_len,
                            const std::vector<int> _fft_n_len);

            ~svl_sink_impl();

            // Where all the action really happens
				int general_work(int noutput_items,
                            gr_vector_int &ninput_items,
                            gr_vector_const_void_star &input_items,
                            gr_vector_void_star &output_items);

            /**
             * @param noutput_items
             * @param ninput_items_required
             */ 
            //void forecast(int noutput_items, gr_vector_int &ninput_items_required);


            // implementation of svl_sink virtual methods
            virtual size_t create_vradio(size_t _fft_n_len); 
            virtual int set_vradio_subcarriers(size_t _vradio_id, size_t _fft_n_len);
         };

  } // namespace svl
} // namespace gr

#endif /* INCLUDED_SVL_SVL-SINK_IMPL_H */
