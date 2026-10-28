#ifndef INCLUDED_HYDRA_RESAMPLER_H
#define INCLUDED_HYDRA_RESAMPLER_H

#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <limits>
#include <stdexcept>
#include <type_traits>

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
    std::atomic<bool> stop_thread{false};
    std::mutex stop_mutex;
    std::mutex configuration_mutex;

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

    ~resampler() { stop(); }

    static constexpr size_t max_fft_size = std::numeric_limits<unsigned int>::max() / 1000;

    static unsigned int buffer_capacity(size_t fft_size)
    {
      if (!fft_size || fft_size > max_fft_size)
        throw std::invalid_argument("Invalid framing size");
      return static_cast<unsigned int>(1000 * fft_size);
    }

    void reconfigure(size_t fft_size)
    {
      const auto capacity = buffer_capacity(fft_size);
      hydra_buffer<input_data_type> input(
          std::is_same_v<input_data_type, iq_sample> ? capacity : 1000);
      hydra_buffer<output_data_type> output(
          std::is_same_v<output_data_type, iq_sample> ? capacity : 1000);
      std::lock_guard<std::mutex> lock(configuration_mutex);
      // Socket workers retain these buffer objects across reconfiguration.
      p_input_buffer->swap(input);
      p_output_buffer->swap(output);
      u_fft_size = fft_size;
    }

   // Stop the thread
   void stop()
   {
      std::lock_guard<std::mutex> lock(stop_mutex);
      // Stop the thread
      stop_thread = true;
      // Stop the buffer thread
      if (run_thread && run_thread->joinable())
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
  buffer_capacity(fft_size);
  // Thread stop flag
  stop_thread = false;

  // Number of IQ samples per FFT window
  u_fft_size = fft_size;
  // The input buffer
  p_input_buffer = input_buffer;

  // Create a different type of output buffer depending on the template
  create_buffer();

  logger = hydra_log("resampler");
  // Create a thread to receive the data
  run_thread = std::make_unique<std::thread>(&resampler::run, this);

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
  p_output_buffer = std::make_shared<hydra_buffer<iq_sample>>(buffer_capacity(u_fft_size));

};

template <>
void
inline resampler<iq_sample, iq_window>::run()
{
  while (not stop_thread)
  {
    bool produced = false;
    {
      std::lock_guard<std::mutex> lock(configuration_mutex);
      auto window = p_input_buffer->read(u_fft_size);
      if (!window.empty())
      {
        p_output_buffer->write(window);
        produced = true;
      }
    }
    if (!produced)
      std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
};

template <>
void
inline resampler<iq_window, iq_sample>::run()
{
  while (not stop_thread)
  {
    bool produced = false;
    {
      std::lock_guard<std::mutex> lock(configuration_mutex);
      auto window = p_input_buffer->read_one();
      if (!window.empty())
      {
        p_output_buffer->write(window.begin(), window.end());
        produced = true;
      }
    }
    if (!produced)
      std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
};



} // Namespace

#endif
