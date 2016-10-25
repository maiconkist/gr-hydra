#include "svl/svl_hypervisor.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP;

#include <volk/volk.h>

namespace gr {
   namespace svl {

Hypervisor::Hypervisor(size_t _fft_m_len):
        fft_m_len(_fft_m_len)
{
   g_fft_complex  = sfft_complex(new gr::svl::fft_complex(fft_m_len));
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
 * @param noutput_items
 * @param ninput_items_required
 */
void
Hypervisor::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{
   size_t ninput = ninput_items_required.size();

   LOG_IF(ninput != g_vradios.size(), ERROR);

   for (size_t i = 0; i < ninput; ++i)
      ninput_items_required[i] = noutput_items;
}

void
Hypervisor::set_radio_mapping()
{
   iq_map_vec sc_allocated(fft_m_len, 0);

	// TODO:: subcarriers in the outerloop is faster
   size_t idx(0);
   for (vradio_vec::iterator it = g_vradios.begin();
         it != g_vradios.end();
         ++it)
   {
      size_t sc_needed = std::min((*it)->get_subcarriers(),
                      fft_m_len);

      iq_map_vec the_map;

      for (; idx < fft_m_len; ++idx)
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

      // Transform buffer from FREQ domain to TIME domain using IFFT
      g_ifft_complex->execute();

		// ::TRICKY:: change output_items pointer
      std::copy(g_ifft_complex->get_outbuf(),
				g_ifft_complex->get_outbuf() + fft_m_len,
			  	&optr[noutput_items]);

      noutput_items += fft_m_len;
   }

		std::cout << "----------" << std::endl;
		for (int i = 0; i < noutput_items; i++)
				  std::cout << optr[i] << ",";
		std::cout << std::endl;


	return noutput_items;
}

void 
Hypervisor::rx_add_samples(const gr_complex *samples, size_t len)
{
   // the total of if itens we have transfered so far
   size_t consumed = 0;

   if (0 == g_rx_samples.size())
      g_rx_samples.push(samples_vec());


	size_t old_size = g_rx_samples.size();

   // While we have samples to transfer
   while (consumed < len)
   {
      // If we filled the last samples_vec, create a new one
      if (g_rx_samples.back().size() == fft_m_len)
         g_rx_samples.push(samples_vec());

      size_t rest = std::min(len - consumed,
				fft_m_len - g_rx_samples.back().size());

		g_rx_samples.back().insert(g_rx_samples.back().end(),
				&samples[consumed], &samples[consumed + rest]);
   }

   LOG_IF(consumed > len, INFO) << "consumed > len";
}

bool const
Hypervisor::rx_ready()
{
   if (g_rx_samples.size() == 0 || (g_rx_samples.front().size() < fft_m_len))
   {
      return false;
   }

   return true;
}


size_t
Hypervisor::rx_outbuf(gr_complex *output_items, size_t max_noutput_items)
{
   size_t noutput_items = 0;

	while (rx_ready() && noutput_items < max_noutput_items)
	{
	   const samples_vec &samp_time_vec = g_rx_samples.front();
			g_rx_samples.pop();

		// Transform buffer from TIME domain to FREQ domain using FFT
		std::copy(samp_time_vec.begin(), samp_time_vec.end(),
				g_fft_complex->get_inbuf());

		g_fft_complex->execute();

		// Demap samples and fill buffer to forward to rx chain
		size_t idx = 0;
		for (vradio_vec::iterator it = g_vradios.begin();
				it != g_vradios.end();
				++it, ++idx)
		{
			(*it)->demap_iq_samples(g_fft_complex->get_outbuf());
			(*it)->get_rx_samples(&output_items[idx], fft_m_len);
		}

		noutput_items++;
	}

   return noutput_items;
}

} /* namespace svl */
} /* namespace gr */
