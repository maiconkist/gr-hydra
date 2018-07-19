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

#include <algorithm>
#include <functional>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <numeric>

namespace gr {
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
   g_fft_complex  = sfft_complex(new gr::hydra::fft_complex(rx_fft_len));
   g_ifft_complex = sfft_complex(new gr::hydra::fft_complex(tx_fft_len, false));
};

/**
 * @param _fft_n_len
 * @return New radio id
 */
size_t
Hypervisor::create_vradio(double cf, double bandwidth)
{
   size_t fft_n = bandwidth / (g_tx_bw / tx_fft_len);

   vradio_ptr vradio(new VirtualRadio(g_vradios.size(), this));
   g_vradios.push_back(vradio);

   // ID representing the radio;
   return g_vradios.size() - 1;
}

void
Hypervisor::attach_virtual_radio(VirtualRadioPtr vradio)
{
   // Check if VirtualRadio with same id is already attached
   auto vr = get_vradio(vradio->get_id());
   if (vr != nullptr)
      return;

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
   auto new_end = std::remove_if(g_vradios.begin(), g_vradios.end(),
                                 [radio_id](const auto & vr) {
                                    return vr->get_id() == radio_id; });

   g_vradios.erase(new_end, g_vradios.end());

   return true;
}

int
Hypervisor::notify(VirtualRadio &vr)
{
   iq_map_vec subcarriers_map = g_tx_subcarriers_map;
   std::replace(subcarriers_map.begin(),
                subcarriers_map.end(),
                vr.get_id(),
                -1);

   // enter 'if' in case of success
   if (set_radio_mapping(vr, subcarriers_map ) > 0)
   {
      //LOG(INFO) << "success";
      g_tx_subcarriers_map = subcarriers_map;
      return 1;
   }

   return -1;
}

void
Hypervisor::set_tx_resources(double cf, double bw, size_t fft)
{
   g_tx_cf = cf;
   g_tx_bw = bw;
   tx_fft_len = fft;
   g_tx_subcarriers_map = iq_map_vec(fft, -1);
   g_ifft_complex = sfft_complex(new gr::hydra::fft_complex(fft, false));

   g_tx_thread = std::make_unique<std::thread>(&Hypervisor::tx_run, this);
}

void
Hypervisor::tx_run()
{
   size_t g_tx_sleep_time = llrint(get_tx_fft() * 1e9 / get_tx_bandwidth());

#define groupl 500
#define bufferlen get_tx_fft() * groupl

   gr_complex *optr = new gr_complex [bufferlen];

   size_t the_shift = 0;
   size_t img_counter = 0;

   while (true)
   {
      std::this_thread::sleep_for(std::chrono::nanoseconds(g_tx_sleep_time));
      get_tx_window(optr + the_shift, get_tx_fft());

      the_shift += get_tx_fft();
      if (the_shift  == bufferlen )
      {

#if 0
         if (std::accumulate(optr , optr + the_shift, 0.0,
                             [](double t, const std::complex<float> &f){return t + std::abs(f);}))
         {
            cv::Mat img(groupl, get_tx_fft(),  CV_8UC3, cv::Scalar(0,0,0));
            cv::Mat image = img;


            float max = 0;
            for(int x=0; x< img.cols; x++)
            {
               for(int y=0;y<img.rows;y++)
               {
                  float val = std::abs(optr[x * groupl + y]);
                  if (val > max) max = val;
               }
            }

            for(int x=0; x< img.cols; x++)
            {
               for(int y=0;y<img.rows;y++)
               {
                  float val = std::abs(optr[x * groupl + y]);
                  cv::Vec3b color = image.at<cv::Vec3b>(cv::Point(x,y));
                  color.val[0] = uchar(val / max * 255.0);
                  color.val[1] = uchar(val / max * 255.0);
                  color.val[2] = uchar(val / max * 255.0);
                  image.at<cv::Vec3b>(cv::Point(x,y)) = color;
               }
            }
            cv::imwrite(std::string("./waterfall_" + std::to_string(img_counter++) + ".png"), img);
         }
#endif

         the_shift = 0;
      }
   }
}


void
Hypervisor::set_rx_resources(double cf, double bw, size_t fft)
{
   g_rx_cf = cf;
   g_rx_bw = bw;
   rx_fft_len = fft;
   g_fft_complex  = sfft_complex(new gr::hydra::fft_complex(rx_fft_len));
}

void
Hypervisor::set_radio_mapping()
{
   iq_map_vec subcarriers_map(tx_fft_len, -1);

   // for each virtual radio, to its mapping to subcarriers
   // ::TRICKY:: we dont stop if a virtual radio cannot be allocated
   for (vradio_vec::iterator it = g_vradios.begin();
        it != g_vradios.end();
        ++it)
     {
        std::cout << "setting map for VR " << (*it)->get_id() << std::endl;
        set_radio_mapping(*((*it).get()), subcarriers_map);
     }
     g_tx_subcarriers_map = subcarriers_map;
}

int
Hypervisor::set_radio_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map)
{
   double vr_bw = vr.get_tx_bandwidth();
   double vr_cf = vr.get_tx_central_frequency();
   double offset = (vr_cf - vr_bw/2.0) - (g_tx_cf - g_tx_bw/2.0) ;

   // First VR subcarrier
   int sc = offset / (g_tx_bw / tx_fft_len);
   size_t fft_n = vr_bw /(g_tx_bw /tx_fft_len);

   if (sc < 0 || sc > tx_fft_len) {
      std::cout << "Cannot allocate subcarriers for VR " << vr.get_id() << std::endl;
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
   vr.set_iq_mapping(the_map);

   return 1;
}

size_t
Hypervisor::get_tx_window(gr_complex *optr, size_t len)
{
   for (vradio_vec::iterator it = g_vradios.begin();
        it != g_vradios.end();
        ++it)
   {
      (*it)->map_tx_samples(g_ifft_complex->get_inbuf());

   }
   // Transform buffer from FREQ domain to TIME domain using IFFT

   g_ifft_complex->execute();

   std::copy(g_ifft_complex->get_outbuf(),
             g_ifft_complex->get_outbuf() + tx_fft_len,
             optr);

   return tx_fft_len;
}

} /* namespace hydra */
} /* namespace gr */
