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

#ifndef INCLUDED_HYDRA_VIRTUAL_RADIO_H
#define INCLUDED_HYDRA_VIRTUAL_RADIO_H

#include <hydra/types.h>
#include <hydra/hydra_fft.h>
#include <hydra/hydra_socket.h>
#include <hydra/hydra_buffer.h>
#include <hydra/hydra_stats.h>

#include <vector>
#include <mutex>

namespace hydra {

class VirtualRadio
{
public:
  /** CTOR
   * @param _idx
   */
 VirtualRadio(size_t _idx, Hypervisor *hypervisor);

 int set_rx_chain(unsigned int u_rx_udp,
                  double d_rx_centre_freq,
                  double d_rx_bw);

 int set_tx_chain(unsigned int u_tx_udp,
                  double d_tx_centre_freq,
                  double d_tx_bw,
                  bool b_pad = false);

  /** Return VRadio unique ID
   * @return VRadio ID
   */
  int const get_id() {return g_idx;}

  bool const get_tx_enabled(){ return true; };
  size_t const get_tx_udp_port(){ return u_tx_udp_port; }
  size_t const get_tx_fft() {return u_rx_fft_size;}
  double const get_tx_central_frequency() { return g_tx_cf; }
  double const get_tx_bandwidth() {return g_tx_bw;}

  size_t const set_tx_fft(size_t n) {return u_tx_fft_size = n;}

  bool const get_rx_enabled(){ return true; };
  size_t const get_rx_udp_port(){ return u_rx_udp_port; }
  size_t const get_rx_fft() {return u_rx_fft_size;}
  double const get_rx_central_frequency() { return g_rx_cf; }
  double const get_rx_bandwidth() {return g_tx_bw;}


  /**
   * @param cf Central frequency
   */
  int set_central_frequency(double cf);

  /**
   * @param bw
   */
  void set_bandwidth(double bw);

  /**
   * @param iq_map
   */
  void set_iq_mapping(const iq_map_vec &iq_map);

  /** Get samples from samples_buf that are used by this virtual radio
   * @param samples_buf
   */
  void demap_iq_samples(const gr_complex *samples_buf);

  /** Copy rx samples in the buff to samples_buff
   * @param noutput_items
   * @param samples_buff
   * @return Number of samples mapped to samples_buff
   */
  size_t get_source_samples(size_t noutput_items, gr_complex *samples_buff);

  /**
   * @param samples_buf
   */
  bool map_tx_samples(gr_complex *samples_buf);

  /**
   */
  bool const ready_to_demap_iq_samples();

private:
  size_t u_rx_fft_size; // Subcarriers used by this VRadio
  size_t u_rx_udp_port;
  bool b_receiver;
  ReportPtr rx_report;
  double g_rx_cf;      // Central frequency
  double g_rx_bw;      // Bandwidth 
  
  samples_vec g_rx_samples;
  sfft_complex g_ifft_complex;
  TxUDPPtr rx_socket;
  TxBufferPtr rx_buffer;
  window_stream* rx_windows;

  size_t u_tx_fft_size; // Subcarriers used by this VRadio
  size_t u_tx_udp_port;
  bool b_transmitter;
  ReportPtr tx_report;
  double g_tx_cf;      // Central frequency
  double g_tx_bw;      // Bandwidth 
  sfft_complex g_fft_complex;
  RxBufferPtr tx_buffer;
  RxUDPPtr tx_socket;

  int g_idx;        // Radio unique ID
  iq_map_vec g_iq_map;
  std::mutex g_mutex;

  // pointer to this VR hypervisor
  Hypervisor *p_hypervisor;
};

/* TYPEDEFS for this class */
typedef std::shared_ptr<VirtualRadio> vradio_ptr;
typedef std::vector<vradio_ptr> vradio_vec;

} /* namespace hydra */


#endif /* ifndef INCLUDED_HYDRA_VIRTUAL_RADIO_H */
