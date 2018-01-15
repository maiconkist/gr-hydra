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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <hydra/api.h>

#include <fftw3.h>
#include <gnuradio/types.h>
#include <boost/shared_ptr.hpp>

namespace gr {
	namespace hydra {

class HYDRA_API fft_complex 
{
 private:
  size_t g_fft_size;
  bool g_forward;
  gr_complex *g_inbuf;
  gr_complex *g_outbuf;
  fftwf_plan g_plan;

 public:
  /** CTOR
   * @param fft_size
   * @param fortward
   */
  fft_complex(size_t fft_size, bool forward = true);

  /**
   */
  int set_data(const gr_complex *data, size_t len);

  /**
   */
  gr_complex* get_inbuf();


  /**
   */
  gr_complex* get_outbuf();

  /**
   */
  void execute();
};

/* TYPEDEFS for this class */
typedef boost::shared_ptr<gr::hydra::fft_complex> sfft_complex;


} /* namespace hydra */
} /* namespace gr */

#endif /* #ifndef INCLUDED_HYDRA_FFT_H */
