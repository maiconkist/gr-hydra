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
 private:

  size_t tx_fft_len; // FFT M length
  double g_tx_cf; // Hypervisor central frequency
  double g_tx_bw; // Hypervisor bandwidth
  sfft_complex g_ifft_complex;
  std::unique_ptr<std::thread> g_tx_thread;
  iq_map_vec g_tx_subcarriers_map; // mapping of subcarriers
  uhd_hydra_sptr g_tx_dev;

  size_t rx_fft_len; // FFT M length
  double g_rx_cf; // Hypervisor central frequency
  double g_rx_bw; // Hypervisor bandwidth
  samples_vec g_source_samples;
  sfft_complex g_fft_complex;

  vradio_vec g_vradios;

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

  void set_tx_resources(uhd_hydra_sptr tx_dev, double cf, double bw, size_t fft_len);
  void set_tx_bandwidth(double bw){ g_tx_bw = bw; }
  void set_tx_central_frequency(double cf){ g_tx_cf = cf; }
  double const get_tx_central_frequency() { return g_tx_cf; }
  double const get_tx_bandwidth() { return g_tx_bw; }
  size_t const get_tx_fft() { return tx_fft_len; }

  void tx_run();

  void set_rx_resources(double cf, double bw, size_t fft_len);
  void set_rx_bandwidth(double bw){ g_rx_bw = bw; }
  void set_rx_central_frequency(double cf){ g_rx_cf = cf; }
  double const get_rx_central_frequency() { return g_rx_cf; }
  double const get_rx_bandwidth() { return g_rx_bw; }
  size_t const get_rx_fft() { return rx_fft_len; }

  /** Map all virtual radios to subcarriers. Reset all mapping.
   */
  void set_radio_mapping();

  /** Allocate subcarriers for Virtual Radio vr only
   * @return -1 if error, 0 otherwise
   */
  int set_radio_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map);

  /**
   */
 size_t get_tx_window(gr_complex *optr, size_t len);


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
};

}; /* namespace hydra */

#endif /*  INCLUDED_HYDRA_HYPERVISOR_H */
