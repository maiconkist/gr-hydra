#include "hydra/hydra_buffer.h"


namespace hydra {

RxBuffer::RxBuffer(
   std::deque<std::complex<float>>* input_buffer,
   std::mutex* in_mtx,
   float sampling_rate,
   unsigned int fft_size,
   bool pad)
{
  // Time threshold - round the division to a long int
  l_threshold = llrint(fft_size * 1e6 / sampling_rate);

  // Number of IQ samples per FFT window
  u_fft_size = fft_size;
  // The input buffer
  p_input_buffer = input_buffer;
  // Get the mutex
  p_in_mtx = in_mtx;
  // Get the padding flag
  b_pad = pad;

  // Create a thread to receive the data
  buffer_thread = std::make_unique<std::thread>(&RxBuffer::run, this);
}


const window *
RxBuffer::consume()
{
   /* We do an ugly thing here: we have 'never_delete'
    * storing the data want to be consumed by the hypervisor
    */
   std::lock_guard<std::mutex> _l(out_mtx);

   if (output_buffer.size())
   {
      never_delete = output_buffer.front();
      output_buffer.pop_front();
   }
   else
   {
      return nullptr;
   }

   return &never_delete;
}

void RxBuffer::run()
{
  // Thread stop condition
  thr_stop = false;
  // Vector of IQ samples that comprise a FFT window
  std::vector<std::complex<float>> window;
  // Reserve number of elements
  window.reserve(u_fft_size);
  // Empty IQ sample, as zeroes
  std::complex<float> empty_iq = {0.0, 0.0};
  // Integer to hold the current size of the queue
  long long int ll_cur_size;

  // Consume indefinitely
  while(true)
  {
     // Wait for "threshold" nanoseconds
     std::this_thread::sleep_for(std::chrono::microseconds(l_threshold));

     // If the destructor has been called
     if (thr_stop){return;}

     // Lock access to the buffer
     // Windows not being consumed
     if (output_buffer.size() > 100)
     {
       //std::lock_guard<std::mutex> _l(out_mtx);
       //output_buffer.pop_front();
       //std::cerr << "Too many windows!" << std::endl;
     }

     {
        std::lock_guard<std::mutex> _p(*p_in_mtx);
        // Get the current size of the queue
        ll_cur_size = p_input_buffer->size();
        // Check whether the buffer has enough IQ samples
        if (ll_cur_size >= u_fft_size)
        {
           // Insert IQ samples from the input buffer into the window
           window.assign(p_input_buffer->begin(),
                         p_input_buffer->begin() + u_fft_size); // 0..C

           // Erase the beginning of the queue
           p_input_buffer->erase(p_input_buffer->begin(),
                                 p_input_buffer->begin() + u_fft_size);

           std::lock_guard<std::mutex> _l(out_mtx);
           output_buffer.push_back(window);
        }
        // Otherwise, the buffer is not ready yet
        else if (b_pad)
        {
           // Copy the current amount of samples from the buffer to the window
           window.assign(p_input_buffer->begin(),
                         p_input_buffer->begin() + ll_cur_size); // 0..C-1
           // Erase the beginning of the queue
           p_input_buffer->erase(p_input_buffer->begin(),
                                 p_input_buffer->begin() + ll_cur_size);

           // Fill the remainder of the window with complex zeroes
           window.insert(window.begin() + ll_cur_size,
                         u_fft_size - ll_cur_size,
                         empty_iq); // C..F-1

           // Add the window to the output buffer
           std::lock_guard<std::mutex> _l(out_mtx);
           output_buffer.push_back(window);
        }
        // Without padding, just transmit an empty window and wait for the next one
        else
        {
           // Fill the window with complex zeroes
           window.assign(u_fft_size, empty_iq);

           // Add the window to the output buffer
           std::lock_guard<std::mutex> _l(out_mtx);
           output_buffer.push_back(window);
           out_mtx.unlock();
        }
     } // Too much data check

  } // End of loop
} // End of method



TxBuffer::TxBuffer(window_stream* input_buffer,
                   std::mutex* in_mtx,
                   double sampling_rate,
                   unsigned int fft_size)
{
  // Time threshold - round the division to a long int
  threshold = fft_size * 1e9 / sampling_rate;

  // Number of IQ samples per FFT window
  u_fft_size = fft_size;
  // The input buffer
  p_input_buffer = input_buffer;
  // Get the input mutex
  p_in_mtx = in_mtx;

  // Create a thread to receive the data
  buffer_thread = std::make_unique<std::thread>(&TxBuffer::run, this);
}

void
TxBuffer::run()
{
  // Thread stop condition
  thr_stop = false;
  // Empty IQ sample, as zeroes
  iq_sample empty_iq = {0.0, 0.0};
  // Temporary array to hold a window worth of IQ samples
  std::vector<iq_sample> window;
  window.reserve(u_fft_size);

  // Produce indefinitely
  while(true)
  {
    // Wait for "threshold" nanoseconds
    std::this_thread::sleep_for(std::chrono::nanoseconds(threshold));
    // If the destructor has been called
    if (thr_stop){return;}

    // Data not being consumed
    if (output_buffer.size() > 1e8)
    {
      //std::cerr << "Too much data!" << std::endl;
    }
    // Plenty of space
    else
    {
      // Lock access to the buffer
      std::lock_guard<std::mutex> _inmtx(*p_in_mtx);

      // Check if there's a valid window ready for transmission
      if (not p_input_buffer->empty())
      {
        // Copy the first window from the input buffer
        window = p_input_buffer->front();
        // Pop the oldest window
        p_input_buffer->pop_front();

        // Lock access to the output buffer
        std::lock_guard<std::mutex> _omtx(out_mtx);
        // Insert IQ samples of the oldest window in the output deque
        output_buffer.insert(output_buffer.end(),
                                window.begin(),
                                window.end());
      }
      // If the queue of windows is empty at the moment
      else
      {
        std::lock_guard<std::mutex> _omtx(out_mtx);
        // Stream empty IQ samples that comprise the window duration
        output_buffer.insert(output_buffer.end(), u_fft_size, empty_iq);
      } // End padding
    } // End overflow
  } // End data check
}

void
TxBuffer::produce(const gr_complex *buf, size_t len)
{
  std::lock_guard<std::mutex> _l(*p_in_mtx);
}



} // namespace hydra
