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

	  samples_vec_vec g_source_samples;

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
     * @param noutput_items
     * @param ninput_items
     * @param input_items
     */
    void sink_add_samples(int noutput_items,
                          gr_vector_int &ninput_items,
                          gr_vector_const_void_star &input_items);

    /**
     * @param output_buff
     * @param max_noutput_items
     * @return 
     */
    size_t sink_outbuf(gr_vector_void_star &output_items, size_t max_noutput_items);

    /**
     */
    bool const sink_ready();

    /**
     * @param noutput_items
     * @param output_items
     * @param max_noutput_items
     */
    size_t source_add_samples(int nouput_items,
                              gr_vector_int &ninput_items,
                              gr_vector_const_void_star &input_items);

    /**
     * @param output_items
     */
    gr_vector_int get_source_outbuf(gr_vector_void_star &output_items);

    /**
     */
    bool const source_ready();
};

typedef boost::shared_ptr<Hypervisor> hypervisor_ptr;

} /* namespace hydra */
} /* namespace gr */

#endif /*  INCLUDED_SVL_SVL_HYPERVISOR_H */
