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

#include "hydra/hydra_virtual_radio.h"

namespace hydra {

VirtualRadio::VirtualRadio(size_t _idx, Hypervisor *hypervisor):
   g_idx(_idx),
   p_hypervisor(hypervisor),
   b_receiver(false),
   b_transmitter(false),
   g_rx_udp_port(0),
   g_tx_udp_port(0)
{
  logger = hydra_log("VR #" + std::to_string(g_idx));
}


int
VirtualRadio::set_rx_chain(unsigned int u_rx_udp,
                           double d_rx_freq,
                           double d_rx_bw,
                           const std::string &server_addr,
                           const std::string &remote_addr)
{
  // If already receiving
  if (b_receiver) { return 1; }

  // Set the VR RX UDP port
  g_rx_udp_port = u_rx_udp;
  g_rx_cf = d_rx_freq;
  g_rx_bw = d_rx_bw;

  g_rx_fft_size = p_hypervisor->get_rx_fft() * (d_rx_bw / p_hypervisor->get_rx_bandwidth());
  g_ifft_complex  = sfft_complex(new fft_complex(g_rx_fft_size, false));

  // TODO this must be shared with the hypervisor, or come from it
  rx_windows = std::make_shared<hydra_buffer<iq_window>>(1000);

  // Create new resampler
  rx_resampler = std::make_unique<resampler<iq_window, iq_sample>>(
      rx_windows,
      d_rx_bw,
      g_rx_fft_size);

  /* Create ZMQ transmitter */
  rx_socket = zmq_sink::make(
      rx_resampler->buffer(),
      server_addr,
      remote_addr,
      std::to_string(u_rx_udp));

  /* Always in the end. */
  p_hypervisor->notify(*this, Hypervisor::SET_RX_MAP);

  /* Toggle receiving flag */
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
                           const std::string &server_addr,
                           const std::string &remote_addr)
{
   // If already transmitting
   if (b_transmitter)
      // Return error
      return 1;

   // Set the VR TX UDP port
   g_tx_udp_port = u_tx_udp;
   g_tx_bw = d_tx_bw;
   g_tx_cf = d_tx_cf;

   g_tx_fft_size = p_hypervisor->get_tx_fft() * (d_tx_bw / p_hypervisor->get_tx_bandwidth());

   // Create ZMQ receiver
   tx_socket = zmq_source::make(
       server_addr,
       remote_addr,
       std::to_string(u_tx_udp),
       1000*g_tx_fft_size);

   // Create new resampler
   tx_resampler = std::make_unique<resampler<iq_sample, iq_window>>(
       tx_socket->buffer(),
       d_tx_bw,
       g_tx_fft_size);

   // create fft object
   g_fft_complex  = sfft_complex(new fft_complex(g_tx_fft_size));

   p_hypervisor->notify(*this, Hypervisor::SET_TX_MAP);

   // Toggle transmitting flag
   b_transmitter = true;

   // TODO Create TX reports object
   // tx_report = std::make_unique<xvl_report>(u_id, tx_socket->windows());

   // Successful return
   return 0;
}

int
VirtualRadio::set_tx_freq(double cf)
{
  if (cf == g_tx_cf) return 0;

   double old_cf = g_tx_cf;
   g_tx_cf = cf;

   int err = p_hypervisor->notify(*this, Hypervisor::SET_TX_MAP);
   if (err < 0)
      g_tx_cf = old_cf;

   return err;
}

void
VirtualRadio::set_tx_bandwidth(double bw)
{
  if (bw == g_tx_bw) return;


   double old_bw = g_tx_bw;
   g_tx_bw = bw;

   int err = p_hypervisor->notify(*this, Hypervisor::SET_TX_MAP);
   if (err < 0)
      g_tx_bw = old_bw;
}

void
VirtualRadio::set_tx_mapping(const iq_map_vec &iq_map)
{
  g_tx_map = iq_map;
}

bool
VirtualRadio::map_tx_samples(iq_sample *samples_buf)
{
  // If the transmitter chain was not defined
  if (not b_transmitter){return false;}

  // Try to get a window from the resampler
  iq_window buf  = tx_resampler->buffer()->read_one();

  // Return false if the window is empty
  if (buf.empty()){return false;}

  // Copy samples in TIME domain to FFT buffer, execute FFT
  g_fft_complex->set_data(&buf.front(), g_tx_fft_size);
  g_fft_complex->execute();
  iq_sample *outbuf = g_fft_complex->get_outbuf();

  // map samples in FREQ domain to samples_buff
  // perfors fft shift
  int idx = 0;
  for (auto it = g_tx_map.begin(); it != g_tx_map.end(); ++it, ++idx)
  {
    samples_buf[*it] = outbuf[idx];
  }

  return true;
}

int
VirtualRadio::set_rx_freq(double cf)
{
  double old_cf = g_rx_cf;
  g_rx_cf = cf;

  int err = p_hypervisor->notify(*this);
  if (err < 0)
    g_rx_cf = old_cf;

  return err;
}

void
VirtualRadio::set_rx_bandwidth(double bw)
{
  double old_bw = g_rx_bw;
  g_rx_bw = bw;

  int err = p_hypervisor->notify(*this);
  if (err < 0)
    g_rx_bw = old_bw;
}

void
VirtualRadio::set_rx_mapping(const iq_map_vec &iq_map)
{
  g_rx_map = iq_map;
}

void
VirtualRadio::demap_iq_samples(const iq_sample *samples_buf, size_t len)
{
  // If the receiver chain was not defined
  if (not b_receiver){ return;}

  /* Copy the samples used by this radio */
  for (size_t idx = 0; idx < g_rx_fft_size; ++idx)
    g_ifft_complex->get_inbuf()[idx] = samples_buf[g_rx_map[idx]];

  // Perform the FFT operation
  g_ifft_complex->execute();

  /* Append new samples */
  rx_resampler->buffer()->write(g_ifft_complex->get_outbuf(), g_rx_fft_size);
}

} /* namespace hydra */
