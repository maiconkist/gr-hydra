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
#ifndef INCLUDED_HYDRA_HYPERVISOR_H
#define INCLUDED_HYDRA_HYPERVISOR_H

#include <hydra/types.h>
#include <hydra/hydra_fft.h>
#include <hydra/hydra_virtual_radio.h>

#include <vector>
#include <thread>


namespace hydra {

class Hypervisor
{
 public:
  Hypervisor();
  Hypervisor(size_t _fft_m_len,
             double central_frequency,
             double bandwidth);

  /**
   * @param cf VRadio central frequency
   * @param bandwidth VRadio bandwidth
   * @return New radio id
   */
  size_t create_vradio(double cf, double bandwidth);

  /**
   */
  void attach_virtual_radio(VirtualRadioPtr vr);
  bool detach_virtual_radio(size_t radio_id);

  /**
   * @param idx
   * @return vradio_ptr to VR
   */
  VirtualRadioPtr const get_vradio(size_t idx);

  /** Called by Virtual Radio instances to notify changes
   * @param vr
   * @return -1 if error, 0 otherwise
   */
  int notify(VirtualRadio &vr);

  double const get_tx_central_frequency() { return g_tx_cf; }
  double const get_tx_bandwidth() { return g_tx_bw; }
  size_t const get_tx_fft() { return tx_fft_len; }
  void set_tx_resources(uhd_hydra_sptr tx_dev, double cf, double bw, size_t fft_len);
  void set_tx_bandwidth(double bw){ g_tx_bw = bw; }
  void set_tx_central_frequency(double cf){ g_tx_cf = cf; }
  void set_tx_mapping();
  int set_tx_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map);
  void tx_run();
  size_t get_tx_window(window &optr, size_t len); // where the tx things happen


  double const get_rx_central_frequency() { return g_rx_cf; }
  double const get_rx_bandwidth() { return g_rx_bw; }
  size_t const get_rx_fft() { return rx_fft_len; }
  void set_rx_resources(uhd_hydra_sptr rx_dev, double cf, double bw, size_t fft_len);
  void set_rx_bandwidth(double bw){ g_rx_bw = bw; }
  void set_rx_central_frequency(double cf){ g_rx_cf = cf; }
  void set_rx_mapping();
  int set_rx_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map);
  void rx_run();
  void forward_rx_window(window &optr, size_t len); // where the rx things happen



#if 0
  /**
   * @param noutput_items
   * @param output_items
   * @param max_noutput_items
   */
  size_t source_add_samples(int nouput_items,
                            gr_vector_int &ninput_items,
                            gr_vector_const_void_star &input_items);

  /**
   * @param output_items
   */
  gr_vector_int get_source_outbuf(size_t noutput_items, gr_vector_void_star &output_items);
#endif

  /**
   */
  bool const source_ready();

private:


  // All TX structures
  size_t tx_fft_len; // FFT M length
  double g_tx_cf; // Hypervisor central frequency
  double g_tx_bw; // Hypervisor bandwidth
  sfft_complex g_ifft_complex;
  std::unique_ptr<std::thread> g_tx_thread;
  iq_map_vec g_tx_subcarriers_map; // mapping of subcarriers
  uhd_hydra_sptr g_tx_dev;


  // All RX structures
  size_t rx_fft_len; // FFT M length
  double g_rx_cf; // Hypervisor central frequency
  double g_rx_bw; // Hypervisor bandwidth
  sfft_complex g_fft_complex;
  std::unique_ptr<std::thread> g_rx_thread;
  iq_map_vec g_rx_subcarriers_map; // mapping of subcarriers
  uhd_hydra_sptr g_rx_dev;

  vradio_vec g_vradios;
};

}; /* namespace hydra */

#endif /*  INCLUDED_HYDRA_HYPERVISOR_H */
