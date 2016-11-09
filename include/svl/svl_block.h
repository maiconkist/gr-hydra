/* -*- c++ -*- */
/* 
 * Copyright 2016 Trinity Connect Centre.
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
#ifndef INCLUDED_SVL_SVL_BLOCK_H
#define INCLUDED_SVL_SVL_BLOCK_H

#include <gnuradio/block.h>

#include <svl/api.h>
#include <svl/svl_hypervisor.h>

namespace gr {
   namespace svl {

class SVL_API svl_block: virtual public gr::block
{
   protected:
      typedef boost::shared_ptr<Hypervisor> hypervisor_ptr;
      hypervisor_ptr g_hypervisor;

		size_t fft_m_len;

   public:
		svl_block(size_t n_ports,
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

      /**
       * @param _vradio_id
       * @param _fft_n_len
       */
      int set_vradio_subcarriers(size_t _vradio_id,
            size_t _fft_n_len)
      {
			return g_hypervisor->set_vradio_subcarriers(_vradio_id, _fft_n_len);
      }

		/**
		 * @param noutput_items
		 * @param ninput_items_required
		 */
		void forecast(int noutput_items,gr_vector_int &ninput_items_required)
		{
			return g_hypervisor->forecast(noutput_items,
					ninput_items_required);
		}
};

} // namespace svl
} // namespace gr

#endif /* INCLUDED_SVL_SVL-SINK_H */
