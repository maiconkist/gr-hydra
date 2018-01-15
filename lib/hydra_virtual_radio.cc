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

#include <hydra/hydra_virtual_radio.h>
#include <hydra/hydra_hypervisor.h>

#include <iostream>

namespace gr {
   namespace hydra {

VirtualRadio::VirtualRadio(Hypervisor &hypervisor,
    size_t _idx,
    double central_frequency,
    double bandwidth,
    size_t _fft_n_len):
        g_hypervisor(hypervisor),
        g_idx(_idx),
        g_cf(central_frequency),
        g_bw(bandwidth),
        fft_n_len(_fft_n_len)
{
   g_fft_complex = sfft_complex(new gr::hydra::fft_complex(fft_n_len)) ;
   g_ifft_complex = sfft_complex(new gr::hydra::fft_complex(fft_n_len, false));
}

int
VirtualRadio::set_central_frequency(double cf)
{
   double old_cf = cf;
   g_cf = cf;

   int err = g_hypervisor.notify(*this);
   if (err < 0)
      g_cf = old_cf;

   return err;
}

void
VirtualRadio::set_bandwidth(double bw)
{
   double old_bw = g_bw;
   g_bw = bw;

   int err = g_hypervisor.notify(*this);
   if (err < 0)
      g_bw = old_bw;
}

void
VirtualRadio::add_sink_sample(const gr_complex *samples, size_t len)
{
   g_tx_samples.insert(g_tx_samples.end(), &samples[0], &samples[len]);
}

void
VirtualRadio::set_iq_mapping(const iq_map_vec &iq_map)
{
   g_iq_map = iq_map;
}

void
VirtualRadio::demap_iq_samples(const gr_complex *samples_buf)
{
   // Copy the samples used by this radio
   for (size_t idx = 0; idx < fft_n_len; ++idx)
      g_ifft_complex->get_inbuf()[idx] = samples_buf[g_iq_map[idx]];

   g_ifft_complex->execute();

   // Append new samples
   g_rx_samples.insert(g_rx_samples.end(), g_ifft_complex->get_outbuf(),
         g_ifft_complex->get_outbuf() + fft_n_len);
}

size_t
VirtualRadio::get_source_samples(size_t noutput_items, gr_complex *samples_buff)
{
   if (g_rx_samples.size() == 0) return 0;

   size_t len = std::min(g_rx_samples.size(), noutput_items);
   std::copy(g_rx_samples.begin(), g_rx_samples.begin() + len, samples_buff);
   g_rx_samples.erase(g_rx_samples.begin(), g_rx_samples.begin() + len);

   return len;
}

bool
VirtualRadio::map_iq_samples(gr_complex *samples_buf)
{
   if (!ready_to_map_iq_samples()) return false;

   // Copy samples in TIME domain to FFT buffer, execute FFT
   g_fft_complex->set_data(&g_tx_samples[0], fft_n_len);

   g_fft_complex->execute();
   gr_complex *outbuf = g_fft_complex->get_outbuf();

   // map samples in FREQ domain to samples_buff
   // perfors fft shift 
   size_t idx = 0;
   for (iq_map_vec::iterator it = g_iq_map.begin();
         it != g_iq_map.end();
         ++it, ++idx)
   {
      samples_buf[*it] = outbuf[idx]; 
   }

   // Delete samples from our buffer
   g_tx_samples.erase(g_tx_samples.begin(), g_tx_samples.begin() + fft_n_len);

   return true;
}

bool const
VirtualRadio::ready_to_map_iq_samples()
{
   if (g_tx_samples.size() < fft_n_len)
      return false;
   return true;
}

bool const
VirtualRadio::ready_to_demap_iq_samples()
{
   return g_rx_samples.size() > 0;
}

} /* namespace hydra */
} /* namespace gr */
