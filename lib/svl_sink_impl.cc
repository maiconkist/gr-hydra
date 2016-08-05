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
//#include <gnuradio/fft/fft.h>
#include <fftw3.h>
#include <string.h>

#include "svl_sink_impl.h"

#define PRINT_VEC_SAMPLES(x) \
   std::cout << "P:" << __FUNCTION__ << "[" << __LINE__ << "] -- " << \
#x << " -- len: " << x.size() << std::endl;  \
for (samples_vec::iterator it = x.begin(); it != x.end(); ++it){ \
   std::cout << *it << ","; \
} \
std::cout << std::endl;

namespace gr {
   namespace svl {


      // Implemented my own fft_complex clas.
      // GNURADIO's one was not working properly. To work, I would need to perform memory aligments in the input buffers
      // So, I implemented this one which has the same function signatures.
      class fft_complex 
      {

         private:
            size_t g_fft_size;
            gr_complex *g_inbuf, *g_outbuf;
            fftwf_plan g_plan;

         public:
            fft_complex(size_t fft_size, bool forward = true):
               g_fft_size(fft_size)
            {
               g_inbuf = new gr_complex[fft_size];
               g_outbuf = new gr_complex[fft_size];

               g_plan = fftwf_plan_dft_1d(fft_size,
                     reinterpret_cast<fftwf_complex *>(g_inbuf),
                     reinterpret_cast<fftwf_complex *>(g_outbuf),
                     forward? FFTW_FORWARD:FFTW_BACKWARD,
                     FFTW_MEASURE);
            }

            gr_complex*
            get_inbuf()
            {
               return g_inbuf;
            }

            gr_complex*
            get_outbuf()
            {
               return g_outbuf;
            }

            void
            execute()
            {
               fftwf_execute(g_plan);
            }
      };

      class VirtualRadio;

      typedef boost::shared_ptr<gr::svl::fft_complex> sfft_complex;
      typedef std::vector<gr_complex> samples_vec;
      typedef std::vector<size_t> iq_map_vec;
      typedef boost::shared_ptr<VirtualRadio> vradio_ptr;

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
            size_t g_idx;

            samples_vec g_tx_samples;
            samples_vec g_rx_samples;

            sfft_complex g_fft_complex;
            sfft_complex g_ifft_complex;

            iq_map_vec g_iq_map;

         public:
            VirtualRadio(size_t _idx, size_t _fft_n_len):
               fft_n_len(_fft_n_len),
               g_idx(_idx)
            {
               g_fft_complex = sfft_complex(new gr::svl::fft_complex(fft_n_len)) ;
               g_ifft_complex = sfft_complex(new gr::svl::fft_complex(fft_n_len, false));
            }

            size_t const
            get_subcarriers()
            {
               return fft_n_len; 
            }

            void
            set_subcarriers(size_t _fft_n_len)
            {
               fft_n_len = _fft_n_len;
            }

            void
            add_iq_sample(const gr_complex *samples, size_t len)
            {
               g_tx_samples.insert(g_tx_samples.end(), samples, samples + len);
            }

            void
            set_iq_mapping(const iq_map_vec &iq_map)
            {
               if (iq_map.size() != fft_n_len)
               {
                  // Error msg
               }

               g_iq_map = iq_map;
            }

            void
            demap_iq_samples(samples_vec samples_buf)
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

            bool
            const ready_to_map_iq_samples()
            {
               if (g_tx_samples.size() < fft_n_len)
               {
                  return false;
               }
               return true;
            }

