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

#include "hydra/hydra_hypervisor.h"

#include <cmath>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace hydra {

namespace {

struct BinRange
{
  size_t first;
  size_t count;
};

std::optional<BinRange> bin_range(double cf, double bandwidth,
                                 double frontend_cf, double frontend_bandwidth,
                                 size_t fft_size)
{
  if (!fft_size || fft_size > size_t(std::numeric_limits<int>::max()) ||
      !std::isfinite(cf) || !std::isfinite(bandwidth) || bandwidth <= 0 ||
      !std::isfinite(frontend_cf) || !std::isfinite(frontend_bandwidth) ||
      frontend_bandwidth <= 0 || bandwidth > frontend_bandwidth)
    return std::nullopt;

  const double low = frontend_cf - frontend_bandwidth / 2;
  const double high = frontend_cf + frontend_bandwidth / 2;
  const double bin_width = frontend_bandwidth / fft_size;
  if (!std::isfinite(low) || !std::isfinite(high) || bin_width <= 0)
    return std::nullopt;
  const double first = (cf - bandwidth / 2 - low) / bin_width;
  const double count = fft_size * (bandwidth / frontend_bandwidth);
  if (!std::isfinite(first) || first < 0 || first >= fft_size ||
      cf + bandwidth / 2 > high || count < 1 || count > fft_size)
    return std::nullopt;

  BinRange range{size_t(first), size_t(count)};
  if (range.count > fft_size - range.first)
    return std::nullopt;
  return range;
}

bool can_map(double cf, double bandwidth, double frontend_cf,
             double frontend_bandwidth, size_t fft_size, int owner,
             const iq_map_vec& bins)
{
  const auto range = bin_range(cf, bandwidth, frontend_cf, frontend_bandwidth, fft_size);
  if (!range || bins.size() != fft_size) return false;
  return std::all_of(bins.begin() + range->first,
                     bins.begin() + range->first + range->count,
                     [owner](int id) { return id == -1 || id == owner; });
}

}

void
Hypervisor::validate_frontend(double cf, double bandwidth, size_t fft_size)
{
  if (!bin_range(cf, bandwidth, cf, bandwidth, fft_size))
    throw std::invalid_argument("Invalid front-end frequency, bandwidth, or FFT size");
}

size_t
Hypervisor::rx_fft_size_for(double cf, double bandwidth) const
{
  auto lock = lock_state();
  const auto range = bin_range(cf, bandwidth, g_rx_cf, g_rx_bw, rx_fft_len);
  return range ? range->count : 0;
}

size_t
Hypervisor::tx_fft_size_for(double cf, double bandwidth) const
{
  auto lock = lock_state();
  const auto range = bin_range(cf, bandwidth, g_tx_cf, g_tx_bw, tx_fft_len);
  return range ? range->count : 0;
}

Hypervisor::Hypervisor()
{
  logger = hydra_log("hypervisor");
}

Hypervisor::Hypervisor(size_t _fft_m_len,
      double central_frequency,
      double bandwidth):
        tx_fft_len(_fft_m_len),
        g_tx_cf(central_frequency),
        g_tx_bw(bandwidth),
        g_tx_subcarriers_map(_fft_m_len, -1),
        rx_fft_len(_fft_m_len),
        g_rx_cf(central_frequency),
        g_rx_bw(bandwidth),
        g_rx_subcarriers_map(_fft_m_len, -1)
{
   validate_frontend(central_frequency, bandwidth, _fft_m_len);
   g_fft_complex  = sfft_complex(new fft_complex(rx_fft_len));
   g_ifft_complex = sfft_complex(new fft_complex(tx_fft_len, false));

  // Set chains to undefined
  b_tx_chain = b_rx_chain = false;

  logger = hydra_log("hypervisor");
};

Hypervisor::~Hypervisor()
{
  stop();
}

void
Hypervisor::stop()
{
  std::lock_guard<std::mutex> stop_lock(stop_mutex);
  thr_tx_stop = true;
  thr_rx_stop = true;
  if (g_tx_thread && g_tx_thread->joinable()) g_tx_thread->join();
  if (g_rx_thread && g_rx_thread->joinable()) g_rx_thread->join();

  vradio_vec radios;
  {
   auto lock = lock_state();
   radios = g_vradios;
   b_tx_chain = false;
   b_rx_chain = false;
  }
  for (const auto& radio : radios)
   radio->stop();
}

