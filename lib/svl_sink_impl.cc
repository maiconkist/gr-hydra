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
#include <gnuradio/fft/fft.h>
#include <string.h>

#include "svl_sink_impl.h"

namespace gr {
  namespace svl {

	typedef boost::shared_ptr<gr::fft::fft_complex> sfft_complex;
	typedef std::vector<gr_complex> samples_vec;
	typedef std::vector<size_t> iq_map_vec;

    /*
     *
     * VRADIO
     *
     *
     */
	class VirtualRadio
	{
		private:

			size_t fft_n_len;
			samples_vec g_tx_samples;
			samples_vec g_rx_samples;

			sfft_complex g_fft_complex;
			sfft_complex g_ifft_complex;

			iq_map_vec g_iq_map;

		public:
			VirtualRadio(size_t _fft_n_len):
					fft_n_len(_fft_n_len)
			{
				g_fft_complex = sfft_complex(new gr::fft::fft_complex(fft_n_len) ) ;
				g_ifft_complex = sfft_complex(new gr::fft::fft_complex(fft_n_len, false) ) ;
			}

			size_t const get_subcarriers()
			{
				return fft_n_len;	
			}

			void set_subcarriers(size_t _fft_n_len)
			{
				fft_n_len = _fft_n_len;
			}

			void set_tx_samples(const samples_vec &tx_samples)
			{
					if (g_tx_samples.size() != tx_samples.size())
					{
						// Error msg	
					}

					g_tx_samples.insert(g_tx_samples.end(),tx_samples.begin(),tx_samples.end());
			}

			void set_iq_mapping(const iq_map_vec &iq_map)
			{
					if (iq_map.size() != fft_n_len)
					{
							// Error msg
					}

					g_iq_map = iq_map;
			}

			void demap_iq_samples(samples_vec samples_buf)
			{
					samples_vec rx_samples_freq(fft_n_len, 0);


					// Copy the samples used by this radio
					size_t idx(0);
					for (iq_map_vec::iterator it = g_iq_map.begin();
						it != g_iq_map.end();
						++it, ++idx)
					{
						rx_samples_freq[idx] = samples_buf[*it];	
					}

					// Transfer samples to fft_complex buff and perform fft
          			memcpy(g_ifft_complex->get_inbuf(), &rx_samples_freq[0], fft_n_len);
					g_ifft_complex->execute();

					// Copy buff from fft_complex
					g_rx_samples = samples_vec(g_ifft_complex->get_outbuf(), g_ifft_complex->get_outbuf() + fft_n_len);
			}

			void map_iq_samples(samples_vec samples_buf)
			{
					memcpy(g_fft_complex->get_inbuf(), &g_tx_samples[0], fft_n_len);
					samples_vec tx_samples_freq(g_fft_complex->get_outbuf(), g_fft_complex->get_outbuf() + fft_n_len);

					size_t idx(0);
					for (samples_vec::iterator it = tx_samples_freq.begin();
						it != tx_samples_freq.end();
						++it, ++idx)
					{
						samples_buf[idx] = *it;
					}
			}
	};


    /*
     *
     * HYPERVISOR
     *
     *
     */
     class Hypervisor
     {
			typedef boost::shared_ptr<VirtualRadio> vradio_ptr;
			typedef std::vector<vradio_ptr> vradio_vec;

		private:
				vradio_vec g_vradios;


				size_t fft_m_len;
				samples_vec g_tx_iq_vec;
			    samples_vec g_rx_iq_vec;

			   	sfft_complex g_fft_complex;

		public:

		     Hypervisor(size_t _fft_m_len):
					 fft_m_len(_fft_m_len)
		     {
					 g_tx_iq_vec.resize(fft_m_len);
					 g_rx_iq_vec.resize(fft_m_len);
		     };

			 size_t create_vradio()
			 {
				vradio_ptr vradio(new VirtualRadio(0));
				g_vradios.push_back( vradio );

				// ID representing the radio;
				return g_vradios.size() - 1;
			 };

			 int set_vradio_subcarriers(size_t vradio_id, size_t bandwidth)
			 {
					 g_vradios[vradio_id]->set_subcarriers(bandwidth);

					 return 1;
			 }

			 void set_radio_mapping()
			 {
					 iq_map_vec sc_allocated(fft_m_len, 0);


					 size_t idx(0);
					 for (vradio_vec::iterator it = g_vradios.begin();
									 it != g_vradios.end();
									 ++it, ++idx)
					 {
							 size_t sc_needed = std::min((*it)->get_subcarriers(), fft_m_len);

							 iq_map_vec the_map;


							 for (size_t idx = 0; idx < fft_m_len; ++idx)
							 {
									 if (sc_allocated[idx] == 0)
									 {
											 the_map.push_back(idx);
											 sc_allocated[idx] = 1;
									 }

									 if (the_map.size() == sc_needed)
											 break;
							 
							 }

							 (*it)->set_iq_mapping(the_map);
					 }
			 }

			 void multiplex()
			 {
					 g_tx_iq_vec = samples_vec(fft_m_len);


					 for (vradio_vec::iterator it = g_vradios.begin();
									 it != g_vradios.end();
									 ++it)
					 {
							 (*it)->set_tx_samples(g_tx_iq_vec);
					 
					 }
			 }

			 void demultiplex()
			 {
					memcpy(g_fft_complex->get_inbuf(), &g_rx_iq_vec[0], fft_m_len);

					g_fft_complex->execute();

					samples_vec rx_samples_freq(g_fft_complex->get_outbuf(), g_fft_complex->get_outbuf() + fft_m_len);
			 }

     };

    /*
     *
     *
     *
     *
     */

    svl_sink::sptr
    svl_sink::make()
    {
      return gnuradio::get_initial_sptr
        (new svl_sink_impl());
    }

    /*
     * The private constructor
     */
    svl_sink_impl::svl_sink_impl()
      : gr::sync_block("svl_sink",
              gr::io_signature::make(1, 2, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
	    g_hypervisor = hypervisor_ptr(new Hypervisor(128));	
    }

    /*
     * Our virtual destructor.
     */
    svl_sink_impl::~svl_sink_impl()
    {
    }


	size_t svl_sink_impl::create_vradio()
	{
		return g_hypervisor->create_vradio();	
	}

	int svl_sink_impl::set_vradio_subcarriers(size_t vradio_id, size_t bandwidth)
	{
		return g_hypervisor->set_vradio_subcarriers(vradio_id, bandwidth);	
	}


    int
    svl_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *inprt = (const gr_complex *) input_items[0];
      gr_complex *outptr = (gr_complex *) output_items[0];

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace svl */
} /* namespace gr */

