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
#include <complex>


#define PRINT_DEBUG(txt) std::cout << #txt ": " << txt << std::endl;

namespace hydra {

  typedef enum {
    USRP,
    IMAGE_GEN,
  } uhd_mode;

  // Commonly used datatypes
  typedef std::complex<float> iq_sample;
  typedef std::vector<iq_sample> iq_window;

  // Legacy stream datatype, considering removal
  typedef std::deque<iq_sample> sample_stream;
  typedef std::deque<iq_window> window_stream;

  // Class names
  class abstract_device;
  class Hypervisor;
  class VirtualRadio;

  // Poiner definitions
  typedef std::shared_ptr<abstract_device> uhd_hydra_sptr;

  typedef std::shared_ptr<iq_sample[]> samples_ptr;
  typedef std::vector<iq_sample> samples_vec;
  typedef std::queue<samples_vec> samples_vec_vec;

  typedef std::vector<int> iq_map_vec;

  typedef std::shared_ptr<Hypervisor> HypervisorPtr;
  typedef std::shared_ptr<VirtualRadio> VirtualRadioPtr;

  typedef std::shared_ptr<VirtualRadio> vradio_ptr;
  typedef std::vector<vradio_ptr> vradio_vec;

} /* namespace hydra */

#endif /* ifndef INCLUDED_HYDRA_TYPES_H */
