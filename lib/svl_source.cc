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

#include <svl/svl_source.h>
#include <gnuradio/io_signature.h>

#include "easylogging++.h"

namespace gr {
	namespace svl {

svl_source::svl_source_ptr
svl_source::make(size_t _n_ports,
		size_t _fft_m_len,
      const std::vector<int> _fft_n_len)
{
   LOG(INFO) << "making svl_source";
   return gnuradio::get_initial_sptr(new svl_source(_n_ports,
         _fft_m_len,
         _fft_n_len));
}

svl_source::svl_source(size_t _n_outputs,
		size_t _fft_m_len,
      const std::vector<int> _fft_n_len):gr::block("svl_source",
	gr::io_signature::make(1, 1, sizeof(gr_complex)),
   gr::io_signature::make(_n_outputs, _n_outputs, sizeof(gr_complex) * _fft_m_len))
{
   g_hypervisor = hypervisor_ptr(new Hypervisor(_fft_m_len));

   for (size_t i = 0; i < _n_outputs; ++i)
      create_vradio(_fft_n_len[i]);

   g_hypervisor->set_radio_mapping();
   LOG(INFO) << "New svl_source created: outputs: " << _n_outputs;
}

svl_source::~svl_source()
{
}

int
svl_source::general_work(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
   LOG(INFO) << "input_items.size(): "  << input_items.size();
   LOG(INFO) << "output_items.size(): " << output_items.size();
   LOG(INFO) << "noutput_items: "       << noutput_items;

   // Get input buffer
   const gr_complex *in = reinterpret_cast<const gr_complex *>(input_items[0]);

   g_hypervisor->rx_add_samples(in, ninput_items[0]);
   consume_each(ninput_items[0]);

   if (g_hypervisor->rx_ready())
   {
      size_t t =  g_hypervisor->rx_outbuf(reinterpret_cast<gr_complex *>(output_items[0]), noutput_items);

      // Tell runtime system how many output items we produced.
   	LOG(INFO) << "Output generated";
      return t;
   }


   // Tell runtime system how many output items we produced.
   return 0;
}


} /* namespace svl */
} /* namespace gr */
