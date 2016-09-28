#ifndef INCLUDED_SVL_SVL_VIRTUAL_RADIO_H
#define INCLUDED_SVL_SVL_VIRTUAL_RADIO_H

#include <svl/api.h>
#include <svl/svl_fft.h>
#include <svl/types.h>

#include <vector>

namespace gr {
	namespace svl {

/**
 */
class SVL_API VirtualRadio
{
	private:
		size_t fft_n_len;
		size_t g_idx;

		samples_vec_vec g_tx_samples;
		samples_vec_vec g_rx_samples;

		sfft_complex g_fft_complex;
		sfft_complex g_ifft_complex;

		iq_map_vec g_iq_map;

	public:
		/** CTOR
		 * @param _idx
		 * @param _fft_n_len
		 */
		VirtualRadio(size_t _idx, size_t _fft_n_len);

		/**
		 * @return fft_n_len
		 */
		size_t const get_subcarriers() { return fft_n_len; }

		/**
		 * @param _fft_n_len
		 */
		void set_subcarriers(size_t _fft_n_len) { fft_n_len = _fft_n_len; }

		/** The number of IQ samples required to produced noutput_items output
		 * One output is a buffer with fft_n_items
		 * @param noutput_items Total of noutput_items required
		 * @return Total of IQ samples require to produce nouput_items
		 */
		int forecast(int noutput_items);

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
		void demap_iq_samples(const samples_vec &samples_buf);

		/** Copy rx samples in the buff to samples_buff
		 * @param samples_buff
		 * @param len
		 */
		void get_rx_samples(gr_complex *samples_buff, size_t len);

		/**
		 * @param samples_buf
		 */
		bool map_iq_samples(samples_vec &samples_buf);

		/**
		 */
		bool const ready_to_map_iq_samples();
};

/* TYPEDEFS for this class */
typedef boost::shared_ptr<VirtualRadio> vradio_ptr;
typedef std::vector<vradio_ptr> vradio_vec;


} /* namespace svl */
} /* namespace gr */


#endif /* ifndef INCLUDED_SVL_SVL_VIRTUAL_RADIO_H */