size_t
Hypervisor::create_vradio(double cf, double bandwidth)
{
   auto lock = lock_state();
   if (!tx_fft_size_for(cf, bandwidth))
     throw std::invalid_argument("Invalid virtual radio bandwidth or frequency");
   vradio_ptr vradio(new VirtualRadio(g_vradios.size(), this));


   g_vradios.push_back(vradio);

   // ID representing the radio;
   return g_vradios.size() - 1;
}

void
Hypervisor::attach_virtual_radio(VirtualRadioPtr vradio)
{
   auto lock = lock_state();
  /* Check if VirtualRadio with same id is already attached */
   auto vr = get_vradio(vradio->get_id());
   if (vr != nullptr){return;}

   g_vradios.push_back(vradio);
}

VirtualRadioPtr
const Hypervisor::get_vradio(size_t id)
{
   auto lock = lock_state();
   auto it = std::find_if(g_vradios.begin(), g_vradios.end(),
                          [id](const VirtualRadioPtr obj) {return obj->get_id() == id;});

   if (it == g_vradios.end())
      return nullptr;

   return *it;
}

bool
Hypervisor::detach_virtual_radio(size_t radio_id)
{
  VirtualRadioPtr vr;
  auto lock = lock_state();
  // Get pointer to the virtual radios
  vr = get_vradio(radio_id);
  if (vr == nullptr)
    return false;

  if (vr->get_tx_enabled())
  {
    // Temporary copy of the subcarrier map
    iq_map_vec subcarriers_map = g_tx_subcarriers_map;
    // Free mapping
    std::replace(subcarriers_map.begin(), subcarriers_map.end(),
        static_cast<int>(radio_id), -1);

    // Update the mapping
    g_tx_subcarriers_map = subcarriers_map;
  }
  if (vr->get_rx_enabled())
  {
    // Temporary copy of the subcarrier map
    iq_map_vec subcarriers_map = g_rx_subcarriers_map;
    // Free mapping
    std::replace(subcarriers_map.begin(), subcarriers_map.end(),
        static_cast<int>(radio_id), -1);

    // Update the mapping
    g_rx_subcarriers_map = subcarriers_map;
  }

  // Erase-remove idiom
  g_vradios.erase(
      std::remove(g_vradios.begin(), g_vradios.end(), vr),
      g_vradios.end());

  lock.unlock();
  vr->stop();
  return true;
}

int
Hypervisor::notify(VirtualRadio &vr, Hypervisor::Notify set_maps)
{
  auto lock = lock_state();
  const bool update_tx = set_maps == SET_NONE ? vr.get_tx_enabled() : set_maps & SET_TX_MAP;
  const bool update_rx = set_maps == SET_NONE ? vr.get_rx_enabled() : set_maps & SET_RX_MAP;
  if ((update_tx && !can_map(vr.get_tx_freq(), vr.get_tx_bandwidth(),
                            g_tx_cf, g_tx_bw, tx_fft_len, vr.get_id(), g_tx_subcarriers_map)) ||
      (update_rx && !can_map(vr.get_rx_freq(), vr.get_rx_bandwidth(),
                            g_rx_cf, g_rx_bw, rx_fft_len, vr.get_id(), g_rx_subcarriers_map)))
    return -1;

  auto tx_map = g_tx_subcarriers_map;
  auto rx_map = g_rx_subcarriers_map;
  if (update_tx)
  {
    std::replace(tx_map.begin(), tx_map.end(), vr.get_id(), -1);
    if (set_tx_mapping(vr, tx_map) < 0) return -1;
  }
  if (update_rx)
  {
    std::replace(rx_map.begin(), rx_map.end(), vr.get_id(), -1);
    if (set_rx_mapping(vr, rx_map) < 0) return -1;
  }
  if (update_tx) g_tx_subcarriers_map.swap(tx_map);
  if (update_rx) g_rx_subcarriers_map.swap(rx_map);
  return 0;
}

void
Hypervisor::set_tx_resources(uhd_hydra_sptr tx_dev, double cf, double bw, size_t fft)
{
   validate_frontend(cf, bw, fft);
   std::lock_guard<std::mutex> lifecycle_lock(stop_mutex);
   thr_tx_stop = true;
   if (g_tx_thread && g_tx_thread->joinable()) g_tx_thread->join();
   auto lock = lock_state();
   g_tx_dev = tx_dev;
   g_tx_cf = cf;
   g_tx_bw = bw;
   tx_fft_len = fft;
   g_tx_subcarriers_map = iq_map_vec(fft, -1);
   g_ifft_complex = sfft_complex(new fft_complex(fft, false));

   // Thread stop flag
   thr_tx_stop = false;
   // Toggle TX chain flag
   b_tx_chain = true;

   g_tx_thread = std::make_unique<std::thread>(&Hypervisor::tx_run, this);
}

