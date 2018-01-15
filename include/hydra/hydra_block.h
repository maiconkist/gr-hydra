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

#ifndef INCLUDED_HYDRA_HYDRA_BLOCK_H
#define INCLUDED_HYDRA_HYDRA_BLOCK_H

#include <gnuradio/block.h>

#include <hydra/api.h>
#include <hydra/types.h>
#include <hydra/hydra_hypervisor.h>

namespace gr {
   namespace hydra {

class HYDRA_API hydra_block: virtual public gr::block
{
 protected:
  hypervisor_ptr g_hypervisor;
  size_t fft_m_len;

 public:
  hydra_block(size_t n_ports,
              size_t _fft_m_len,
              double central_frequency,
              double bandwidth)
    {
      g_hypervisor = hypervisor_ptr(new Hypervisor(_fft_m_len, central_frequency, bandwidth));
    }

  /**
   * @param cf Central frequency
   * @param bandwidth Bandwidth
   */
  size_t create_vradio(double cf, double bandwidth) {
    return  g_hypervisor->create_vradio(cf, bandwidth);  
  }


  /** Return pointer to Hypervisor
   * ::NOTE:: I tried to return a hypervisor_ptr, but it did not worked when called from python using SWIG
   */
  Hypervisor * get_hypervisor()
  {
    return g_hypervisor.get();
  }

  int set_central_frequency(size_t vr_idx, float cf)
  {
    return g_hypervisor->get_vradio(vr_idx)->set_central_frequency(cf);
  }
};

} // namespace hydra
} // namespace gr

#endif /* INCLUDED_HYDRA_HYDRA_H */
