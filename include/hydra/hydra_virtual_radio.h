#ifndef INCLUDED_SVL_SVL_VIRTUAL_RADIO_H
#define INCLUDED_SVL_SVL_VIRTUAL_RADIO_H

#include <hydra/api.h>
#include <hydra/hydra_fft.h>
#include <hydra/types.h>

#include <vector>

namespace gr {
	namespace hydra {

class Hypervisor;

class SVL_API VirtualRadio
{
	private:
		size_t fft_n_len; // Subcarriers used by this VRadio
		int g_idx; // Radio unique ID
		double g_cf; // Central frequency
		double g_bw; // Bandwidth 

		samples_vec g_tx_samples;
		samples_vec_vec g_rx_samples;

		sfft_complex g_fft_complex;
		sfft_complex g_ifft_complex;

		iq_map_vec g_iq_map;

		// pointer to this VR hypervisor
		Hypervisor &g_hypervisor;

	public:
		/** CTOR
		 * @param hypervisor
		 * @param _idx
		 * @param central_frequency
		 * @param bandwidth
		 * @param _fft_n_len
		 */
		VirtualRadio(Hypervisor &hypervisor,
				size_t _idx,
				double central_frequency,
				double bandwidth,
				size_t _fft_n_len);

		/** Return VRadio unique ID
		 * @return VRadio ID
		 */
		int const get_id(){ return g_idx; }

		/**
		 * @return fft_n_len
		 */
		size_t const get_subcarriers() {
				  return fft_n_len;

		}

      void set_subcarriers(size_t n) {
         fft_n_len = n;
         g_fft_complex = sfft_complex(new gr::hydra::fft_complex(fft_n_len)) ;
         g_ifft_complex = sfft_complex(new gr::hydra::fft_complex(fft_n_len, false));
      }

		/**
		 * @return g_cf The central frequency
		 */
		double const get_central_frequency() {
				  return g_cf;
		}

		/**
		 * @return Bandwidth
		 */
		double const get_bandwidth() {
				  return g_bw;
		}

		/**
		 * @param cf Central frequency
		 */
		void set_central_frequency(double cf);

		/**
		 * @param bw
		 */
		void set_bandwidth(double bw);

		/** Added the buff samples to the VR tx queue.
		 *
		 * @param samples The samples  that must be added to the VR tx queue.
		 * @param len samples lenght.
		 */
		void add_iq_sample(const gr_complex *samples, size_t len);

		/**
		 * @param iq_map
		 */
		void set_iq_mapping(const iq_map_vec &iq_map);

		/** Get samples from samples_buf that are used by this virtual radio
		 * @param samples_buf
		 */
		void demap_iq_samples(const gr_complex *samples_buf);

		/** Copy rx samples in the buff to samples_buff
		 * @param samples_buff
		 * @param len
		 */
		void get_rx_samples(gr_complex *samples_buff, size_t len);

		/**
		 * @param samples_buf
		 */
		bool map_iq_samples(gr_complex *samples_buf);

		/**
		 */
		bool const ready_to_map_iq_samples();
};

/* TYPEDEFS for this class */
typedef boost::shared_ptr<VirtualRadio> vradio_ptr;
typedef std::vector<vradio_ptr> vradio_vec;

} /* namespace hydra */
} /* namespace gr */


#endif /* ifndef INCLUDED_SVL_SVL_VIRTUAL_RADIO_H */
