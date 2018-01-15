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

#ifndef INCLUDED_HYDRA_SOURCE_IMPL_H
#define INCLUDED_HYDRA_SOURCE_IMPL_H

#include <hydra/api.h>
#include <hydra/hydra_block.h>

namespace gr {
   namespace hydra {

class HYDRA_API hydra_source: public hydra_block
{
 public:
  typedef boost::shared_ptr<hydra_source> hydra_source_ptr;

  /**
   * @param _n_ports
   * @param _fft_m_len
   * @param _fft_n_len
   */
  static hydra_source_ptr make(size_t _n_ports,
                               size_t _fft_m_len,
                               double central_frequency,
                               double bandwidth,
                               const std::vector< std::vector<double> > vradios_config);

  /** CTOR
   */
  hydra_source(size_t _n_inputs,
               size_t _fft_m_len,
               double central_frequency,
               double bandwidth,
               const std::vector< std::vector<double> > vradio_conf);

  /** DTOR
   */
  ~hydra_source();

  /**
   */
  virtual void forecast(int noutput_items, gr_vector_int &ninput_items_required);

  // Where all the action really happens
  int general_work(int noutput_items,
                   gr_vector_int &ninput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items);

};

} // namespace hydra
} // namespace gr

#endif /* INCLUDED_HYDRA_SOURCE_IMPL_H */
