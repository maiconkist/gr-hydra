#ifndef INCLUDED_HYDRA_UHD_INTERFACE_H
#define INCLUDED_HYDRA_UHD_INTERFACE_H

#include "hydra/hydra_fft.h"
#include "hydra/hydra_log.h"
#include "hydra/types.h"

#include <uhd/usrp/multi_usrp.hpp>
#include <zmq.hpp>
#include <memory>
#include <mutex>

#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>
#include <uhd/usrp/usrp.h>
#include <opencv2/opencv.hpp>


namespace hydra {

class abstract_device
{
 public:
 abstract_device(){}

  virtual void send(const iq_window &buf, size_t len) { std::cerr << __PRETTY_FUNCTION__ << " not implemented" << std::endl;};
  virtual size_t receive(iq_window &buf, size_t len) { std::cerr << __PRETTY_FUNCTION__ << " not implemented" << std::endl;};

  virtual void set_tx_config(double freq, double rate, double gain)
  {
    // Check for invalid RF parameters
    if ((freq <= 0.0) or (rate <= 0.0) or (gain < 0.0) or (gain > 1.0))
    {
      std::cerr << "<uhd_interface> Invalid TX RF parameters: Freq=" << freq << "[Hz], Rate=" << rate <<"[Hz], Gain=" << gain << std::endl;
      exit(40);
    }
    // Otherwise, continue
    g_tx_freq = freq; g_tx_rate = rate; g_tx_gain = gain;
  };
  virtual void set_rx_config(double freq, double rate, double gain)
  {
    // Check for invalid RF parameters
    if ((freq <= 0.0) or (rate <= 0.0) or (gain < 0.0) or (gain > 1.0))
    {
      std::cerr << "<uhd_interface> Invalid RX RF parameters: Freq=" << freq << "[Hz], Rate=" << rate << "[Hz], Gain=" << gain << std::endl;
      exit(40);
    }
    // Otherwise, continue
    g_rx_freq = freq; g_rx_rate = rate; g_rx_gain = gain;
  };

  virtual void release() {};

 protected:
  double g_tx_freq;
  double g_tx_rate;
  double g_tx_gain;
  double g_rx_freq;
  double g_rx_rate;
  double g_rx_gain;

  hydra_log logger;
};


class device_uhd: public abstract_device
{
public:
  device_uhd(std::string device_args = "");
  ~device_uhd();

  void send(const iq_window &buf, size_t len);
  size_t receive(iq_window &buf, size_t len);

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
   void send(const iq_window &buf, size_t len);
   size_t receive(iq_window &buf, size_t len);

private:
   samples_vec g_iq_samples;
   std::string file_read;
};


class device_loopback: public abstract_device
{
public:
   device_loopback(std::string device_args = "");
   void send(const iq_window &buf, size_t len);
   size_t receive(iq_window &buf, size_t len);

private:
   std::mutex g_mutex;
   window_stream g_windows_vec;
};

class device_file: public abstract_device
{
public:
  // Contructor
  device_file(std::string file_name = "trace.fc32");

  // TX method
  void send(const iq_window &buf, size_t len);

  // RX method
  size_t receive(iq_window &buf, size_t len);

  // Configure the TX and RX chains
  void set_tx_config(double freq, double rate, double gain);
  void set_rx_config(double freq, double rate, double gain);

  // Release device
  void release ()
  {
    // Close the file stream
    input_file_stream.close();
    output_file_stream.close();
  };

private:
  // Filename
  std::string s_file_name;

  // Declare input and output file streams
  std::ifstream input_file_stream;
  std::ofstream output_file_stream;

  // Hold the sampling rates
  double tx_rate;
  double rx_rate;

};

class device_network: public abstract_device
{
public:
   device_network(std::string host_add, std::string remote_addr);
   void send(const iq_window &buf, size_t len);
   size_t receive(iq_window &buf, size_t len);

private:
   bool init_tx, init_rx;

   std::string g_host_addr, g_remote_addr;
   std::unique_ptr<zmq::socket_t> socket_tx, socket_rx;
};


} // namespace hydra

#endif
