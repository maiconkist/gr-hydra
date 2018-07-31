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

namespace hydra {

VirtualRadio::VirtualRadio(size_t _idx, Hypervisor *hypervisor):
   g_idx(_idx),
   p_hypervisor(hypervisor),
   b_receiver(false),
   b_transmitter(false)
{
}

int
VirtualRadio::set_rx_chain(unsigned int u_rx_udp,
                           double d_rx_freq,
                           double d_rx_bw)
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
  u_rx_fft_size = 1;


  // TODO this must be shared with the hypervisor, or come from it
  std::mutex * hyp_mutex = new std::mutex;
  rx_windows = new window_stream;
  // Create new TX timed buffer
  rx_buffer = std::make_unique<TxBuffer>(rx_windows,
                                         hyp_mutex,
                                         d_rx_bw,
                                         u_rx_fft_size);

  // Create UDP transmitter
  rx_socket = std::make_unique<TxUDP>(rx_buffer->stream(),
                                      rx_buffer->mutex(),
                                      "0.0.0.0",
                                      std::to_string(u_rx_udp_port));


  // Toggle receiving flag
  b_receiver = true;



  // Create reports object
  //rx_report = std::make_unique<xvl_report>(g_idx, rx_socket->buffer());

  // Successful return
  return 0;
}

int
VirtualRadio::set_tx_chain(unsigned int u_tx_udp,
                           double d_tx_cf,
                           double d_tx_bw,
                           bool b_pad)
{
   // If already transmitting
   if (b_transmitter)
      // Return error
      return 1;

   // Set the VR TX UDP port
   u_tx_udp_port = u_tx_udp;
   g_tx_bw = d_tx_bw;
   g_tx_cf = d_tx_cf;

   u_tx_fft_size = p_hypervisor->get_tx_fft() * (d_tx_bw / p_hypervisor->get_tx_bandwidth());

   // Create UDP receiver
   tx_socket = std::make_unique<RxUDP>("0.0.0.0", std::to_string(u_tx_udp_port));

   // Create new timed buffer
   tx_buffer = std::make_unique<RxBuffer>(tx_socket->buffer(),
                                          tx_socket->mutex(),
                                          d_tx_bw,
                                          u_tx_fft_size,
                                          b_pad);

   // create fft object
   g_fft_complex  = sfft_complex(new fft_complex(u_tx_fft_size));

   // Toggle transmitting flag
   b_transmitter = true;
   p_hypervisor->notify(*this);


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

   int err = p_hypervisor->notify(*this);
   if (err < 0)
      g_tx_cf = old_cf;

   return err;
}

void
VirtualRadio::set_bandwidth(double bw)
{
   double old_bw = g_tx_bw;
   g_tx_bw = bw;

   int err = p_hypervisor->notify(*this);
   if (err < 0)
      g_tx_bw = old_bw;
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
VirtualRadio::map_tx_samples(gr_complex *samples_buf)
{
   std::lock_guard<std::mutex> _l(g_mutex);

   const iq_window * buf = tx_buffer->consume();

   if (buf == nullptr)
   {
      return false;
   }

   const gr_complex *window = reinterpret_cast<const gr_complex*>(buf->data());

   // Copy samples in TIME domain to FFT buffer, execute FFT
   g_fft_complex->set_data(window, u_tx_fft_size);
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

   return true;
}

} /* namespace hydra */
