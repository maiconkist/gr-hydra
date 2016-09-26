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


#ifndef INCLUDED_SVL_SVL_BLOCK_H
#define INCLUDED_SVL_SVL_BLOCK_H

#include <svl/api.h>
#include <gnuradio/block.h>

namespace gr {
   namespace svl {

      /*!
       * \brief Multiplex and demultiplex multiple waveforms in the spectrum band
       * \ingroup svl
       */
      class SVL_API svl_block : virtual public gr::block
      {
         public:
            // gr::svl::sptr
            typedef boost::shared_ptr<svl_block> sptr;

            /*!
             * \brief Return a shared_ptr to a new instance of svl::svl_sink.
             *
             * To avoid accidental use of raw pointers, svl::svl_sink's
             * constructor is in a private implementation
             * class. svl::svl_sink::make is the public interface for
             * creating new instances.
             *
             * @param _n_ports Number of VRadios
             * @param _fft_m_len Size of the FFT M
             * @param _fft_n_len Vector with the size of FFT N for each VRadio. Format: [fft_n_for_vr1, fft_n_for_vr2, ..., fft_n_for_vrk]
             */
            static sptr make(size_t _n_ports,
                  size_t _fft_m_len,
                  const std::vector<int> _fft_n_len);

            /**
             * @param _fft_n_len
             */
            virtual size_t create_vradio(size_t _fft_n_len) = 0;

            /**
             * @param _vradio_id
             * @param _fft_n_len
             */
            virtual int set_vradio_subcarriers(size_t _vradio_id, size_t _fft_n_len) = 0;
      };
   } // namespace svl
} // namespace gr

#endif /* INCLUDED_SVL_SVL-SINK_H */
