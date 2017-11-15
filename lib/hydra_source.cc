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
}

/** DTOR
*/
hydra_source::~hydra_source()
{
}

void
hydra_source::forecast(int nouput_items, gr_vector_int &ninput_items_required)
{
   ninput_items_required[0] = g_hypervisor->get_total_subcarriers();
}

int
hydra_source::general_work(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
{
   g_hypervisor->source_add_samples(noutput_items,
         ninput_items,
         input_items);

   std::cout << "ninput: " << ninput_items[0] << std::endl;

   // Consume items in the input port.
   // We use the for loop even though we have only one input port
   for (size_t i = 0; i < ninput_items.size(); ++i)
      consume(i, ninput_items[i]);

   // Get demultiplexed output from VRs
   gr_vector_int nproduced = g_hypervisor->source_outbuf(output_items);

   for (size_t it = 0; it < nproduced.size(); ++it)
   {
      produce(it , nproduced[it]);
      std::cout << "produced: " << nproduced[it] << std::endl;
   }

   return WORK_CALLED_PRODUCE;
}


} /* namespace hydra */
} /* namespace gr */
