#include "hydra/hydra_uhd_interface.h"

#include <iostream>
#include <opencv2/opencv.hpp>

namespace hydra {

#define EXECUTE_OR_PRINT(fun)                       \
  if(fun){                                          \
    std::cout << "Error in " <<  #fun << std::endl; \
  }

device_uhd::device_uhd(double freq,
                     double rate,
                     double gain,
                     std::string device_args):
   abstract_device(freq, rate, gain)
{
   std::cout <<  "Creating USRP with args \"" <<  device_args << "\"" << std::endl;

   usrp = uhd::usrp::multi_usrp::make(device_args);

   std::cout << "Setting TX Rate: " << rate << std::endl;
   usrp->set_tx_rate(rate);
   std::cout << "Actual TX Rate: " << usrp->get_tx_rate() << std::endl;

   std::cout << "Setting TX Gain: " << gain << std::endl;
   usrp->set_tx_gain(gain);
   std::cout << "Actual TX Gain: " << usrp->get_tx_gain() << std::endl;

   std::cout << "Setting TX freq: " << freq / 1e6 << " MHz" << std::endl;
   usrp->set_tx_freq(freq);
   std::cout << "Actual TX freq: " << usrp->get_tx_freq()/1e6 << " MHz" << std::endl;

   //create a transmit streamer
   std::string cpu_format;
   uhd::stream_args_t stream_args("fc64", "sc16");
   std::vector<size_t> channel_nums {0};
   stream_args.channels = channel_nums;
   tx_stream = usrp->get_tx_stream(stream_args);


}

device_uhd::~device_uhd()
{

  //uhd_tx_streamer_free(&tx_stream);
  //uhd_tx_metadata_free(&tx_md);
  //uhd_usrp_free(&usrp);
}

void
device_uhd::send(const window &buf, size_t len)
{
  const static uhd::tx_metadata_t md;
  static size_t num_samps_sent = 0;

  tx_stream->send(&buf.front(), buf.size(), md);
}

void
device_uhd::set_frequency(double frequency)
{
   abstract_device::set_frequency(frequency);
}

void
device_uhd::set_rate(double rate)
{
   abstract_device::set_rate(rate);
}

void
device_uhd::set_gain(double gain)
{
   abstract_device::set_gain(gain);
}


device_image_gen::device_image_gen(double freq, double rate, double gain):
   abstract_device(freq, rate, gain)
{
}


void
device_image_gen::send(const window &buf, size_t len)
{
   static const size_t cols = len;
   const size_t rows = 500;
   static size_t img_counter = 0;

   g_iq_samples.insert(g_iq_samples.end(), buf.begin(), buf.end());

   // TODO get fft size and number of rows from somewhere else
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

} // namespace hydra
