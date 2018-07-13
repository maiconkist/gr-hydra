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

VirtualRadio::VirtualRadio(size_t _idx):
   g_idx(_idx),
   b_receiver(false),
   b_transmitter(false)
{
}

int
VirtualRadio::set_rx_chain(unsigned int u_rx_udp,
                           double d_rx_freq,
                           double d_rx_bw,
                           unsigned int u_rx_fft,
                           bool b_pad)
{
  // If already receiving
  if (b_receiver)
  {
    // Return error
    return 1;
  }

  // Set the VR RX UDP port
  u_rx_udp_port = u_rx_udp;
  // Set the VR RX FFT size
  u_rx_fft_size = u_rx_fft;

  // Create UDP receiver
  rx_socket = std::make_unique<RxUDP>("0.0.0.0", std::to_string(u_rx_udp_port));
  // Create new timed buffer
  rx_buffer = std::make_unique<RxBuffer>(rx_socket->buffer(),
                                         rx_socket->mutex(),
                                         d_rx_bw,
                                         u_rx_fft_size,
                                         b_pad);

  // Get the pointer to the FFT window array
  rx_windows = rx_buffer->windows();
  // Toggle receiving flag
  b_receiver = true;

  g_ifft_complex = sfft_complex(new gr::hydra::fft_complex(u_rx_fft_size, false));

  // Create reports object
  rx_report = std::make_unique<xvl_report>(g_idx, rx_socket->buffer());

  // Successful return
  return 0;
}

int
VirtualRadio::set_tx_chain(unsigned int u_tx_udp,
                           double d_tx_centre_freq,
                           double d_tx_bw,
                           unsigned int u_tx_fft)
{
   // If already transmitting
   if (b_transmitter)
   {
      // Return error
      return 1;
   }
   // Set the VR TX UDP port
   u_tx_udp_port = u_tx_udp;
   // Set the VR TX FFT size
   u_tx_fft_size = u_tx_fft;

   // TODO this must be shared with the hypervisor, or come from it
   std::mutex * hyp_mutex = new std::mutex;
   tx_windows = new window_stream;
   // Create new TX timed buffer
   tx_buffer = std::make_unique<TxBuffer>(tx_windows,
                                          hyp_mutex,
                                          d_tx_bw,
                                          u_tx_fft_size);

   // Create UDP transmitter
   tx_socket = std::make_unique<TxUDP>(tx_buffer->stream(),
                                       tx_buffer->mutex(),
                                       "0.0.0.0",
                                       std::to_string(u_tx_udp_port));

   // Toggle transmitting flag
   b_transmitter = true;

   // TODO Create TX reports object
   // tx_report = std::make_unique<xvl_report>(u_id, tx_socket->windows());

   // Successful return
   return 0;
}

int
VirtualRadio::set_central_frequency(double cf)
{
   double old_cf = g_tx_cf;
   g_tx_cf = cf;

   int err = g_hypervisor->notify(*this);
   if (err < 0)
      g_tx_cf = old_cf;

   return err;
}

void
VirtualRadio::set_bandwidth(double bw)
{
   double old_bw = g_tx_bw;
   g_tx_bw = bw;

   int err = g_hypervisor->notify(*this);
   if (err < 0)
      g_tx_bw = old_bw;
}

void
VirtualRadio::add_sink_sample(const gr_complex *samples, size_t len)
{
   std::lock_guard<std::mutex> _l(g_mutex);
   g_tx_samples.insert(g_tx_samples.end(), samples, samples + len);
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
   for (size_t idx = 0; idx < u_tx_fft_size; ++idx)
      g_ifft_complex->get_inbuf()[idx] = samples_buf[g_iq_map[idx]];

   g_ifft_complex->execute();

   // Append new samples
   g_rx_samples.insert(g_rx_samples.end(), g_ifft_complex->get_outbuf(),
         g_ifft_complex->get_outbuf() + u_tx_fft_size);
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
   std::lock_guard<std::mutex> _l(g_mutex);

   if (!ready_to_map_iq_samples()) return false;

   // Copy samples in TIME domain to FFT buffer, execute FFT
   std::cout << "Entered VR " << g_idx << std::endl;
   g_fft_complex->set_data(&g_tx_samples[0], u_rx_fft_size);

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
   g_tx_samples.erase(g_tx_samples.begin(), g_tx_samples.begin() + u_tx_fft_size);

   std::cout << "Exit VR " << g_idx << std::endl;
   return true;
}

bool const
VirtualRadio::ready_to_map_iq_samples()
{
   if (g_tx_samples.size() < u_tx_fft_size)
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
