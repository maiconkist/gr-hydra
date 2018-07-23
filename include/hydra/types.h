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

#ifndef INCLUDED_HYDRA_TYPES_H
#define INCLUDED_HYDRA_TYPES_H

#include <vector>
#include <queue>
#include <memory>

#include <gnuradio/types.h>


namespace hydra {
    class abstract_device;
    class Hypervisor;
    class VirtualRadio;


  typedef std::complex<float> iq_sample;
  typedef std::deque<iq_sample> iq_stream;
  typedef std::vector<iq_sample> window;
  typedef std::deque<window> window_stream;

    typedef std::vector<std::complex<float> > iq_window;
    typedef std::shared_ptr<abstract_device> uhd_hydra_sptr;


    typedef std::shared_ptr<gr_complex[]> samples_ptr;
    typedef std::vector<gr_complex> samples_vec;
    typedef std::queue<samples_vec> samples_vec_vec;

    typedef std::vector<int> iq_map_vec;

    typedef std::shared_ptr<Hypervisor> HypervisorPtr;
    typedef std::shared_ptr<VirtualRadio> VirtualRadioPtr;
} /* namespace hydra */

#endif /* ifndef INCLUDED_HYDRA_TYPES_H */
