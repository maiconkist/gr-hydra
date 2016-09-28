/* -*- c++ -*- */
/* 
 * Copyright 2016 Trinity Connect Centre.
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

#include <svl/svl_sink.h>
#include <gnuradio/io_signature.h>

#include <string.h>
#include "easylogging++.h"

namespace gr {
   namespace svl {

svl_sink::svl_sink_ptr
svl_sink::make(size_t _n_ports,
      size_t _fft_m_len,
      const std::vector<int> _fft_n_len)
{
   return gnuradio::get_initial_sptr(new svl_sink(_n_ports,
         _fft_m_len,
         _fft_n_len));
}


svl_sink::svl_sink(size_t _n_inputs,
      size_t _fft_m_len,
      const std::vector<int> _fft_n_len):gr::block("svl_sink",
   gr::io_signature::make(_n_inputs, _n_inputs, sizeof(gr_complex)),
   gr::io_signature::make(1, 1, sizeof(gr_complex)*_fft_m_len))
{
   LOG(INFO) << "New svl_sink created: inputs: " << _n_inputs;
   g_hypervisor = hypervisor_ptr(new Hypervisor(_fft_m_len));

   for (size_t i = 0; i < _n_inputs; ++i)
      create_vradio(_fft_n_len[i]);

   g_hypervisor->set_radio_mapping();
}


svl_sink::~svl_sink()
{
}


int
svl_sink::general_work(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
   LOG(INFO) << "input_items.size(): " << input_items.size();
   LOG(INFO) << "output_items.size(): " << output_items.size();
   LOG(INFO) << "noutput_items: " << noutput_items;

   // Consume the items in the input port i
   for (size_t i = 0; i < ninput_items.size(); ++i)
      consume(i, ninput_items[i]);

   // forward to hypervisor
   g_hypervisor->tx_add_samples(ninput_items, input_items);

   // Check if hypervisor is ready to transmit
   if (g_hypervisor->tx_ready())
   {
      // Get buffer in TIME domain
      // Return what GNURADIO expects
      size_t t =  g_hypervisor->tx_outbuf(reinterpret_cast<gr_complex *>(output_items[0]), noutput_items);
      return t;
   }

   // No outputs generated.
   return 0;
}

} /* namespace svl */
} /* namespace gr */
