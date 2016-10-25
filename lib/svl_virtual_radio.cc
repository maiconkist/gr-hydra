#include <svl/svl_virtual_radio.h>

#include "easylogging++.h"
#include <volk/volk.h>

namespace gr {
   namespace svl {

VirtualRadio::VirtualRadio(size_t _idx, size_t _fft_n_len):
        fft_n_len(_fft_n_len), g_idx(_idx)
{
   g_fft_complex = sfft_complex(new gr::svl::fft_complex(fft_n_len)) ;
   g_ifft_complex = sfft_complex(new gr::svl::fft_complex(fft_n_len, false));
}

/**
 * The number of IQ samples required to produced noutput_items output
 * One output is a buffer with fft_n_items
 *
 * @param noutput_items Total of noutput_items required
 * @return Total of IQ samples require to produce nouput_items
 */
int
VirtualRadio::forecast(int noutput_items)
{
   return fft_n_len;
}

/** Added the buff samples to the VR tx queue.
 * \param samples The samples  that must be added to the VR tx queue.
 * \param len samples lenght.
 */
void
VirtualRadio::add_iq_sample(const gr_complex *samples, size_t len)
{
	g_tx_samples.insert(g_tx_samples.end(), &samples[0], &samples[len]);

#if 0
	if (g_idx == 0)
	{
		std::cout << "------------" << std::endl;
		for (int i = 0; i < fft_n_len; i++)
			std::cout << samples[i] << ",";
		std::cout << std::endl;
	}
#endif
}

/**
 * @param iq_map
 */
void
VirtualRadio::set_iq_mapping(const iq_map_vec &iq_map)
{
	LOG_IF(iq_map.size() != fft_n_len, ERROR)  << "iq_map.size() != fft_n_len";
   g_iq_map = iq_map;
}

/**
 * @param samples_buf FREQ domain samples
 */
void
VirtualRadio::demap_iq_samples(const gr_complex *samples_buf)
{
	samples_vec rx_samples_freq(fft_n_len);
	
	// Copy the samples used by this radio
	size_t idx(0);
	for (iq_map_vec::iterator it = g_iq_map.begin();
			it != g_iq_map.end();
			++it, ++idx)
	{
		rx_samples_freq[idx] = samples_buf[*it];  
	}

	// Transfer samples to fft_complex buff and perform fft
	std::copy(rx_samples_freq.begin(), rx_samples_freq.end(),
			g_ifft_complex->get_inbuf());
	g_ifft_complex->execute();

	g_rx_samples.push(samples_vec(g_ifft_complex->get_outbuf(),
				  g_ifft_complex->get_outbuf() + fft_n_len)
	);
}

/**
 */
void
VirtualRadio::get_rx_samples(gr_complex *samples_buff, size_t len)
{
	LOG_IF(g_rx_samples.front().size() != fft_n_len, ERROR) << "wrong number of samples";

	std::copy(g_rx_samples.front().begin(), g_rx_samples.front().end(),
			samples_buff);

	g_rx_samples.pop();
}

/**
*/
bool const
VirtualRadio::ready_to_map_iq_samples()
{
   if (g_tx_samples.size() < fft_n_len)
   	return false;
   return true;
}

bool
VirtualRadio::map_iq_samples(gr_complex *samples_buf)
{
   LOG_IF(!ready_to_map_iq_samples(), ERROR) << "No samples to map";
   if (!ready_to_map_iq_samples()) return false;

   // Copy samples in TIME domain to FFT buffer, execute FFT
   std::copy(g_tx_samples.begin(),
			g_tx_samples.begin() + fft_n_len,
			g_fft_complex->get_inbuf());

   g_fft_complex->execute();

	gr_complex *outbuf = g_fft_complex->get_outbuf();

   // map samples in FREQ domain to samples_buff
   size_t idx = 0;
   for (iq_map_vec::iterator it = g_iq_map.begin();
			it != g_iq_map.end();
         ++it, ++idx)
   {
		samples_buf[*it] = outbuf[idx] / float(fft_n_len); 
   }

	g_tx_samples.erase(g_tx_samples.begin(),
			g_tx_samples.begin() + fft_n_len);

   return true;
}

} /* namespace svl */
} /* namespace gr */
