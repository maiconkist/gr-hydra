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
#include <hydra/hydra_socket.h>
#include <hydra/hydra_buffer.hpp>
#include <hydra/hydra_resampler.hpp>
#include <hydra/hydra_hypervisor.h>
#include <hydra/hydra_fft.h>
#include <hydra/hydra_log.h>
#include <hydra/hydra_uhd_interface.h>
#include <hydra/hydra_stats.h>

#include <vector>
#include <atomic>
#include <mutex>
// #include <boost/format.hpp>

namespace hydra {

class VirtualRadio
{
  public:
    /** CTOR
     */
    VirtualRadio(size_t _idx, Hypervisor *hypervisor);
    ~VirtualRadio();

    void stop();

    int set_rx_chain(unsigned int u_rx_udp,
                    double d_rx_centre_freq,
                    double d_rx_bw,
                    const std::string &server_addr,
                    const std::string &remote_addr);

    int set_tx_chain(unsigned int u_tx_udp,
                    double d_tx_centre_freq,
                    double d_tx_bw,
                    const std::string &server_addr,
                    const std::string &remote_addr);

    /** Return VRadio unique ID
     * @return VRadio ID
     */
    int const get_id() {return g_idx;}

    bool const get_tx_enabled(){ auto lock = lock_state(); return g_tx_udp_port; };
    size_t const get_tx_udp_port(){ auto lock = lock_state(); return g_tx_udp_port; }
    size_t const get_tx_fft() { auto lock = lock_state(); return g_tx_fft_size; }
    double const get_tx_freq() { auto lock = lock_state(); return g_tx_cf; }
    double const get_tx_bandwidth() { auto lock = lock_state(); return g_tx_bw; }
    int set_tx_freq(double cf);
    void set_tx_bandwidth(double bw);
    int reconfigure_tx(double cf, double bw);
    void set_tx_mapping(const iq_map_vec &iq_map);
    size_t const set_tx_fft(size_t n);
    bool map_tx_samples(iq_sample *samples_buf); // called by the hypervisor


    bool const get_rx_enabled(){ auto lock = lock_state(); return g_rx_udp_port; };
    size_t const get_rx_udp_port(){ auto lock = lock_state(); return g_rx_udp_port; }
    size_t const get_rx_fft() { auto lock = lock_state(); return g_rx_fft_size; }
    double const get_rx_freq() { auto lock = lock_state(); return g_rx_cf; }
    double const get_rx_bandwidth() { auto lock = lock_state(); return g_rx_bw; }
    int set_rx_freq(double cf);
    void set_rx_bandwidth(double bw);
    int reconfigure_rx(double cf, double bw);
    void set_rx_mapping(const iq_map_vec &iq_map);
    size_t const set_rx_fft(size_t n);
    void demap_iq_samples(const iq_sample *samples_buf, size_t len); // called by the hypervisor

    /**
     */
    bool const ready_to_demap_iq_samples();

  private:
    std::unique_lock<std::recursive_mutex> lock_state() const
    {
      return std::unique_lock<std::recursive_mutex>(*state_mutex);
    }

    iq_map_vec g_rx_map;
    size_t g_rx_fft_size = 0; // Subcarriers used by this VRadio
    size_t g_rx_udp_port;
    std::atomic<bool> b_receiver;
    double g_rx_cf = 0;      // Central frequency
    double g_rx_bw = 0;      // Bandwidth
    samples_vec g_rx_samples;
    sfft_complex g_ifft_complex;
    zmq_sink_ptr rx_socket;
    std::unique_ptr<resampler<iq_window, iq_sample>> rx_resampler;
    std::shared_ptr<hydra_buffer<iq_window>> rx_windows;
    ReportPtr rx_report;

    iq_map_vec g_tx_map;
    size_t g_tx_fft_size = 0; // Subcarriers used by this VRadio
    size_t g_tx_udp_port;
    std::atomic<bool> b_transmitter;
    ReportPtr tx_report;
    double g_tx_cf = 0;      // Central frequency
    double g_tx_bw = 0;      // Bandwidth
    sfft_complex g_fft_complex;
    std::unique_ptr<resampler<iq_sample, iq_window>> tx_resampler;
    zmq_source_ptr tx_socket;

    int g_idx;        // Radio unique ID
    std::mutex g_mutex;
    std::shared_ptr<std::recursive_mutex> state_mutex;

    hydra_log logger;

    // pointer to this VR hypervisor
    Hypervisor *p_hypervisor;
};


} /* namespace hydra */


#endif /* ifndef INCLUDED_HYDRA_VIRTUAL_RADIO_H */
