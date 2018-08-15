#ifndef INCLUDED_HYDRA_UHD_INTERFACE_H
#define INCLUDED_HYDRA_UHD_INTERFACE_H

#include "hydra/types.h"
#include <uhd/usrp/multi_usrp.hpp>

namespace hydra {

class abstract_device
{
 public:
 abstract_device(){}

  virtual void send(const window &buf, size_t len) { std::cerr << __PRETTY_FUNCTION__ << " not implemented" << std::endl;};
  virtual void receive(window &buf, size_t len) { std::cerr << __PRETTY_FUNCTION__ << " not implemented" << std::endl;};

  virtual void set_tx_config(double freq, double rate, double gain){ g_tx_freq = freq; g_tx_rate = rate; g_tx_gain = gain;};
  virtual void set_rx_config(double freq, double rate, double gain){ g_rx_freq = freq; g_rx_rate = rate; g_rx_gain = gain;};

  virtual void release() {};

 protected:
  double g_tx_freq;
  double g_tx_rate;
  double g_tx_gain;
  double g_rx_freq;
  double g_rx_rate;
  double g_rx_gain;
};


class device_uhd: public abstract_device
{
public:
  device_uhd(std::string device_args = "");
  ~device_uhd();

  void send(const window &buf, size_t len);
  void receive(window &buf, size_t len);

  void set_tx_config(double freq, double rate, double gain);
  void set_rx_config(double freq, double rate, double gain);

  virtual void release();

private:
  uhd::usrp::multi_usrp::sptr usrp;

  uhd::rx_streamer::sptr rx_stream;
  uhd::tx_streamer::sptr tx_stream;

};


class device_image_gen: public abstract_device
{
public:
   device_image_gen(std::string device_args = "");
   void send(const window &buf, size_t len);
   void receive(window &buf, size_t len);

private:
   samples_vec g_iq_samples;
   std::string file_read;
};


} // namespace hydra

#endif
