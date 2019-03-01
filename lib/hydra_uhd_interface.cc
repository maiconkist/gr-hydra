#include "hydra/hydra_uhd_interface.h"

#include "hydra/hydra_fft.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>
#include <uhd/usrp/usrp.h>
#include <opencv2/opencv.hpp>

namespace hydra {

device_uhd::device_uhd(std::string device_args)
{
   std::cout <<  "Creating USRP with args \"" <<  device_args << "\"" << std::endl;
   usrp = uhd::usrp::multi_usrp::make(device_args);
}


void
device_uhd::release()
{
  if (rx_stream != nullptr)
  {
    uhd::stream_cmd_t stream_cmd = uhd::stream_cmd_t(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    rx_stream->issue_stream_cmd(stream_cmd);
  }

  if (tx_stream != nullptr)
  {
    uhd::tx_metadata_t md;
    md.end_of_burst = true;
    tx_stream->send("", 0, md);
  }
}


device_uhd::~device_uhd()
{
}


void
device_uhd::set_tx_config(double freq, double rate, double gain)
{
  abstract_device::set_tx_config(freq, rate, gain);

  std::cout << "Setting TX Rate: " << rate << std::endl;
  usrp->set_tx_rate(rate);
  std::cout << "Actual TX Rate: " << usrp->get_tx_rate() << std::endl;

  std::cout << "Setting TX Gain: " << gain << std::endl;
  usrp->set_normalized_tx_gain(gain);
  std::cout << "Actual TX Gain: " << usrp->get_tx_gain() << std::endl;

  std::cout << "Setting TX freq: " << freq / 1e6 << " MHz" << std::endl;
  usrp->set_tx_freq(freq);
  std::cout << "Actual TX freq: " << usrp->get_tx_freq()/1e6 << " MHz" << std::endl;

  uhd::stream_args_t stream_args("fc32", "sc16");
  tx_stream = usrp->get_tx_stream(stream_args);
}

void
device_uhd::set_rx_config(double freq, double rate, double gain)
{
  abstract_device::set_rx_config(freq, rate, gain);

  std::cout << "Setting RX Rate: " << rate << std::endl;
  usrp->set_rx_rate(rate);
  std::cout << "Actual RX Rate: " << usrp->get_rx_rate() << std::endl;

  // no gain for reception.
  std::cout << "Setting RX Gain: " << gain << std::endl;
  usrp->set_normalized_rx_gain(0.0);
  std::cout << "Actual RX Gain: " << usrp->get_rx_gain() << std::endl;

  std::cout << "Setting RX freq: " << freq / 1e6 << " MHz" << std::endl;
  usrp->set_rx_freq(freq);
  std::cout << "Actual RX freq: " << usrp->get_rx_freq()/1e6 << " MHz" << std::endl;


  uhd::stream_args_t stream_args("fc32"); //complex floats
  rx_stream = usrp->get_rx_stream(stream_args);

  /* setup streaming */
  uhd::stream_cmd_t stream_cmd = uhd::stream_cmd_t(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
  stream_cmd.num_samps = 0;
  stream_cmd.stream_now = true;
  stream_cmd.time_spec = uhd::time_spec_t();
  rx_stream->issue_stream_cmd(stream_cmd);
}

void
device_uhd::send(const window &buf, size_t len)
{
#ifdef USE_USRP_STREAM_API
    uhd::tx_metadata_t md;
    md.start_of_burst = false;
    md.end_of_burst = false;
    md.has_time_spec = false;

    size_t num_tx_samps = usrp->get_device()->send(
      &big_buf.front(),
      big_buf.size(),
      md,
      uhd::io_type_t::COMPLEX_FLOAT32,
      uhd::device::SEND_MODE_FULL_BUFF);
#else
    uhd::tx_metadata_t md;
    md.start_of_burst = false;
    md.end_of_burst = false;
    md.has_time_spec = false;
    tx_stream->send(&buf.front(), buf.size(), md);
#endif
}

size_t
device_uhd::receive(window &buf, size_t len)
{
  uhd::rx_metadata_t md;

#ifdef USE_USRP_STREAM_API
  /* setup streaming */
  uhd::stream_cmd_t stream_cmd = uhd::stream_cmd_t(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
  stream_cmd.num_samps = 0;
  stream_cmd.stream_now = true;
  stream_cmd.time_spec = uhd::time_spec_t();
  usrp->issue_stream_cmd(stream_cmd);


  size_t num_samps = usrp->get_device()->recv(&buf.front(),
                                              buf.size(),
                                              md,
                                              uhd::io_type_t::COMPLEX_FLOAT32,
                                              uhd::device::RECV_MODE_FULL_BUFF);

  //handle the error codes
  switch(md.error_code)
  {
    case uhd::rx_metadata_t::ERROR_CODE_NONE:
      return num_samps;
      break;

    case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
      std::cout << "U" << std::endl;
      break;

    default:
      std::cout << "D" << std::endl;
      break;
  }
#else
      rx_stream->recv(&buf.front(), buf.size(), md, 0.001024);
#endif
      switch(md.error_code)
      {
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
          return 1;
          break;

        case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
          std::cout << "U" << std::endl;
          return 0;
          break;

        default:
          std::cout << "D" << std::endl;
          return 0;
          break;
      }
}

device_image_gen::device_image_gen(std::string device_args)
{
}

void
device_image_gen::send(const window &buf, size_t len)
{
   static const size_t cols = len;
   static sfft_complex g_ifft_complex = sfft_complex(new fft_complex(len));
   const size_t rows = 500;
   static size_t img_counter = 0;

   g_ifft_complex->set_data(&buf.front(), len);
   g_ifft_complex->execute();

   g_iq_samples.insert(g_iq_samples.end(),
                       g_ifft_complex->get_outbuf(),
                       g_ifft_complex->get_outbuf() + len);

   /* TODO get fft size and number of rows from somewhere else */
   if (g_iq_samples.size() == cols * rows)
   {
      cv::Mat img(rows, cols, CV_8UC3, cv::Scalar(0,0,0));
      cv::Mat image = img;

      float max = 0;
      for(int x=0; x < img.cols; x++)
      {
         for(int y=0;y < img.rows;y++)
         {
            float val = std::abs(g_iq_samples[ x + cols * y]);
            if (val > max) max = val;
         }
      }

      for(int x=0; x< img.cols; x++)
      {
         for(int y=0;y<img.rows;y++)
         {
            float val = std::abs(g_iq_samples[ x + cols * y]);
            cv::Vec3b color = image.at<cv::Vec3b>(cv::Point(x,y));
            color.val[0] = uchar(val / max * 255.0);
            color.val[1] = uchar(val / max * 255.0);
            color.val[2] = uchar(val / max * 255.0);
            image.at<cv::Vec3b>(cv::Point(x,y)) = color;
         }
      }

      std::cout << "Saving image: ./watterfall_" << std::to_string(img_counter) << ".png" << std::endl;
      cv::imwrite(std::string("./waterfall_" + std::to_string(img_counter++) + ".png"), img);
      g_iq_samples.clear();
   }
}

size_t
device_image_gen::receive(window &buf, size_t len)
{
  static const char *str_read = "/home/connect/ofdm.bin";
  static std::ifstream infile(str_read, std::ifstream::binary);

  if (not infile.eof())
  {
    infile.read((char*)&buf.front(), len * sizeof(gr_complex));
  }
  else
  {
    std::cout << "Reseting file" << std::endl;
    infile.clear();
    infile.seekg(0);
    infile.read((char*)&buf.front(), len * sizeof(gr_complex));
  }
}

device_loopback::device_loopback(std::string device_args)
{
}

void
device_loopback::send(const window &buf, size_t len)
{
   std::lock_guard<std::mutex> _l(g_mutex);
   g_windows_vec.push_back(buf);
}

size_t
device_loopback::receive(window &buf, size_t len)
{
   while (g_windows_vec.size() == 0) { return 0; };

   std::lock_guard<std::mutex> _l(g_mutex);
   buf = g_windows_vec.front();
   g_windows_vec.pop_front();

   return buf.size();
}


device_network::device_network(std::string host_addr, std::string remote_addr):
  g_host_addr(host_addr),
  g_remote_addr(remote_addr)
{
}

void
device_network::send(const window &buf, size_t len)
{
  if (!init_tx)
  {
    zmq::context_t context;
    socket_tx = std::make_unique<zmq::socket_t>(context, ZMQ_PUSH);
    socket_tx->connect("tcp://" + g_host_addr);
    init_tx = true;
  }

  zmq::message_t message(buf.size() * sizeof(gr_complex));

  iq_sample *tmp = static_cast<iq_sample *>(message.data());
  for (size_t i = 0; i < buf.size(); ++i)
    tmp[i] = buf[i];

  socket_tx->send(message);
}

size_t
device_network::receive(window &buf, size_t len)
{
   if (!init_rx)
   {
     zmq::context_t context;
     socket_rx = std::make_unique<zmq::socket_t>(context, ZMQ_PULL);
     socket_rx->connect("tcp://" + g_remote_addr);
     init_rx = true;
   }

   zmq::message_t message;
   socket_rx->recv(&message);

   if  (message.size() != len)
     std::cout << "Error: message size does not equal to len" << std::endl;

   if (message.size() > 0)
   {
     iq_sample *tmp = static_cast<iq_sample *>(message.data());

      if (message.size() % sizeof(iq_sample) != 0)
        std::cout << "Error: message not complete" << std::endl;

      buf.insert(buf.begin(),
          tmp,
          tmp + (message.size()/sizeof(iq_sample)));
   }

   return message.size();
}


} /* namespace hydra */
