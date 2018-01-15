/* -*- c++ -*- */
/* 
 * Copyright 2016 Trinity Connect Centre.
 * 
 * HyDRA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * HyDRA is distributed in the hope that it will be useful,
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

#include <hydra/hydra_async_sink.h>
#include <gnuradio/io_signature.h>

#include <string.h>

namespace gr {
   namespace hydra {

hydra_async_sink::hydra_async_sink_ptr
hydra_async_sink::make(size_t _n_ports,
      size_t _fft_m_len,
      double central_frequency,
      double bandwidth,
      const std::vector<std::vector<double> > vradio_conf)
{
   return gnuradio::get_initial_sptr(new hydra_async_sink(_n_ports,
         _fft_m_len,
         central_frequency,
         bandwidth,
         vradio_conf));
}

hydra_async_sink::hydra_async_sink(size_t _n_inputs,
      size_t _fft_m_len,
      double central_frequency,
      double bandwidth,
      const std::vector< std::vector<double> > vradio_conf):
         gr::block("hydra_sink",
               gr::io_signature::make(0, 0, 0),
               gr::io_signature::make(1, 1, sizeof(gr_complex))),
         hydra_block(_n_inputs, _fft_m_len, central_frequency, bandwidth)
{
   set_output_multiple(_fft_m_len);

   for (size_t i = 0; i < _n_inputs; ++i)
   {
      create_vradio(vradio_conf[i][0], vradio_conf[i][1]);

      std::string msg_id = std::string("vr") + std::to_string(i);
      message_port_register_in(pmt::mp(msg_id));

      set_msg_handler(pmt::mp(msg_id),
                      boost::bind(&hydra_async_sink::handle_msg, this, _1, i));
   }

   g_hypervisor->set_radio_mapping();
}

hydra_async_sink::~hydra_async_sink()
{
}

void
hydra_async_sink::handle_msg(pmt::pmt_t val, size_t radio_id)
{
   std::vector<gr_complex> xv;

   // If pair, get the value
   if(pmt::is_pair(val))
        val = pmt::cdr(val);

   //  get elements
   if(pmt::is_c32vector(val))
   {
        xv = pmt::c32vector_elements(val);
   }

   g_hypervisor->get_vradio(radio_id)->add_sink_sample(
      (const gr_complex *) &xv[0], xv.size());
}

int
hydra_async_sink::general_work(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
   // Gen output
   int t = g_hypervisor->sink_outbuf(output_items, noutput_items);

   return t;
}

} /* namespace hydra */
} /* namespace gr */