            bool
            map_iq_samples(samples_vec &samples_buf)
            {
               if (!ready_to_map_iq_samples()) return false;

               // Copy samples in TIME domain to FFT buffer, execute FFT
               memcpy(g_fft_complex->get_inbuf(), &g_tx_samples[0], fft_n_len);
               g_fft_complex->execute();
               samples_vec tx_samples_freq(g_fft_complex->get_outbuf(), g_fft_complex->get_outbuf() + fft_n_len);


               // map samples in FREQ domain to samples_buff
               size_t idx(0);
               for (iq_map_vec::iterator it = g_iq_map.begin();
                     it != g_iq_map.end();
                     ++it, ++idx)
               {
                  samples_buf[(*it)] = tx_samples_freq[idx]; 
               }

               return true;
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
         typedef std::vector<vradio_ptr> vradio_vec;

         private:
         vradio_vec g_vradios;


         size_t fft_m_len;
         samples_vec g_tx_iq_vec;
         samples_vec g_rx_iq_vec;

         sfft_complex g_fft_complex;
         sfft_complex g_ifft_complex;

         public:

         Hypervisor(size_t _fft_m_len):
            fft_m_len(_fft_m_len)
         {
            g_fft_complex = sfft_complex(new gr::svl::fft_complex(fft_m_len));
            g_ifft_complex = sfft_complex(new gr::svl::fft_complex(fft_m_len, false));
         };

         size_t
         create_vradio(size_t _fft_n_len)
         {
            vradio_ptr vradio(new VirtualRadio(g_vradios.size(), _fft_n_len));
            g_vradios.push_back(vradio);

            // ID representing the radio;
            return g_vradios.size() - 1;
         };

         vradio_ptr
         get_vradio(size_t idx)
         {
            if (idx > g_vradios.size())
               std::cout << "ERR: " << __FUNCTION__ << std::endl;
            return g_vradios[idx];
         }

         size_t const
         get_subcarriers()
         {
            return fft_m_len;
         }

         bool const
         tx_ready()
         {
            for (vradio_vec::iterator it = g_vradios.begin();
                  it != g_vradios.end();
                  ++it)
            {
               if (!(*it)->ready_to_map_iq_samples())
               {
                  return false;
               }
            }
            return true;

         }

         int
         set_vradio_subcarriers(size_t vradio_id, size_t bandwidth)
         {
            g_vradios[vradio_id]->set_subcarriers(bandwidth);
            return 1;
         }

         void
         set_radio_mapping()
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

         void
         demultiplex()
         {
            memcpy(g_fft_complex->get_inbuf(), &g_rx_iq_vec[0], sizeof(gr_complex) * fft_m_len);
            g_fft_complex->execute();
            samples_vec rx_samples_freq(g_fft_complex->get_outbuf(), g_fft_complex->get_outbuf() + fft_m_len);
         }

         /*
          *
          *
          */
         samples_vec
         get_tx_outbuf()
         {
            std::cout << __FUNCTION__ << "[" << __LINE__ << "]" << std::endl;
            /* Return the vector with samples
             * @return Vector of samples to transmit in TIME domain.
             */

            // Check if we can generate the samples to transmit
            if (!tx_ready()) return samples_vec(0);

            // For each VirtualRadio call the map_iq_samples func passing our buffer as parameter
            samples_vec samp_freq_vec(fft_m_len);
            for (vradio_vec::iterator it = g_vradios.begin();
                  it != g_vradios.end();
                  ++it)
            {
               (*it)->map_iq_samples(samp_freq_vec);
            }

            // Transform buffer from FREQ domain to TIME domain using IFFT
            memcpy(g_ifft_complex->get_inbuf(), &samp_freq_vec[0], sizeof(gr_complex)* fft_m_len);
            g_ifft_complex->execute();

            g_tx_iq_vec = samples_vec(g_ifft_complex->get_outbuf(),
                  g_ifft_complex->get_outbuf() + fft_m_len);

            for (size_t i = 0; i < fft_m_len; ++i)
               g_tx_iq_vec[i] = g_ifft_complex->get_outbuf()[i];

            // Finished
            PRINT_VEC_SAMPLES(samp_freq_vec);
            PRINT_VEC_SAMPLES(g_tx_iq_vec);
            return g_tx_iq_vec;
         }
      };

      /*
       *
       *
       *
       *
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

      /*
       * The private constructor
       */
      svl_sink_impl::svl_sink_impl(size_t _n_inputs,
                      size_t _fft_m_len,
                      const std::vector<int> _fft_n_len):
         gr::block("svl_sink",
            gr::io_signature::make(_n_inputs, _n_inputs, sizeof(gr_complex)),
            gr::io_signature::make(1, 1, sizeof(gr_complex)*_fft_m_len))
      {
         g_hypervisor = hypervisor_ptr(new Hypervisor(_fft_m_len));  

	 for (size_t i = 0; i < _n_inputs; ++i)
		 create_vradio(_fft_n_len[i]);

	 g_hypervisor->set_radio_mapping();
      }

      /*
       * Our virtual destructor.
       */
      svl_sink_impl::~svl_sink_impl()
      {
      }

      size_t
      svl_sink_impl::create_vradio(size_t _fft_n_len)
      {
         return  g_hypervisor->create_vradio(_fft_n_len);  
      }

      int
      svl_sink_impl::set_vradio_subcarriers(size_t _vradio_id, size_t _fft_n_len)
      {
         return g_hypervisor->set_vradio_subcarriers(_vradio_id, _fft_n_len);   
      }

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
            std::cout << __FUNCTION__	<< ": Forwarding samples to Radio " << i << std::endl;
            const gr_complex *in = reinterpret_cast<const gr_complex *>(input_items[i]);
            g_hypervisor->get_vradio(i)->add_iq_sample(in, ninput_items[i]);

            // Consume the items in the input port i
            consume(i, ninput_items[i]);
         }

         // Check if hypervisor is ready to transmit
         if (g_hypervisor->tx_ready())
         {
            // Get buffer in TIME domain
            samples_vec out_samples_vec = g_hypervisor->get_tx_outbuf();

            // Copy to GNURADIO buffer
            memcpy(reinterpret_cast<gr_complex *>(output_items[0]),
                  &out_samples_vec[0],
                  out_samples_vec.size());

            // Return what GNURADIO expects
            return noutput_items;
         }

         // No outputs generated.
         return 0;
      }
   } /* namespace svl */
} /* namespace gr */
