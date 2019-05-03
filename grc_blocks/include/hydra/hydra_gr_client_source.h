/* -*- c++ -*- */
/*
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifndef INCLUDED_HYDRA_CLIENT_SOURCE_H
#define INCLUDED_HYDRA_CLIENT_SOURCE_H

#include <hydra/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace hydra {

class HYDRA_API hydra_gr_client_source : virtual public gr::hier_block2
{
public:
  typedef boost::shared_ptr<hydra_gr_client_source> sptr;

  static sptr make(unsigned int       u_id,
                   const std::string &s_host,
                   unsigned int       u_port,
                   const std::string &s_group);

  virtual void start_client(double d_center_frequency,
                            double d_samp_rate,
                            size_t u_payload) = 0;
};

  } // namespace hydra
} // namespace gr

#endif /* INCLUDED_HYDRA_SOURCE_H */
