#include "svl/svl_hypervisor.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP;


namespace gr {
   namespace svl {

Hypervisor::Hypervisor(size_t _fft_m_len):
        fft_m_len(_fft_m_len)
{
   g_fft_complex = sfft_complex(new gr::svl::fft_complex(fft_m_len));
   g_ifft_complex = sfft_complex(new gr::svl::fft_complex(fft_m_len, false));
};


/**
 * @param _fft_n_len
 * @return New radio id
 */
size_t
Hypervisor::create_vradio(size_t _fft_n_len)
{
   vradio_ptr vradio(new VirtualRadio(g_vradios.size(), _fft_n_len));
   g_vradios.push_back(vradio);

   // ID representing the radio;
   return g_vradios.size() - 1;
};

/**
 * @param idx
 * @return vradio_ptr to VR
 */
vradio_ptr
Hypervisor::get_vradio(size_t idx)
{
   LOG_IF(idx > g_vradios.size(), WARNING) << "ERROR";
   return g_vradios[idx];
}

/**
 * @return true if can generate output
 */
bool const
Hypervisor::tx_ready()
{
   for (vradio_vec::iterator it = g_vradios.begin();
         it != g_vradios.end();
         ++it)
   {
      if (!(*it)->ready_to_map_iq_samples())
      {
         return false;
      }
   }
   return true;
}

/**
 * @param noutput_items
 * @param ninput_items_required
 */
void
Hypervisor::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{
   size_t ninput = ninput_items_required.size();

   LOG_IF(ninput != g_vradios.size(), ERROR);

   for (size_t i = 0; i < ninput; ++i)
   {
      ninput_items_required[i] = get_vradio(i)->forecast(noutput_items);
   }
}

/**
 */
void
Hypervisor::set_radio_mapping()
{
   iq_map_vec sc_allocated(fft_m_len, 0);

   size_t idx(0);
   for (vradio_vec::iterator it = g_vradios.begin();
         it != g_vradios.end();
         ++it, ++idx)
   {
      size_t sc_needed = std::min((*it)->get_subcarriers(),
                      fft_m_len);

      iq_map_vec the_map;

      for (size_t idx = 0; idx < fft_m_len; ++idx)
      {
         if (sc_allocated[idx] == 0)
         {
            the_map.push_back(idx);
            sc_allocated[idx] = 1;
         }

         if (the_map.size() == sc_needed)
            break;
      }

      (*it)->set_iq_mapping(the_map);
   }
}

/**
 */
void
Hypervisor::demultiplex()
{
   std::copy(g_rx_iq_vec.begin(),
                   g_rx_iq_vec.end(),
                   g_fft_complex->get_inbuf());

   g_fft_complex->execute();

   samples_vec rx_samples_freq(g_fft_complex->get_outbuf(),
                   g_fft_complex->get_outbuf() + fft_m_len);
}

/*
 * @param output_buff
 * @param max_noutput_items
 * @return 
 */
size_t
Hypervisor::get_tx_outbuf(gr_complex *output_items, size_t max_noutput_items)
{
   /* Return the vector with samples
    * @return Vector of samples to transmit in TIME domain.
    */

   // While we can generate samples to transmit
   size_t noutput_items = 0;
   while (tx_ready() && noutput_items < max_noutput_items)
   {
      // For each VirtualRadio call the map_iq_samples
      // func passing our buffer as parameter
      samples_vec samp_freq_vec(fft_m_len);
      for (vradio_vec::iterator it = g_vradios.begin();
                      it != g_vradios.end();
                      ++it)
      {
              (*it)->map_iq_samples(samp_freq_vec);
      }

      // Transform buffer from FREQ domain to TIME domain using IFFT
      std::copy(samp_freq_vec.begin(), samp_freq_vec.end(),
                      g_ifft_complex->get_inbuf() );
      g_ifft_complex->execute();

      // Copy to GNURADIO buffer
      std::copy(g_ifft_complex->get_outbuf(),
                      &g_ifft_complex->get_outbuf()[fft_m_len],
                      &output_items[0]);

      noutput_items++;
      output_items += fft_m_len;
   }

   return noutput_items;
}

} /* namespace svl */
} /* namespace gr */