void
Hypervisor::tx_run()
{
  size_t g_tx_sleep_time = llrint(get_tx_fft() * 1e6 / get_tx_bandwidth());
  iq_window optr(get_tx_fft());

  // Even loop
  while (not thr_tx_stop)
  {
    get_tx_window(optr , get_tx_fft());
    g_tx_dev->send(optr, get_tx_fft());
    std::fill(optr.begin(), optr.end(), std::complex<float>(0,0));
  }

  // Print debug message
  // logger.info("Stopped hypervisor's transmitter chain");
}

void
Hypervisor::set_tx_mapping()
{
   auto lock = lock_state();
   iq_map_vec subcarriers_map(tx_fft_len, -1);

   for (vradio_vec::iterator it = g_vradios.begin();
        it != g_vradios.end();
        ++it)
     {
        logger.debug("TX setting map for VR " + std::to_string((*it)->get_id()));
        set_tx_mapping(*((*it).get()), subcarriers_map);
     }
     g_tx_subcarriers_map = subcarriers_map;
}

int
Hypervisor::set_tx_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map)
{
   auto lock = lock_state();
   double vr_bw = vr.get_tx_bandwidth();
   double vr_cf = vr.get_tx_freq();
   double offset = (vr_cf - vr_bw/2.0) - (g_tx_cf - g_tx_bw/2.0) ;

   const auto range = bin_range(vr_cf, vr_bw, g_tx_cf, g_tx_bw, tx_fft_len);
   if (!range || subcarriers_map.size() != tx_fft_len) return -1;
   int sc = int(range->first);
   size_t fft_n = range->count;
   double c_bw = fft_n*g_tx_bw/tx_fft_len;
   double c_cf = g_tx_cf - g_tx_bw/2 + (g_tx_bw/tx_fft_len) * (sc + (fft_n/2));

   logger.debug(str(boost::format("TX Request VR BW: %1%, CF: %2% ") % vr_bw % vr_cf));
   logger.debug(str(boost::format("TX Actual  VR BW: %1%, CF: %2% ") % c_bw % c_cf));
   logger.info("Created vRF for #" + std::to_string(vr.get_id()) + ": CF @" +std::to_string(vr_cf) + ", BW @" + std::to_string(vr_bw) + ", Offset @" + std::to_string(offset) + ", First SC @ " + std::to_string(sc) + ". Last SC @" + std::to_string(sc + fft_n));

   if (!std::all_of(subcarriers_map.begin() + sc,
                     subcarriers_map.begin() + sc + fft_n,
                     [](int id) { return id == -1; }))
     return -1;
   iq_map_vec the_map(fft_n);
   std::iota(the_map.begin(), the_map.end(), sc);
   vr.set_tx_fft(fft_n);
   vr.set_tx_mapping(the_map);
   for (const auto bin : the_map) subcarriers_map[bin] = vr.get_id();

   return 1;
}

size_t
Hypervisor::get_tx_window(iq_window &optr, size_t len)
{
  auto lock = lock_state();
  // If there's nothing to transmit, return 0
  if (g_vradios.size() == 0){return 0;}

  {
    g_ifft_complex->reset_inbuf();

    std::fill(g_ifft_complex->get_inbuf(),
        g_ifft_complex->get_inbuf()+len,
        std::complex<float>(0,0));

    for (auto it = g_vradios.begin(); it != g_vradios.end(); ++it)
    {
      if ((*it)->get_tx_enabled())
        (*it)->map_tx_samples(g_ifft_complex->get_inbuf());
    }
  }

  g_ifft_complex->execute();

  optr.assign(g_ifft_complex->get_outbuf(),
              g_ifft_complex->get_outbuf() + len);

  return len;
}

