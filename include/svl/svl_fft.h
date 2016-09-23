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

#ifndef INCLUDED_SVL_SVL_FFT_H
#define INCLUDED_SVL_SVL_FFT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/shared_ptr.hpp>
#include <fftw3.h>
#include <gnuradio/types.h>

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
		fft_complex(size_t fft_size, bool forward = true)
		{
	   	g_inbuf = new gr_complex[fft_size];
			g_outbuf = new gr_complex[fft_size];

			g_plan = fftwf_plan_dft_1d(fft_size,
								 reinterpret_cast<fftwf_complex *>(g_inbuf),
								 reinterpret_cast<fftwf_complex *>(g_outbuf),
								 forward? FFTW_FORWARD:FFTW_BACKWARD,
								 FFTW_MEASURE);


		}

		gr_complex* get_inbuf(){ return g_inbuf; }
		gr_complex* get_outbuf() { return g_outbuf; }

		void execute() { fftwf_execute(g_plan); }
};

/* TYPEDEFS for this class */
typedef boost::shared_ptr<gr::svl::fft_complex> sfft_complex;


} /* namespace svl */
} /* namespace gr */

#endif /* #ifndef INCLUDED_SVL_SVL_FFT_H */
