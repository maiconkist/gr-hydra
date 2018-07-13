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

#include <hydra/hydra_source.h>
#include <gnuradio/io_signature.h>

namespace gr {
	namespace hydra {

hydra_source::hydra_source_ptr
hydra_source::make(size_t _n_ports,
		size_t _fft_m_len,
		double central_frequency,
		double bandwidth,
      const std::vector< std::vector<double> > vradio_conf)
{
   return gnuradio::get_initial_sptr(new hydra_source(_n_ports,
            _fft_m_len,
            central_frequency,
            bandwidth,
            vradio_conf));
}

hydra_source::hydra_source(size_t _n_outputs,
      size_t _fft_m_len,
      double central_frequency,
      double bandwidth,
      const std::vector< std::vector<double> > vradio_conf):
   gr::block("hydra_source",
         gr::io_signature::make(1, 1, sizeof(gr_complex)),
         gr::io_signature::make(_n_outputs, _n_outputs, sizeof(gr_complex))),
   hydra_block(_n_outputs, _fft_m_len, central_frequency, bandwidth)
{
   for (size_t i = 0; i < _n_outputs; ++i)
      create_vradio(vradio_conf[i][0], vradio_conf[i][1]);

   g_hypervisor->set_radio_mapping();

   set_output_multiple(_fft_m_len);
}

/** DTOR
*/
hydra_source::~hydra_source()
{
}

void
hydra_source::forecast(int noutput_items,
      gr_vector_int &ninput_items_required)
{
   ninput_items_required[0] = noutput_items;
}

int
hydra_source::general_work(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
#if 0
   size_t consumed = g_hypervisor->source_add_samples(noutput_items,
         ninput_items,
         input_items);

   // Get demultiplexed output from VRs
   gr_vector_int nproduced = g_hypervisor->get_source_outbuf(noutput_items, output_items);

   // Tell scheduler that input samples have been consumed
   consume(0, consumed);

   for (size_t idx = 0; idx < output_items.size(); ++idx)
      produce(idx, nproduced[idx]);

#endif
   return WORK_CALLED_PRODUCE;
}

} /* namespace hydra */
} /* namespace gr */
