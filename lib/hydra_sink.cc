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

#include <hydra/hydra_sink.h>
#include <gnuradio/io_signature.h>

#include <string.h>

namespace gr {
   namespace hydra {

hydra_sink::hydra_sink_ptr
hydra_sink::make(size_t _n_ports,
      size_t _fft_m_len,
		double central_frequency,
		double bandwidth,
      const std::vector<std::vector<double> > vradio_conf)
{
   return gnuradio::get_initial_sptr(new hydra_sink(_n_ports,
         _fft_m_len,
			central_frequency,
			bandwidth,
         vradio_conf));
}

hydra_sink::hydra_sink(size_t _n_inputs,
      size_t _fft_m_len,
		double central_frequency,
		double bandwidth,
      const std::vector< std::vector<double> > vradio_conf):
			gr::block("hydra_sink",
   				gr::io_signature::make(_n_inputs,
							  _n_inputs, sizeof(gr_complex)),
   				gr::io_signature::make(1, 1, sizeof(gr_complex))),
			hydra_block(_n_inputs, _fft_m_len, central_frequency, bandwidth)
{
	set_output_multiple(_fft_m_len);

	for (size_t i = 0; i < _n_inputs; ++i)
		create_vradio(vradio_conf[i][0], vradio_conf[i][1]);

	g_hypervisor->set_radio_mapping();
}

hydra_sink::~hydra_sink()
{
}

int
hydra_sink::general_work(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
   // forward to hypervisor
   g_hypervisor->tx_add_samples(noutput_items, ninput_items, input_items);

	// Gen output
   int t = g_hypervisor->tx_outbuf(output_items, noutput_items);

   // Consume the items in the input port i
   for (size_t i = 0; i < ninput_items.size(); ++i)
      consume(i, ninput_items[i]);

   produce(0, t);


   return WORK_CALLED_PRODUCE;
}

} /* namespace hydra */
} /* namespace gr */