void
Hypervisor::set_rx_resources(uhd_hydra_sptr rx_dev, double cf, double bw, size_t fft_len)
{
  validate_frontend(cf, bw, fft_len);
  std::lock_guard<std::mutex> lifecycle_lock(stop_mutex);
  thr_rx_stop = true;
  if (g_rx_thread && g_rx_thread->joinable()) g_rx_thread->join();
  auto lock = lock_state();
  g_rx_dev = rx_dev;
  g_rx_cf = cf;
  g_rx_bw = bw;
  rx_fft_len = fft_len;
  g_rx_subcarriers_map = iq_map_vec(fft_len, -1);
  g_fft_complex = sfft_complex(new fft_complex(fft_len));

  // Thread stop flag
  thr_rx_stop = false;
  // Toggle RX chain flag
  b_rx_chain = true;

  g_rx_thread = std::make_unique<std::thread>(&Hypervisor::rx_run, this);
}

void
Hypervisor::rx_run()
{
  const auto fft_size = get_rx_fft();
  iq_window incoming(fft_size);
  iq_window window(fft_size);
  size_t filled = 0;

  while (not thr_rx_stop)
  {
    const auto requested = fft_size - filled;
    const auto count = g_rx_dev->receive(incoming, requested);
    if (count > requested || count > incoming.size())
    {
      logger.error("Backend returned more samples than requested");
      filled = 0;
      continue;
    }
    if (!count)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    std::copy_n(incoming.begin(), count, window.begin() + filled);
    filled += count;
    if (filled == fft_size)
    {
      forward_rx_window(window, fft_size);
      filled = 0;
    }
  }

  // Print debug message
  // logger.info("Stopped hypervisor's receiver chain");
}

void
Hypervisor::set_rx_mapping()
{
  auto lock = lock_state();
  iq_map_vec subcarriers_map(rx_fft_len, -1);

  // for each virtual radio, to its mapping to subcarriers
  // ::TRICKY:: we dont stop if a virtual radio cannot be allocated
  for (vradio_vec::iterator it = g_vradios.begin();
       it != g_vradios.end();
       ++it)
    {
      logger.debug("RX setting map for VR " + std::to_string((*it)->get_id()));
      set_rx_mapping(*((*it).get()), subcarriers_map);
    }
  g_rx_subcarriers_map = subcarriers_map;
}

int
Hypervisor::set_rx_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map)
{
   auto lock = lock_state();
   double vr_bw = vr.get_rx_bandwidth();
   double vr_cf = vr.get_rx_freq();
   double offset = (vr_cf - vr_bw/2.0) - (g_rx_cf - g_rx_bw/2.0) ;

   const auto range = bin_range(vr_cf, vr_bw, g_rx_cf, g_rx_bw, rx_fft_len);
   if (!range || subcarriers_map.size() != rx_fft_len) return -1;
   int sc = int(range->first);
   size_t fft_n = range->count;

   // TODO what this means?
   double c_bw = fft_n*g_rx_bw/rx_fft_len;
   double c_cf = g_rx_cf - g_rx_bw/2 + (g_rx_bw/rx_fft_len) * (sc + (fft_n/2));

   logger.debug(str(boost::format("RX Request VR BW: %1%, CF: %2% ") % vr_bw % vr_cf));
   logger.debug(str(boost::format("RX Actual  VR BW: %1%, CF: %2% ") % c_bw % c_cf));
   logger.info("Created vRF for #" + std::to_string(vr.get_id()) + ": CF @" +std::to_string(vr_cf) + ", BW @" + std::to_string(vr_bw) + ", Offset @" + std::to_string(offset) + ", First SC @ " + std::to_string(sc) + ". Last SC @" + std::to_string(sc + fft_n));

   if (!std::all_of(subcarriers_map.begin() + sc,
                     subcarriers_map.begin() + sc + fft_n,
                     [](int id) { return id == -1; }))
     return -1;
   iq_map_vec the_map(fft_n);
   std::iota(the_map.begin(), the_map.end(), sc);
   vr.set_rx_fft(fft_n);
   vr.set_rx_mapping(the_map);
   for (const auto bin : the_map) subcarriers_map[bin] = vr.get_id();

   return 1;
}

void
Hypervisor::forward_rx_window(iq_window &buf, size_t len)
{
  auto lock = lock_state();
  if (len != rx_fft_len || len > buf.size()) return;
  if (g_vradios.size() == 0) return;

    g_fft_complex->set_data(&buf.front(), len);
    g_fft_complex->execute();

    for (vradio_vec::iterator it = g_vradios.begin();
         it != g_vradios.end();
         ++it)
      {
        if ((*it)->get_rx_enabled())
          (*it)->demap_iq_samples(g_fft_complex->get_outbuf(), get_rx_fft());
      }
}

} /* namespace hydra */
