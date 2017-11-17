#include <hydra/hydra_virtual_radio.h>
#include <hydra/hydra_hypervisor.h>

#include <iostream>

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

int
VirtualRadio::set_central_frequency(double cf)
{
   double old_cf = cf;
   g_cf = cf;

   int err = g_hypervisor.notify(*this);
   if (err < 0)
      g_cf = old_cf;

   return err;
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
VirtualRadio::add_sink_sample(const gr_complex *samples, size_t len)
{
   g_tx_samples.insert(g_tx_samples.end(), &samples[0], &samples[len]);
}

void
VirtualRadio::set_iq_mapping(const iq_map_vec &iq_map)
{
   g_iq_map = iq_map;
}

void
VirtualRadio::demap_iq_samples(const gr_complex *samples_buf)
{
   // Copy the samples used by this radio
   for (size_t idx = 0; idx < fft_n_len; ++idx)
      g_ifft_complex->get_inbuf()[idx] = samples_buf[g_iq_map[idx]];

   g_ifft_complex->execute();

   // Append new samples
   g_rx_samples.insert(g_rx_samples.end(), g_ifft_complex->get_outbuf(),
              g_ifft_complex->get_outbuf() + fft_n_len);
}

size_t
VirtualRadio::get_source_samples(gr_complex *samples_buff)
{
   if (g_rx_samples.size() == 0) return 0;

   size_t len = g_rx_samples.size();
   std::copy(g_rx_samples.begin(), g_rx_samples.end(), samples_buff);
   g_rx_samples = samples_vec();

   return len;
}

bool
VirtualRadio::map_iq_samples(gr_complex *samples_buf)
{
   //LOG_IF(!ready_to_map_iq_samples(), ERROR) << "No samples to map";
   if (!ready_to_map_iq_samples()) return false;

   // Copy samples in TIME domain to FFT buffer, execute FFT
   g_fft_complex->set_data(&g_tx_samples[0], fft_n_len);

   g_fft_complex->execute();
   gr_complex *outbuf = g_fft_complex->get_outbuf();

   // map samples in FREQ domain to samples_buff
   // perfors fft shift 
   size_t idx = 0;
   for (iq_map_vec::iterator it = g_iq_map.begin();
         it != g_iq_map.end();
         ++it, ++idx)
   {
      samples_buf[*it] = outbuf[idx]; 
   }

   // Delete samples from our buffer
   g_tx_samples.erase(g_tx_samples.begin(),
         g_tx_samples.begin() + fft_n_len);

   return true;
}

bool const
VirtualRadio::ready_to_map_iq_samples()
{
   if (g_tx_samples.size() < fft_n_len)
      return false;
   return true;
}

bool const
VirtualRadio::ready_to_demap_iq_samples()
{
   return g_rx_samples.size() > 0;
}

} /* namespace hydra */
} /* namespace gr */
