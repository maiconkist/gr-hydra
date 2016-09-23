#ifndef INCLUDED_SVL_SVL_HYPERVISOR_H
#define INCLUDED_SVL_SVL_HYPERVISOR_H

#include "svl_virtual_radio.h"

#include <vector>

namespace gr {
   namespace svl {

class Hypervisor
{
   private:
      typedef std::vector<vradio_ptr> vradio_vec;

      vradio_vec g_vradios;


      size_t fft_m_len;
      samples_vec g_rx_iq_vec;

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
       * @return true if can generate output
       */
      bool const tx_ready();

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
      int set_vradio_subcarriers(size_t vradio_id, size_t bandwidth) {
         g_vradios[vradio_id]->set_subcarriers(bandwidth);
         return 1;
      }

        /**
        */
        void set_radio_mapping();

        /**
        */
        void demultiplex();

        /*
         * @param output_buff
         * @param max_noutput_items
         * @return 
         */
        size_t get_tx_outbuf(gr_complex *output_items, size_t max_noutput_items);
};

} /* namespace svl */
} /* namespace gr */


#endif
