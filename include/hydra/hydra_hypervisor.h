#ifndef INCLUDED_SVL_SVL_HYPERVISOR_H
#define INCLUDED_SVL_SVL_HYPERVISOR_H

#include <hydra/api.h>
#include <hydra/types.h>
#include <hydra/hydra_fft.h>
#include <hydra/hydra_virtual_radio.h>

#include <vector>
#include <boost/shared_ptr.hpp>

namespace gr {
   namespace hydra {

class SVL_API Hypervisor
{
   private:

      size_t fft_m_len; // FFT M length
		double g_cf; // Hypervisor central frequency
		double g_bw; // Hypervisor bandwidth

		samples_vec_vec g_rx_samples;

      sfft_complex g_fft_complex;
      sfft_complex g_ifft_complex;

      vradio_vec g_vradios;
		iq_map_vec g_subcarriers_map; // mapping of subcarriers

   public:
      Hypervisor(size_t _fft_m_len,
				double central_frequency,
				double bandwidth);

      /**
       * @param cf VRadio central frequency
		 * @param bandwidth VRadio bandwidth
       * @return New radio id
       */
      size_t create_vradio(double cf, double bandwidth);

      /**
       * @param idx
       * @return vradio_ptr to VR
       */
      VirtualRadio * const get_vradio(size_t idx);

      /** Get total number of subcarriers, i.e., FFT M
       * @return fft_m_len
       */
      size_t const get_total_subcarriers() { return fft_m_len; }

		/** Get total of subcarriers allocated
		 * @return Total of subcarriers allocated
		 */
      size_t const get_allocated_subcarriers();

      /** Overrida GNURadio virtual method.
       * @param noutput_items
       * @param ninput_items_required
       */
      void forecast(int noutput_items, gr_vector_int &ninput_items_required);

		/** Called by Virtual Radio instances to notify changes
		 * @param vr
		 * @return -1 if error, 0 otherwise
		 */
		int notify(VirtualRadio &vr);

		/**
		 * @param cf central frequency
		 */
		void set_central_frequency(double cf){ g_cf = cf; }

		/**
		 * @param bw The hypervisor bandwidth
		 */
		void set_bandwidth(double bw){ g_bw = bw; }

      /** Map all virtual radios to subcarriers. Reset all mapping.
       */
      void set_radio_mapping();

		/** Allocate subcarriers for Virtual Radio vr only
		 * @return -1 if error, 0 otherwise
		 */
		int set_radio_mapping(VirtualRadio &vr, iq_map_vec &subcarriers_map);

      /**
       */
      void tx_add_samples(int noutput_items,
				gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items);

      /**
       * @param output_buff
       * @param max_noutput_items
       * @return 
       */
      size_t tx_outbuf(gr_vector_void_star &output_items, size_t max_noutput_items);

      /**
       * @return true if can generate output
       */
      bool const tx_ready();


		/*
		 *
		 * 
		 * RX FUNCTIONS.
		 *
		 *
		 */

      /**
		 * @param samples
		 * @param len
       */
      void rx_add_samples(const gr_complex *samples, size_t len);

      /**
       * @param output_buff
       * @param max_noutput_items
       * @return 
       */
      size_t rx_outbuf(gr_complex *output_items,
            size_t max_noutput_items);

      /**
       */
      bool const rx_ready();
};

typedef boost::shared_ptr<Hypervisor> hypervisor_ptr;

} /* namespace hydra */
} /* namespace gr */

#endif /*  INCLUDED_SVL_SVL_HYPERVISOR_H */
