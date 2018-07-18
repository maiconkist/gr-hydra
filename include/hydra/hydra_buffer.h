#ifndef INCLUDED_HYDRA_BUFFER_H
#define INCLUDED_HYDRA_BUFFER_H

#include <queue>
#include <vector>
#include <complex>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <math.h>

#include <hydra/types.h>

typedef std::complex<float> iq_sample;
typedef std::deque<iq_sample> iq_stream;
typedef std::vector<iq_sample> window;
typedef std::deque<window> window_stream;

class RxBuffer
{
private:
   // Pointer to the buffer object
   iq_stream * p_input_buffer;
   // Hold the FFT size
   unsigned int u_fft_size;
   // Flag to indicate use of padding or empty frame
   bool b_pad;
   // Time threshold for padding/empty frames
   long long int l_threshold;

   // Queue of arrays of IQ samples to be used for the FFT
   window_stream output_buffer;

   // Pointer to the buffer thread
   std::unique_ptr<std::thread> buffer_thread;
   // Pointer to the input mutex
   std::mutex* p_in_mtx;
   // Output mutex
   std::mutex out_mtx;
   // Thread stop condition
   bool thr_stop;

public:
   // CTOR 
   RxBuffer(iq_stream* input_buffer,
            std::mutex* in_mtx,
            float sampling_rate,
            unsigned int fft_size,
            bool pad);

   // DTOR
   ~RxBuffer()
   {
      // Stop the thread
      thr_stop = true;
      // Stop the buffer thread
      buffer_thread->join();
   };

   // Method to receive UDP data and put it into a buffer
   void run();

   window never_delete;
   const iq_window * consume();

   // Returns pointer to the output buffer of windows
   window_stream* windows(){return &output_buffer;};

   // Returns pointer to the mutex
   std::mutex* mutex() {return &out_mtx;};
};

class TxBuffer
{
public:
   // Constructor
   TxBuffer(window_stream* input_buffer,
            std::mutex* in_mtx,
            double sampling_rate,
            unsigned int fft_size);

   // Destructor
   ~TxBuffer()
   {
      // Stop the thread
      thr_stop = true;
      // Stop the buffer thread
      buffer_thread->join();
   };

   // Method to transmit UDP data from a buffer
   void run();

   // Returns pointer to the output buffer, a stream of IQ samples
   iq_stream* stream(){return &output_buffer;};
   // Returns pointer to the mutex
   std::mutex* mutex() {return &out_mtx;};

private:
   // Pointer to the input FFT
   window_stream* p_input_buffer;
   // Hold the FFT size
   unsigned int u_fft_size;
   // Time threshold for padding/empty frames
   std::size_t threshold;
   // Output deque

   iq_stream output_buffer;

   // Pointer to the buffer thread
   std::unique_ptr<std::thread> buffer_thread;
   // Pointer to the mutex
   std::mutex* p_in_mtx;
   // Output mutex
   std::mutex out_mtx;
   // Thread stop condition
   bool thr_stop;
};

typedef std::unique_ptr<RxBuffer> RxBufferPtr;
typedef std::unique_ptr<TxBuffer> TxBufferPtr;

#endif
