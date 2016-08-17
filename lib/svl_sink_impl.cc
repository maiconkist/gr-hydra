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

            std::vector<samples_vec> g_tx_samples;
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

            /**
             * @param _fft_n_len
             */
            void
            set_subcarriers(size_t _fft_n_len)
            {
               fft_n_len = _fft_n_len;
            }


            /**
             * The number of IQ samples required to produced noutput_items output
             * One output is a buffer with fft_n_items
             *
             * @param noutput_items Total of noutput_items required
             * @return Total of IQ samples require to produce nouput_items
             */
            int
            forecast(int noutput_items)
            {
               if (0 == g_tx_samples.size())
                       return noutput_items * fft_n_len;

               int required = noutput_items - g_tx_samples.size();
               if (required <= 0) return 0;

               if (g_tx_samples.back().size() < fft_n_len)
               {
                       required -= 1;
                       return (required * fft_n_len) + (fft_n_len - g_tx_samples.back().size());
               }
               else return (required * fft_n_len);
            }

            /**
             * @return fft_n_len
             */
            size_t const
            get_subcarriers()
            {
               return fft_n_len; 
            }

            /** Added the buff samples to the VR tx queue.
             *
             * \param samples The samples  that must be added to the VR tx queue.
             * \param len samples lenght.
             */
            void
            add_iq_sample(const gr_complex *samples, size_t len)
            {
               // the total of if itens we have transfered so far
               size_t consumed = 0;

               if (0 == g_tx_samples.size())
                       g_tx_samples.push_back(samples_vec(0));

               // While we have samples to transfer
               while (consumed < len)
               {
                  // If we filled the last samples_vec, create a new one
                  if (g_tx_samples.back().size() == fft_n_len)
                     g_tx_samples.push_back(samples_vec());

                  size_t rest = std::min(len - consumed,
                                  fft_n_len - g_tx_samples.back().size());

                  // TRICKY: use std::copy instead of this loop.
		            // Was seg faulting
                  for (int i = 0; i < rest; ++i )
                     g_tx_samples.back().push_back(samples[consumed+i]);

                  consumed += rest;
               }

               if (consumed > len)
                  std::cout << __FUNCTION__ << ": ERROR" << std::endl;
            }

            /**
             * @param iq_map
             */
            void
            set_iq_mapping(const iq_map_vec &iq_map)
            {
               if (iq_map.size() != fft_n_len)
               {
                  // Error msg
               }

               g_iq_map = iq_map;
            }

            /**
             * @param samples_buf
             */
            void
            demap_iq_samples(samples_vec samples_buf)
            {
               samples_vec rx_samples_freq(fft_n_len);

               // Copy the samples used by this radio
               size_t idx(0);
               for (iq_map_vec::iterator it = g_iq_map.begin();
                     it != g_iq_map.end();
                     ++it, ++idx)
               {
                  rx_samples_freq[idx] = samples_buf[*it];  
               }

               // Transfer samples to fft_complex buff and perform fft
               std::copy(rx_samples_freq.begin(), rx_samples_freq.end(),
                               g_ifft_complex->get_inbuf());
               g_ifft_complex->execute();

               // Copy buff from fft_complex
               g_rx_samples = samples_vec(g_ifft_complex->get_outbuf(),
                               g_ifft_complex->get_outbuf() + fft_n_len);
            }


            /**
             */
            bool
            const ready_to_map_iq_samples()
            {
               if (g_tx_samples.size() == 0 || (g_tx_samples[0].size() < fft_n_len))
               {
                  return false;
               }

               return true;
            }

            /**
             * @param samples_buf
             */
            bool
            map_iq_samples(samples_vec &samples_buf)
            {
               if (!ready_to_map_iq_samples()) return false;

               // Copy samples in TIME domain to FFT buffer, execute FFT
               std::copy(g_tx_samples[0].begin(),
                               g_tx_samples[0].end(),
                               g_fft_complex->get_inbuf());

               g_fft_complex->execute();

               samples_vec tx_samples_freq(g_fft_complex->get_outbuf(),
                               g_fft_complex->get_outbuf() + fft_n_len);

               g_tx_samples.erase(g_tx_samples.begin());

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

         /**
          * @param _fft_n_len
          * @return New radio id
          */
         size_t
         create_vradio(size_t _fft_n_len)
         {
            vradio_ptr vradio(new VirtualRadio(g_vradios.size(), _fft_n_len));
            g_vradios.push_back(vradio);

            // ID representing the radio;
            return g_vradios.size() - 1;
         };

         /**
          * @param idx
          * @return vradio_ptr to VR
          */
         vradio_ptr
         get_vradio(size_t idx)
         {
            if (idx > g_vradios.size())
               std::cout << "ERR: " << __FUNCTION__ << std::endl;
            return g_vradios[idx];
         }

         /**
          * @return fft_m_len
          */
         size_t const
         get_subcarriers()
         {
            return fft_m_len;
         }

         /**
          * @return true if can generate output
          */
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

         /**
          * @param noutput_items
          * @param ninput_items_required
          */
         void
         forecast(int noutput_items, gr_vector_int &ninput_items_required)
         {
            size_t ninput = ninput_items_required.size();

            if  (ninput != g_vradios.size())
               std::cout << "ERR: " << __FUNCTION__ << std::endl;


            for (size_t i = 0; i < ninput; ++i)
            {
               ninput_items_required[i] = get_vradio(i)->forecast(noutput_items);
            }
         }

         /**
          * @param vradio_id
          * @param bandwidth
          * @return Always 1
          */
         int
         set_vradio_subcarriers(size_t vradio_id, size_t bandwidth)
         {
            g_vradios[vradio_id]->set_subcarriers(bandwidth);
            return 1;
         }

         /**
          */
         void
         set_radio_mapping()
         {
            iq_map_vec sc_allocated(fft_m_len, 0);

            size_t idx(0);
            for (vradio_vec::iterator it = g_vradios.begin();
                  it != g_vradios.end();
                  ++it, ++idx)
            {
               size_t sc_needed = std::min((*it)->get_subcarriers(),
                               fft_m_len);

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

         /**
          */
         void
         demultiplex()
         {
            std::copy(g_rx_iq_vec.begin(),
                            g_rx_iq_vec.end(),
                            g_fft_complex->get_inbuf());

            g_fft_complex->execute();

            samples_vec rx_samples_freq(g_fft_complex->get_outbuf(),
                            g_fft_complex->get_outbuf() + fft_m_len);
         }

         /*
          * @param output_buff
          * @param max_noutput_items
          * @return 
          */
         size_t
         get_tx_outbuf(gr_complex *output_items, size_t max_noutput_items)
         {
            /* Return the vector with samples
             * @return Vector of samples to transmit in TIME domain.
             */

            // While we can generate samples to transmit
            size_t noutput_items = 0;
            while (tx_ready() && noutput_items < max_noutput_items)
            {
               // For each VirtualRadio call the map_iq_samples
               // func passing our buffer as parameter
               samples_vec samp_freq_vec(fft_m_len);
               for (vradio_vec::iterator it = g_vradios.begin();
                               it != g_vradios.end();
                               ++it)
               {
                       (*it)->map_iq_samples(samp_freq_vec);
               }
   
               // Transform buffer from FREQ domain to TIME domain using IFFT
               std::copy(samp_freq_vec.begin(), samp_freq_vec.end(),
                               g_ifft_complex->get_inbuf() );
               g_ifft_complex->execute();
   
               // Copy to GNURADIO buffer
               std::copy(g_ifft_complex->get_outbuf(),
                               &g_ifft_complex->get_outbuf()[fft_m_len],
                               &output_items[0]);

               noutput_items++;
               output_items += fft_m_len;
            }

            return noutput_items;
         }
      };

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
