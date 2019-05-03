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

#ifndef INCLUDED_HYDRA_FFT_H
#define INCLUDED_HYDRA_FFT_H

#include <fftw3.h>
#include <hydra/types.h>
#include <boost/shared_ptr.hpp>

namespace hydra {

class fft_complex
{
 private:
  size_t g_fft_size;
  bool g_forward;
  iq_sample *g_inbuf;
  iq_sample *g_outbuf;
  fftwf_plan g_plan;

 public:
  /** CTOR
   * @param fft_size
   * @param fortward
   */
  fft_complex(size_t fft_size, bool forward = true);

  /**
   */
  int set_data(const iq_sample *data, size_t len);

  /**
   */
  iq_sample* get_inbuf();


  /**
   */
  iq_sample* get_outbuf();

  /**
   */
  void execute();
};

/* TYPEDEFS for this class */
typedef boost::shared_ptr<hydra::fft_complex> sfft_complex;


} /* namespace hydra */

#endif /* #ifndef INCLUDED_HYDRA_FFT_H */
