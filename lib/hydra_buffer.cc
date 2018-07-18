#include "hydra/hydra_buffer.h"

RxBuffer::RxBuffer(
   std::deque<std::complex<float>>* input_buffer,
   std::mutex* in_mtx,
   float sampling_rate,
   unsigned int fft_size,
   bool pad)
{
  // Time threshold - round the division to a long int
  l_threshold = llrint(fft_size * 1e9 / sampling_rate);
  std::cout << "FFT Size: " << fft_size << "\tSampling Rate: " <<
     sampling_rate << "\tTime threshold: " << l_threshold << std::endl;

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
   std::lock_guard<std::mutex> _l(out_mtx);

   if (output_buffer.size())
   {
      std::cout << "returning consume data" << std::endl;
      never_delete = output_buffer.front();
      output_buffer.pop_front();
   }
   else
   {
      std::cout << "returning consume nullptr" << std::endl;
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
     std::this_thread::sleep_for(std::chrono::nanoseconds(l_threshold));
     // If the destructor has been called
     if (thr_stop){return;}

     // Lock access to the buffer

     // Windows not being consumed
     if (output_buffer.size() > 1e3)
     {
        std::cerr << "Too many windows!" << std::endl;
     }
     // Plenty of space
     else
     {
        p_in_mtx->lock();
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

           // Release access to the buffer
           p_in_mtx->unlock();

           out_mtx.lock();
           output_buffer.push_back(window);
           out_mtx.unlock();
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

           // Release access to the buffer
           p_in_mtx->unlock();

           // Fill the remainder of the window with complex zeroes
           window.insert(window.begin() + ll_cur_size,
                         u_fft_size - ll_cur_size,
                         empty_iq); // C..F-1

           // Add the window to the output buffer
           out_mtx.lock();
           output_buffer.push_back(window);
           out_mtx.unlock();
        }
        // Without padding, just transmit an empty window and wait for the next one
        else
        {
           // Release access to the input buffer
           p_in_mtx->unlock();

           // Fill the window with complex zeroes
           window.assign(u_fft_size, empty_iq);

           // Add the window to the output buffer
           out_mtx.lock();
           output_buffer.push_back(window);
           out_mtx.unlock();
        }
     } // Too much data check

     // std::cout << output_buffer.size() << std::endl;
  } // End of loop
} // End of method



TxBuffer::TxBuffer(window_stream* input_buffer,
                   std::mutex* in_mtx,
                   double sampling_rate,
                   unsigned int fft_size)
{
  // Time threshold - round the division to a long int
  threshold = fft_size * 1e9 / sampling_rate;
  std::cout << "TX: FFT Size: " << fft_size << "\tSampling Rate: " <<
    sampling_rate << "\tTime threshold: " << threshold << std::endl;

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
      std::cerr << "Too much data!" << std::endl;
    }
    // Plenty of space
    else
    {
      // Lock access to the buffer
      p_in_mtx->lock();

      // Check if there's a valid window ready for transmission
      if (not p_input_buffer->empty())
      {
        // Copy the first window from the input buffer
        window = p_input_buffer->front();
        // Pop the oldest window
        p_input_buffer->pop_front();

        // Release access to the input buffer
        p_in_mtx->unlock();


        // Lock access to the output buffer
        out_mtx.lock();
        // Insert IQ samples of the oldest window in the output deque
        output_buffer.insert(output_buffer.end(),
                                window.begin(),
                                window.end());
        // Release access to the output buffer
        out_mtx.unlock();
      }
      // If the queue of windows is empty at the moment
      else
      {
        // Release access to the buffer
        p_in_mtx->unlock();

        // Lock access to the output buffer
        out_mtx.lock();
        // Stream empty IQ samples that comprise the window duration
        output_buffer.insert(output_buffer.end(), u_fft_size, empty_iq);
        // Release access to the output buffer
        out_mtx.unlock();
      } // End padding
    } // End overflow
  } // End data check
}
