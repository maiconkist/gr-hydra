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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <svl/svl_source.h>

namespace gr {
   namespace svl {

svl_block::sptr
svl_source::make(size_t _n_ports,
               size_t _fft_m_len,
               const std::vector<int> _fft_n_len)
{
	return gnuradio::get_initial_sptr(new svl_source());
}


svl_source::svl_source() : gr::block("svl_source",
      gr::io_signature::make(0, 0, 0),
      gr::io_signature::make(0, 0, 0))
{

}

svl_source::~svl_source()
{
}

int
svl_source::work(int noutput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
   // Tell runtime system how many output items we produced.
   return noutput_items;
}


   } /* namespace svl */
} /* namespace gr */
