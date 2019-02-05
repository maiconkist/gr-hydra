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
#include "hydra/hydra_uhd_interface.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <mutex>

namespace hydra {


Hypervisor::Hypervisor()
{
}

Hypervisor::Hypervisor(size_t _fft_m_len,
      double central_frequency,
      double bandwidth):
        tx_fft_len(_fft_m_len),
        g_tx_cf(central_frequency),
        g_tx_bw(bandwidth),
        g_tx_subcarriers_map(_fft_m_len, -1)
{
   g_fft_complex  = sfft_complex(new fft_complex(rx_fft_len));
   g_ifft_complex = sfft_complex(new fft_complex(tx_fft_len, false));
};

size_t
Hypervisor::create_vradio(double cf, double bandwidth)
{
   size_t fft_n = bandwidth / (g_tx_bw / tx_fft_len);
   vradio_ptr vradio(new VirtualRadio(g_vradios.size(), this));


   std::lock_guard<std::mutex> _l(vradios_mtx);
   g_vradios.push_back(vradio);

   // ID representing the radio;
   return g_vradios.size() - 1;
}

void
Hypervisor::attach_virtual_radio(VirtualRadioPtr vradio)
{
  /* Check if VirtualRadio with same id is already attached */
   auto vr = get_vradio(vradio->get_id());
   if (vr != nullptr)
      return;

   std::lock_guard<std::mutex> _l(vradios_mtx);
   g_vradios.push_back(vradio);
}

VirtualRadioPtr
const Hypervisor::get_vradio(size_t id)
{
   auto it = std::find_if(g_vradios.begin(), g_vradios.end(),
                          [id](const VirtualRadioPtr obj) {return obj->get_id() == id;});

   if (it == g_vradios.end())
      return nullptr;

   return *it;
}

bool
Hypervisor::detach_virtual_radio(size_t radio_id)
{
  std::lock_guard<std::mutex> _l(vradios_mtx);

  auto new_end = std::remove_if(g_vradios.begin(), g_vradios.end(),
                                [radio_id](const auto & vr) {
                                  return vr->get_id() == radio_id; });

  g_vradios.erase(new_end, g_vradios.end());
  return true;
}

int
Hypervisor::notify(VirtualRadio &vr, Hypervisor::Notify set_maps)
{
  if (vr.get_tx_enabled() || (set_maps & Hypervisor::SET_TX_MAP))
  {
    iq_map_vec subcarriers_map = g_tx_subcarriers_map;
    std::replace(subcarriers_map.begin(),
                 subcarriers_map.end(),
                 vr.get_id(),
                 -1);

    // enter 'if' in case of success
    if (set_tx_mapping(vr, subcarriers_map ) > 0)
    {
      // LOG(INFO) << "success";
      g_tx_subcarriers_map = subcarriers_map;
    }
  }

  if (vr.get_rx_enabled() || (set_maps & Hypervisor::SET_RX_MAP))
  {
    iq_map_vec subcarriers_map = g_rx_subcarriers_map;
    std::replace(subcarriers_map.begin(),
                 subcarriers_map.end(),
                 vr.get_id(),
                 -1);

    // enter 'if' in case of success
    if (set_rx_mapping(vr, subcarriers_map ) > 0)
    {
      //LOG(INFO) << "success";
      g_rx_subcarriers_map = subcarriers_map;
    }
  }

  return 1;
}

void
Hypervisor::set_tx_resources(uhd_hydra_sptr tx_dev, double cf, double bw, size_t fft)
{
   g_tx_dev = tx_dev;
   g_tx_cf = cf;
   g_tx_bw = bw;
   tx_fft_len = fft;
   g_tx_subcarriers_map = iq_map_vec(fft, -1);
   g_ifft_complex = sfft_complex(new fft_complex(fft, false));


   g_tx_thread = std::make_unique<std::thread>(&Hypervisor::tx_run, this);
}

void
Hypervisor::tx_run()
{
  size_t g_tx_sleep_time = llrint(get_tx_fft() * 1e6 / get_tx_bandwidth());
  window optr(get_tx_fft());

  while (true)
  {
    get_tx_window(optr , get_tx_fft());
    g_tx_dev->send(optr, get_tx_fft());
    std::fill(optr.begin(), optr.end(), std::complex<float>(0,0));
  }
}

void
Hypervisor::set_tx_mapping()
{
   iq_map_vec subcarriers_map(tx_fft_len, -1);

   std::lock_guard<std::mutex> _l(vradios_mtx);
   for (vradio_vec::iterator it = g_vradios.begin();
        it != g_vradios.end();
        ++it)
     {
        std::cout << "TX setting map for VR " << (*it)->get_id() << std::endl;
        set_tx_mapping(*((*it).get()), subcarriers_map);
     }
     g_tx_subcarriers_map = subcarriers_map;
}

int
Hypervisor::set_tx_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map)
{
   double vr_bw = vr.get_tx_bandwidth();
   double vr_cf = vr.get_tx_freq();
   double offset = (vr_cf - vr_bw/2.0) - (g_tx_cf - g_tx_bw/2.0) ;

   // First VR subcarrier
   int sc = offset / (g_tx_bw / tx_fft_len);
   size_t fft_n = vr_bw /(g_tx_bw /tx_fft_len);

   if (sc < 0 || sc > tx_fft_len)
   {
     return -1;
   }

   std::cout << "VR " << vr.get_id() << ": CF @" << vr_cf << ", BW @" << vr_bw << ", Offset @" << offset << ", First SC @ " << sc << ". Last SC @" << sc + fft_n << std::endl;

   // Allocate subcarriers sequentially from sc
   iq_map_vec the_map;
   for (; sc < tx_fft_len; sc++)
   {
      //LOG_IF(subcarriers_map[sc] != -1, INFO) << "Subcarrier @" <<  sc << " already allocated";
      if (subcarriers_map[sc] != -1)
         return -1;

      the_map.push_back(sc);
      subcarriers_map[sc] = vr.get_id();

      // break when we allocated enough subcarriers
      if (the_map.size() == fft_n)
         break;
   }

   vr.set_tx_fft(fft_n);
   vr.set_tx_mapping(the_map);

   return 1;
}

