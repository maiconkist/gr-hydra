#ifndef INCLUDED_SVL_SVL_HYPERVISOR_H
#define INCLUDED_SVL_SVL_HYPERVISOR_H

#include <svl/api.h>
#include <svl/svl_fft.h>
#include <svl/svl_virtual_radio.h>

#include <vector>

namespace gr {
   namespace svl {

class SVL_API Hypervisor
{
   private:
      vradio_vec g_vradios;

      size_t fft_m_len;
		samples_vec_vec g_rx_samples;

      sfft_complex g_fft_complex;
      sfft_complex g_ifft_complex;

   public:
      Hypervisor(size_t _fft_m_len);

      /**
       * @param _fft_n_len
       * @return New radio id
       */
      size_t create_vradio(size_t _fft_n_len);

      /**
       * @param idx
       * @return vradio_ptr to VR
       */
      vradio_ptr get_vradio(size_t idx);

      /**
       * @return fft_m_len
       */
      size_t const get_subcarriers() { return fft_m_len; }

      /**
       * @param noutput_items
       * @param ninput_items_required
       */
      void forecast(int noutput_items, gr_vector_int &ninput_items_required);

      /**
       * @param vradio_id
       * @param bandwidth
       * @return Always 1
       */
      int set_vradio_subcarriers(size_t vradio_id, size_t bandwidth)
		{
            g_vradios[vradio_id]->set_subcarriers(bandwidth);
            return 1;
      }

      /**
       */
      void set_radio_mapping();

      /**
       */
      void tx_add_samples(int noutput_items,
				gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items);

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
      size_t tx_outbuf(gr_vector_void_star &output_items, size_t max_noutput_items);

      /**
       * @param output_buff
       * @param max_noutput_items
       * @return 
       */
      size_t rx_outbuf(gr_complex *output_items,
            size_t max_noutput_items);


      /**
       * @return true if can generate output
       */
      bool const tx_ready();

      /**
       */
      bool const rx_ready();
      
};

} /* namespace svl */
} /* namespace gr */

#endif /*  INCLUDED_SVL_SVL_HYPERVISOR_H */
