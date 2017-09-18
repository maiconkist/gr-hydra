#include "hydra/hydra_hypervisor.h"

#include <algorithm>
#include <iostream>

namespace gr {
   namespace hydra {

Hypervisor::Hypervisor(size_t _fft_m_len,
      double central_frequency,
      double bandwidth):
        fft_m_len(_fft_m_len),
        g_cf(central_frequency),
        g_bw(bandwidth),
        g_subcarriers_map(fft_m_len, -1)
{
		  g_fft_complex  = sfft_complex(new gr::hydra::fft_complex(fft_m_len));
		  g_ifft_complex = sfft_complex(new gr::hydra::fft_complex(fft_m_len, false));
};

/**
 * @param _fft_n_len
 * @return New radio id
 */
size_t
Hypervisor::create_vradio(double cf, double bandwidth)
{
		  size_t fft_n = bandwidth / (g_bw / fft_m_len);

		  vradio_ptr vradio(new VirtualRadio(*this, g_vradios.size(), cf, bandwidth, fft_n));
		  g_vradios.push_back(vradio);

		  //LOG(INFO) << "VR " << g_vradios.size() - 1 << "- FFT N: " << vradio->get_id() << ", CF: " << vradio->get_central_frequency() << ", BW: " << vradio->get_bandwidth();

		  // ID representing the radio;
		  return g_vradios.size() - 1;
};

int
Hypervisor::notify(VirtualRadio &vr)
{
   std::cout << "notify" << std::endl;;
   iq_map_vec subcarriers_map = g_subcarriers_map;
   std::replace(subcarriers_map.begin(),
                subcarriers_map.end(),
                vr.get_id(),
                -1);

   // enter 'if' in case of success
   if (set_radio_mapping(vr, subcarriers_map ) > 0)
   {
      //LOG(INFO) << "success";
      g_subcarriers_map = subcarriers_map;
      return 1;
   }

   return -1;
}

VirtualRadio * const
Hypervisor::get_vradio(size_t idx)
{
   //LOG_IF(idx > g_vradios.size(), WARNING) << "ERROR";
   return g_vradios[idx].get();
}

size_t const
Hypervisor::get_allocated_subcarriers()
{
   // Check if we can fit the subcarriers requested
   size_t total_subcarriers = 0;

   for (vradio_vec::iterator it = g_vradios.begin();
         it != g_vradios.end();
         ++it)
   {
      total_subcarriers += (*it)->get_subcarriers();
   }

   return total_subcarriers;
}


void
Hypervisor::forecast(int noutput_items,
      gr_vector_int &ninput_items_required)
{
   size_t ninput = ninput_items_required.size();

   //LOG_IF(ninput != g_vradios.size(), ERROR);

   int factor = noutput_items / fft_m_len;

   for (size_t i = 0; i < ninput; ++i)
	{
      ninput_items_required[i] = g_vradios[i]->get_subcarriers() * factor;
	}

}

void
Hypervisor::set_radio_mapping()
{
   iq_map_vec subcarriers_map(fft_m_len, -1);

   // for each virtual radio, to its mapping to subcarriers
   // ::TRICKY:: we dont stop if a virtual radio cannot be allocated
   for (vradio_vec::iterator it = g_vradios.begin();
        it != g_vradios.end();
        ++it)
     {
        std::cout << "setting map for VR " << (*it)->get_id() << std::endl;
        set_radio_mapping(*((*it).get()), subcarriers_map);
     }
     g_subcarriers_map = subcarriers_map;
}

int
Hypervisor::set_radio_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map)
{
   double vr_bw = vr.get_bandwidth();
   double vr_cf = vr.get_central_frequency();
   double offset = (vr_cf - vr_bw/2.0) - (g_cf - g_bw/2.0) ;

   // First VR subcarrier
   int sc = offset / (g_bw / fft_m_len);
   size_t fft_n = vr_bw /(g_bw /fft_m_len);

   //LOG_IF(sc < 0, ERROR) << "VR " << vr.get_id() << ": SC outside range - too low [" << offset << "]";
   //LOG_IF(sc > fft_m_len, ERROR) << "VR " << vr.get_id() << ": SC outside range - too high [" << offset << "]";

   if (sc < 0 || sc > fft_m_len) {
      std::cout << "Cannot allocate subcarriers for VR " << vr.get_id() << std::endl;
      return -1;
   }

   std::cout << "VR " << vr.get_id() << ": CF @" << vr_cf << ", BW @" << vr_bw << ", Offset @" << offset << ", First SC @ " << sc << ". Last SC @" << sc + fft_n << std::endl;

   // Allocate subcarriers sequentially from sc
   iq_map_vec the_map;
   for (; sc < fft_m_len; sc++)
   {
      //LOG_IF(subcarriers_map[sc] != -1, INFO) << "Subcarrier @" <<  sc << " already allocated";
      if (subcarriers_map[sc] != -1) return -1;

      the_map.push_back(sc);
      subcarriers_map[sc] = vr.get_id();

      // break when we allocated enough subcarriers
      if (the_map.size() == fft_n)
         break;
   }

   vr.set_subcarriers(fft_n);
   vr.set_iq_mapping(the_map);

   return 1;
}

void
Hypervisor::tx_add_samples(int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items)
{
   int factor = noutput_items / fft_m_len;

   for (size_t i = 0; i < ninput_items.size(); i++)
   {
      // For each input port
      // Get the input port buffer
      // Send the buffer to the correct virtual radio
      // TRICKY: Im assuming that input port 0 is mapped to Virtual Radio 0
      //         ........................... 1 .......................... 1
      //         ........................... 2 .......................... 2
      //         ........................... N .......................... N
      get_vradio(i)->add_iq_sample((const gr_complex *) input_items[i],
            get_vradio(i)->get_subcarriers() * factor);

      ninput_items[i] = get_vradio(i)->get_subcarriers() * factor;
   }
}

bool const
Hypervisor::tx_ready()
{
   for (vradio_vec::iterator it = g_vradios.begin();
         it != g_vradios.end();
         ++it)
   {
      if (!(*it)->ready_to_map_iq_samples())
         return false;
   }
   return true;
}


size_t
Hypervisor::tx_outbuf(gr_vector_void_star &output_items, size_t max_noutput_items)
{
   // While we can generate samples to transmit
   size_t noutput_items = 0;
   gr_complex *optr = (gr_complex *)output_items[0];

   // For each VirtualRadio call the map_iq_samples
   // func passing our buffer as parameter
   while (tx_ready() && noutput_items < max_noutput_items)
   {
      for (vradio_vec::iterator it = g_vradios.begin();
            it != g_vradios.end();
            ++it)
      {
         (*it)->map_iq_samples(g_ifft_complex->get_inbuf());
      }

      for (size_t i = 0; i < g_subcarriers_map.size(); ++i)
      {
        if (g_subcarriers_map[i] == -1)
        {
            g_ifft_complex->get_inbuf()[i] = gr_complex(0, 0);
        }
      }

      std::rotate(g_ifft_complex->get_inbuf(),
            g_ifft_complex->get_inbuf() + fft_m_len/2,
            g_ifft_complex->get_inbuf() + fft_m_len);

      // Transform buffer from FREQ domain to TIME domain using IFFT
      g_ifft_complex->execute();

      std::copy(g_ifft_complex->get_outbuf(),
            g_ifft_complex->get_outbuf() + fft_m_len,
            optr);

      optr += fft_m_len;
      noutput_items += fft_m_len;
   }

   return noutput_items;
}


} /* namespace hydra */
} /* namespace gr */
