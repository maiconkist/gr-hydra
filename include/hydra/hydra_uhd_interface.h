#ifndef INCLUDED_HYDRA_UHD_INTERFACE_H
#define INCLUDED_HYDRA_UHD_INTERFACE_H

#include <uhd.h>
#include "hydra/types.h"

namespace hydra {

class abstract_device
{
public:
   abstract_device(double freq, double rate, double gain):
      g_freq(freq), g_rate(rate), g_gain(gain)
   {}

   virtual void send(const gr_complex *buf, size_t len) = 0;

   virtual void set_frequency(double freq) { g_freq = freq; };
   virtual void set_rate(double rate) { g_rate = rate; };
   virtual void set_gain(double gain) { g_gain = gain; };

   double const get_frequency() { return g_freq; }
   double const get_rate() { return g_rate; }
   double const get_gain() { return g_gain; }

private:
   double g_freq;
   double g_rate;
   double g_gain;
};


class device_uhd: public abstract_device
{
public:
   device_uhd(double freq, double rate, double gain, std::string device_args = "");
   ~device_uhd();

   void send(const gr_complex *buf, size_t len);

   virtual void set_frequency(double frequency);
   virtual void set_rate(double rate);
   virtual void set_gain(double gain);

private:
   uhd_usrp_handle usrp;

   uhd_tx_streamer_handle tx_streamer;
   uhd_tx_metadata_handle tx_md;
   size_t tx_samps_per_buff;
};


class device_image_gen: public abstract_device
{
public:
   device_image_gen(double freq, double rate, double gain);
   void send(const gr_complex *buf, size_t len);

private:
   samples_vec g_iq_samples;
};


} // namespace hydra

#endif
