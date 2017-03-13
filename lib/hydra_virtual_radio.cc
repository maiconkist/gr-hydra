#include <hydra/hydra_virtual_radio.h>
#include <hydra/hydra_hypervisor.h>

namespace gr {
   namespace hydra {

VirtualRadio::VirtualRadio(Hypervisor &hypervisor,
    size_t _idx,
    double central_frequency,
    double bandwidth,
    size_t _fft_n_len):
        g_hypervisor(hypervisor),
        g_idx(_idx),
        g_cf(central_frequency),
        g_bw(bandwidth),
        fft_n_len(_fft_n_len)
{
   g_fft_complex = sfft_complex(new gr::hydra::fft_complex(fft_n_len)) ;
   g_ifft_complex = sfft_complex(new gr::hydra::fft_complex(fft_n_len, false));
}

void
VirtualRadio::set_central_frequency(double cf)
{
   double old_cf = cf;
   g_cf = cf;

   int err = g_hypervisor.notify(*this);
   if (err < 0)
      g_cf = old_cf;
}

void
VirtualRadio::set_bandwidth(double bw)
{
   double old_bw = g_bw;
   g_bw = bw;

   int err = g_hypervisor.notify(*this);
   if (err < 0)
      g_bw = old_bw;
}

void
VirtualRadio::add_iq_sample(const gr_complex *samples, size_t len)
{
   g_tx_samples.insert(g_tx_samples.end(), &samples[0], &samples[len]);

   //LOG_IF(g_tx_samples.size() > 5000, INFO) << "VR " << g_idx << ": g_tx_samples.size() == " << g_tx_samples.size();
}

void
VirtualRadio::set_iq_mapping(const iq_map_vec &iq_map)
{
   //LOG_IF(iq_map.size() != fft_n_len, ERROR)  << "iq_map.size() != fft_n_len";
   g_iq_map = iq_map;
}

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

void
VirtualRadio::get_rx_samples(gr_complex *samples_buff, size_t len)
{
   //LOG_IF(g_rx_samples.front().size() != fft_n_len, ERROR) << "wrong number of samples";

   std::copy(g_rx_samples.front().begin(), g_rx_samples.front().end(),
         samples_buff);

   g_rx_samples.pop();
}

bool
VirtualRadio::map_iq_samples(gr_complex *samples_buf)
{
   //LOG_IF(!ready_to_map_iq_samples(), ERROR) << "No samples to map";
   if (!ready_to_map_iq_samples()) return false;


   // Copy samples in TIME domain to FFT buffer, execute FFT
   std::copy(g_tx_samples.begin(),
         g_tx_samples.begin() + fft_n_len,
         g_fft_complex->get_inbuf());

   // Delete samples from our buffer
   g_tx_samples.erase(g_tx_samples.begin(),
         g_tx_samples.begin() + fft_n_len);

   g_fft_complex->execute();

   gr_complex *outbuf = g_fft_complex->get_outbuf();

   // map samples in FREQ domain to samples_buff
   // perfors fft shift 
   size_t idx = 0;
   for (iq_map_vec::iterator it = g_iq_map.begin();
         it != g_iq_map.end();
         it++, idx++)
   {
      samples_buf[*it] = outbuf[(idx + (fft_n_len/2)) % fft_n_len] / float(fft_n_len); 
   }

   return true;
}


bool const
VirtualRadio::ready_to_map_iq_samples()
{
   if (g_tx_samples.size() < fft_n_len)
      return false;
   return true;
}

} /* namespace hydra */
} /* namespace gr */
