#ifndef INCLUDED_HYDRA_RESAMPLER_H
#define INCLUDED_HYDRA_RESAMPLER_H

#include <thread>
#include <chrono>

#include "hydra/hydra_buffer.hpp"
#include "hydra/hydra_log.h"
#include "hydra/types.h"


namespace hydra {

template <typename input_data_type = iq_sample, typename output_data_type = iq_window>
class resampler
{
  private:
    // Pointer to the buffer object
    std::shared_ptr<hydra_buffer<input_data_type>> p_input_buffer;
    // Inernal buffer object
    std::shared_ptr<hydra_buffer<output_data_type>> p_output_buffer;

    hydra_log logger;

    // Event loop thread
    std::unique_ptr<std::thread> run_thread;
    // Thread stop condition
    bool stop_thread;

    // Event loop method
    void run();

    // Create the proper output buffer
    void create_buffer();

    // Hold the FFT size
    unsigned int u_fft_size;

  public:
    // Default constructor
    resampler();

    // Consturctor
    resampler(
        std::shared_ptr<hydra_buffer<input_data_type>> input_buffer,
        double sampling_rate,
        size_t fft_size);

   // Stop the thread
   void stop()
   {
      // Stop the thread
      stop_thread = true;
      // Stop the buffer thread
      run_thread->join();
   };

  // Return pointer to the internal buffer
  std::shared_ptr<hydra_buffer<output_data_type>> buffer()
  {
    return p_output_buffer;
  };

};


template <typename input_data_type, typename output_data_type>
resampler<input_data_type, output_data_type>::resampler()
{
};

template <typename input_data_type, typename output_data_type>
resampler<input_data_type, output_data_type>::resampler(
    std::shared_ptr<hydra_buffer<input_data_type>> input_buffer,
    double sampling_rate,
    size_t fft_size)
{
  // Thread stop flag
  stop_thread = false;

  // Number of IQ samples per FFT window
  u_fft_size = fft_size;
  // The input buffer
  p_input_buffer = input_buffer;

  // Create a different type of output buffer depending on the template
  create_buffer();

  // Create a thread to receive the data
  run_thread = std::make_unique<std::thread>(&resampler::run, this);

  logger = hydra_log("resampler");

  logger.info("Created resampler");
};


template<>
void
inline resampler<iq_sample, iq_window>::create_buffer()
{
  // Create a buffer with enough capacity for 1000 windows
  p_output_buffer = std::make_shared<hydra_buffer<iq_window>>(1000);

};

template<>
void
inline resampler<iq_window, iq_sample>::create_buffer()
{
  // Create a buffer with enough capacity for 1000 windows
  p_output_buffer = std::make_shared<hydra_buffer<iq_sample>>(1000*u_fft_size);

};

template <>
void
inline resampler<iq_sample, iq_window>::run()
{
  // Vector of IQ samples that comprise a FFT window
  iq_window temp_object(u_fft_size);

  // If the destructor has been called
  while (not stop_thread)
  {
    // Check whether the buffer has enough IQ samples
    if (p_input_buffer->size() >= u_fft_size)
    {
      // Insert IQ samples from the input buffer into the window
      temp_object = p_input_buffer->read(u_fft_size);

      //TODO Check the need for resampling here

      p_output_buffer->write(temp_object);

      /*
      for (auto it = temp_object.begin(); it != temp_object.end(); it++)
      {
        std::cout << (*it) << " ";
      }
      std::cout << std::endl;
      */
    }
    // Otherwise, we are not there yet
    else
    {
      // Wait for a microssecond and prevent the thread to explode
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }
};

template <>
void
inline resampler<iq_window, iq_sample>::run()
{
  // Vector of IQ samples that comprise a FFT window
  iq_window temp_object(u_fft_size);

  // If the destructor has been called
  while (not stop_thread)
  {
    // Check whether the buffer has windows
    if (p_input_buffer->size())
    {
      /*IMPORTANT
       *
       * Only consume a single window per time, required for resampling
       *
       */

      // Insert IQ samples from the input buffer into the window
      temp_object = p_input_buffer->read_one();

      //TODO Check the need for resampling here

      // Write all the IQ sample in this window
      p_output_buffer->write(temp_object.begin(), temp_object.end());
    }
    // Otherwise, we are not there yet
    else
    {
      // Wait for a microssecond and prevent the thread to explode
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }
};



} // Namespace

#endif
