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

#include <gnuradio/io_signature.h>
#include <string.h>

#include "svl_sink_impl.h"
#include "svl/svl_fft.h"
#include "svl/svl_virtual_radio.h"

namespace gr {
   namespace svl {

/*
 * @param _n_inputs
 * @param _fft_m_len
 * @param _fft_n_len
 */
svl_sink::sptr
svl_sink::make(size_t _n_inputs,
               size_t _fft_m_len,
               const std::vector<int> _fft_n_len)
{
	return gnuradio::get_initial_sptr(new svl_sink_impl(_n_inputs,
      _fft_m_len,
      _fft_n_len));
}

/**
 * The private constructor
 * @param _n_inputs
 * @param _fft_m_len
 * @param _fft_n_len
 */
svl_sink_impl::svl_sink_impl(size_t _n_inputs,
                size_t _fft_m_len,
                const std::vector<int> _fft_n_len): gr::block("svl_sink",
      gr::io_signature::make(_n_inputs, _n_inputs, sizeof(gr_complex)),
      gr::io_signature::make(1, 1, sizeof(gr_complex)*_fft_m_len))
{
   g_hypervisor = hypervisor_ptr(new Hypervisor(_fft_m_len));  

   for (size_t i = 0; i < _n_inputs; ++i)
      create_vradio(_fft_n_len[i]);

   g_hypervisor->set_radio_mapping();
}

/**
 * Our virtual destructor.
 */
svl_sink_impl::~svl_sink_impl()
{
}

/**
 * @param _fft_n_len
 */
size_t
svl_sink_impl::create_vradio(size_t _fft_n_len)
{
   return  g_hypervisor->create_vradio(_fft_n_len);  
}

/**
 * @param _vradio_id
 * @param _fft_n_len
 */
int
svl_sink_impl::set_vradio_subcarriers(size_t _vradio_id, size_t _fft_n_len)
{
   return g_hypervisor->set_vradio_subcarriers(_vradio_id, _fft_n_len);   
}

/**
 * @param noutput_items
 * @param ninput_items_required
 */
/*
void
svl_sink_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{
   g_hypervisor->forecast(noutput_items, ninput_items_required);
}
*/

/**
 * @param noutput_items
 * @param ninput_items
 * @param input_items
 * @param output_items
 */
int
svl_sink_impl::general_work(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
   std::cout << "----- work \n"
      << "input_items.size(): " << input_items.size() << "\n"
      << "output_items.size(): " << output_items.size() << "\n"
      << "noutput_items: " << noutput_items << "\n"
   << std::endl;

   for (size_t i = 0; i < ninput_items.size(); ++i)
   {
      // For each input port
      // Get the input port buffer
      // Send the buffer to the correct virtual radio
      //
      // TRICKY: Im assuming that input port 0 is mapped to Virtual Radio 0
      //         ........................... 1 .......................... 1
      //         ........................... 2 .......................... 2
      //         ........................... N .......................... N
      const gr_complex *in = reinterpret_cast<const gr_complex *>(input_items[i]);
      g_hypervisor->get_vradio(i)->add_iq_sample(in, ninput_items[i]);

      // Consume the items in the input port i
      consume(i, ninput_items[i]);
   }

   // Check if hypervisor is ready to transmit
   if (g_hypervisor->tx_ready())
   {
      // Get buffer in TIME domain
      // Return what GNURADIO expects
      size_t t =  g_hypervisor->get_tx_outbuf(reinterpret_cast<gr_complex *>(output_items[0]), noutput_items);
      return t;
   }

   // No outputs generated.
   return 0;
}
} /* namespace svl */
} /* namespace gr */
