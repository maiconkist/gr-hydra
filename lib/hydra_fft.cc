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

#include "hydra/hydra_fft.h"

#include <algorithm>
#include <mutex>
#include <new>
#include <limits>
#include <stdexcept>

namespace hydra {

namespace {
std::mutex planner_mutex;
}

fft_complex::fft_complex(size_t fft_size, bool forward):
   g_fft_size(fft_size),
   g_forward(forward)
{
   if (!fft_size || fft_size > size_t(std::numeric_limits<int>::max()) ||
       fft_size > std::numeric_limits<size_t>::max() / sizeof(fftw_complex))
      throw std::invalid_argument("FFT size must be a positive supported integer");
   g_inbuf.reset(static_cast<iq_sample*>(fftw_malloc(sizeof(fftw_complex) * fft_size)));
   g_outbuf.reset(static_cast<iq_sample*>(fftw_malloc(sizeof(fftw_complex) * fft_size)));
   if (!g_inbuf || !g_outbuf)
      throw std::bad_alloc();

   std::lock_guard<std::mutex> lock(planner_mutex);
   g_plan = fftwf_plan_dft_1d(fft_size,
                   reinterpret_cast<fftwf_complex*>(g_inbuf.get()),
                   reinterpret_cast<fftwf_complex*>(g_outbuf.get()),
                   forward? FFTW_FORWARD:FFTW_BACKWARD,
                   FFTW_MEASURE);
   if (!g_plan)
      throw std::runtime_error("Unable to create FFT plan");
}

fft_complex::~fft_complex()
{
   std::lock_guard<std::mutex> lock(planner_mutex);
   fftwf_destroy_plan(g_plan);
}

int
fft_complex::set_data(const iq_sample *data, size_t len)
{
   // Copy samples to fft buffer
   std::copy(data, data + len, g_inbuf.get());
   return 0;
}

void
fft_complex::reset_inbuf()
{
  std::fill_n(g_inbuf.get(), g_fft_size, 0);
}

void
fft_complex::reset_outbuf()
{
  std::fill_n(g_outbuf.get(), g_fft_size, 0);
}

iq_sample*
fft_complex::get_inbuf()
{
   return g_inbuf.get();
}

iq_sample*
fft_complex::get_outbuf()
{
   return g_outbuf.get();
}

void
fft_complex::execute()
{
   if (g_forward == false)
   {
      std::rotate(g_inbuf.get(),
            g_inbuf.get() + g_fft_size/2,
            g_inbuf.get() + g_fft_size);
   }

   fftwf_execute(g_plan);

   if (g_forward == true)
   {
      std::rotate(g_outbuf.get(),
            g_outbuf.get() + g_fft_size/2,
            g_outbuf.get() + g_fft_size);

      for (size_t i = 0; i < g_fft_size; ++i)
         g_outbuf[i] /= float(g_fft_size);
   }
}

} /* namespace hydra */