size_t
Hypervisor::get_tx_window(window &optr, size_t len)
{
   if (g_vradios.size() == 0) return 0;

  {
    std::lock_guard<std::mutex> _l(vradios_mtx);

    for (vradio_vec::iterator it = g_vradios.begin();
         it != g_vradios.end();
         ++it)
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
  g_rx_dev = rx_dev;
  g_rx_cf = cf;
  g_rx_bw = bw;
  rx_fft_len = fft_len;
  g_rx_subcarriers_map = iq_map_vec(fft_len, -1);
  g_fft_complex = sfft_complex(new fft_complex(fft_len));

  g_rx_thread = std::make_unique<std::thread>(&Hypervisor::rx_run, this);
}

void
Hypervisor::rx_run()
{
  size_t g_rx_sleep_time = llrint(get_rx_fft() * 1e9 / get_rx_bandwidth());
  window optr(get_rx_fft());

  while (true)
  {
     if (g_rx_dev->receive(optr, get_rx_fft()))
     {
        forward_rx_window(optr, get_rx_fft());
     }
  }
}

void
Hypervisor::set_rx_mapping()
{
  iq_map_vec subcarriers_map(rx_fft_len, -1);

  // for each virtual radio, to its mapping to subcarriers
  // ::TRICKY:: we dont stop if a virtual radio cannot be allocated
  std::lock_guard<std::mutex> _l(vradios_mtx);
  for (vradio_vec::iterator it = g_vradios.begin();
       it != g_vradios.end();
       ++it)
    {
      std::cout << "RX setting map for VR " << (*it)->get_id() << std::endl;
      set_rx_mapping(*((*it).get()), subcarriers_map);
    }
  g_rx_subcarriers_map = subcarriers_map;
}

int
Hypervisor::set_rx_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map)
{
   double vr_bw = vr.get_rx_bandwidth();
   double vr_cf = vr.get_rx_freq();
   double offset = (vr_cf - vr_bw/2.0) - (g_rx_cf - g_rx_bw/2.0) ;

   // First VR subcarrier
   int sc = offset / (g_rx_bw / rx_fft_len);
   size_t fft_n = vr_bw /(g_rx_bw /rx_fft_len);


   if (sc < 0 || sc > rx_fft_len)
   {
      return -1;
   }

   double c_bw = fft_n*g_rx_bw/rx_fft_len;
   double c_cf = g_rx_cf - g_rx_bw/2 + (g_rx_bw/rx_fft_len) * (sc + (fft_n/2));

   std::cout << boost::format("RX Request VR BW: %1%, CF: %2% ") % vr_bw % vr_cf << std::endl;
   std::cout << boost::format("RX Actual  VR BW: %1%, CF: %2% ") % c_bw % c_cf << std::endl;

   // Allocate subcarriers sequentially from sc
   iq_map_vec the_map;
   for (; sc < rx_fft_len; sc++)
   {
      //LOG_IF(subcarriers_map[sc] != -1, INFO) << "Subcarrier @" <<  sc << " already allocated";
      if (subcarriers_map[sc] != -1)
         return -1;

      the_map.push_back(sc);
      subcarriers_map[sc] = vr.get_id();

      // break when we allocated enough subcarriers
      if (the_map.size() == fft_n)
         break;
   }

   vr.set_rx_fft(fft_n);
   vr.set_rx_mapping(the_map);

   return 1;
}

void
Hypervisor::forward_rx_window(window &buf, size_t len)
{
  if (g_vradios.size() == 0) return;

  g_fft_complex->set_data(&buf.front(), len);
  g_fft_complex->execute();

  std::lock_guard<std::mutex> _l(vradios_mtx);
  for (vradio_vec::iterator it = g_vradios.begin();
       it != g_vradios.end();
       ++it)
  {
    if ((*it)->get_rx_enabled())
      (*it)->demap_iq_samples(g_fft_complex->get_outbuf(), get_rx_fft());
  }
}

} /* namespace hydra */
