#include "hydra/hydra_uhd_interface.h"

#include <iostream>
#include <opencv2/opencv.hpp>

namespace hydra {

device_uhd::device_uhd(double freq,
                     double rate,
                     double gain,
                     std::string device_args):
   abstract_device(freq, rate, gain)
{
   if(uhd_set_thread_priority(uhd_default_thread_priority, true))
      std::cout <<  "Unable to set thread priority. Continuing anyway.\n" << std::endl;;

   std::cout <<  "Creating USRP with args \"" <<  device_args << "\"" << std::endl;
   uhd_usrp_make(&usrp, device_args.c_str());

   uhd_tx_streamer_make(&tx_streamer);

   // Create TX metadata
   uhd_tx_metadata_make(&tx_md, false, 0, 0.1, true, false);

   // Create other necessary structs
   uhd_tune_request_t tune_request;
   tune_request.target_freq = freq;
   tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
   tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
   uhd_tune_result_t tune_result;

   size_t channel = 0;

   // Set rate
   std::cout << "Setting TX Rate: " << rate << std::endl;
   uhd_usrp_set_tx_rate(usrp, rate, channel);
   // See what rate actually is
   uhd_usrp_get_tx_rate(usrp, channel, &rate);
   std::cout << "Actual TX Rate: " << rate << std::endl;

   // Set gain
   std::cout << "Setting TX Gain: " << gain << std::endl;
   uhd_usrp_set_tx_gain(usrp, gain, 0, "");
   // See what gain actually is
   uhd_usrp_get_tx_gain(usrp, channel, "", &gain);
   std::cout << "Actual TX Gain: " << gain << std::endl;

   // Set frequency
   std::cout << "Setting TX frequency: " << freq / 1e6 << " MHz" << std::endl;
   uhd_usrp_set_tx_freq(usrp, &tune_request, channel, &tune_result);
   uhd_usrp_get_tx_freq(usrp, channel, &freq);
   std::cout << "Actual TX frequency: " <<  freq / 1e6 << " MHz" << std::endl;

   // Set up streamer
   char cpu_format[] = "fc32";
   char otw_format[] = "sc16";
   char args[] =  "";
   uhd_stream_args_t stream_args = {
      cpu_format : cpu_format,
      otw_format : otw_format,
      args : args,
      channel_list : &channel,
      n_channels : 1
   };

   stream_args.channel_list = &channel;
   uhd_usrp_get_tx_stream(usrp, &stream_args, tx_streamer);
   // Set up buffer
   uhd_tx_streamer_max_num_samps(tx_streamer, &tx_samps_per_buff);
   std::cout << "Buffer size in samples: " << tx_samps_per_buff << std::endl;;
}

device_uhd::~device_uhd()
{
   uhd_tx_streamer_free(&tx_streamer);
   uhd_tx_metadata_free(&tx_md);
   uhd_usrp_free(&usrp);
}

void
device_uhd::send(const gr_complex *buf, size_t len)
{
   static size_t num_samps_sent = 0;

   uhd_tx_streamer_send(tx_streamer, (const void **)(&buf), tx_samps_per_buff, &tx_md, 0.1, &num_samps_sent);
}

void
device_uhd::set_frequency(double frequency)
{
   size_t channel = 0;
   uhd_tune_request_t tune_request;
   tune_request.target_freq = frequency;
   tune_request.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
   tune_request.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO;
   uhd_tune_result_t tune_result;

   // Set frequency
   uhd_usrp_set_tx_freq(usrp, &tune_request, channel, &tune_result);

   // Get actual frequency
   uhd_usrp_get_tx_freq(usrp, channel, &frequency);

   abstract_device::set_frequency(frequency);
}

void
device_uhd::set_rate(double rate)
{
   size_t channel = 0;

   std::cout << "Setting TX Rate: " << rate << std::endl;
   uhd_usrp_set_tx_rate(usrp, rate, channel);

   // See what rate actually is
   uhd_usrp_get_tx_rate(usrp, channel, &rate);
   std::cout << "Actual TX Rate: " << rate << std::endl;

   abstract_device::set_rate(rate);
}

void
device_uhd::set_gain(double gain)
{
   size_t channel = 0;

   // Set gain
   std::cout << "Setting TX Gain: " << gain << std::endl;
   uhd_usrp_set_tx_gain(usrp, gain, 0, "");
   // See what gain actually is
   uhd_usrp_get_tx_gain(usrp, channel, "", &gain);
   std::cout << "Actual TX Gain: " << gain << std::endl;

   abstract_device::set_gain(gain);
}


device_image_gen::device_image_gen(double freq, double rate, double gain):
   abstract_device(freq, rate, gain)
{
}


void
device_image_gen::send(const gr_complex *buf, size_t len)
{
   static const size_t cols = len;
   const size_t rows = 500;
   static size_t img_counter = 0;

   g_iq_samples.insert(g_iq_samples.end(), buf, buf + len);

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
