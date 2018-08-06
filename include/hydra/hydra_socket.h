#ifndef HYDRA_SOCKET_INCLUDE_H
#define HYDRA_SOCKET_INCLUDE_H

#include <iostream>
#include <complex>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "hydra/types.h"


// Using name space UDP
using boost::asio::ip::udp;

namespace hydra {

class udp_receiver
{
private:
   // Default UDP buffer size
   const static size_t BUFFER_SIZE = 1024;
   // Default IQ samples size
   const static size_t IQ_SIZE = sizeof(iq_sample);
   // Async IO services
   boost::asio::io_service io_service;
   // Socket object
   boost::asio::ip::udp::socket *p_socket;
   // Connection endpoint
   boost::asio::ip::udp::endpoint endpoint_;
   // Received endpoint
   boost::asio::ip::udp::endpoint endpoint_rcvd;

   // UDP thread to handle the receiving of datagrams
   std::unique_ptr<std::thread> rx_udp_thread;

   // Create input buffer
   std::array<char, BUFFER_SIZE> input_buffer;
   // Create remainder buffer, which just needs 16 bytes
   std::array<char, IQ_SIZE> remainder_buffer;
   // Create output buffer
   iq_stream output_buffer;

   // Counter of bytes remaining from a previous reception
   unsigned int u_remainder;
   // Reinterpreted pointer to the input buffer
   iq_sample* p_reinterpreted_cast;
   // Lock access to the deque
   std::mutex out_mtx;

public:
   // Constructor
   udp_receiver(const std::string& host, const std::string& port);
   // Destructor
   ~udp_receiver()
   {
      // Reset the IO service
      io_service.reset();
      // Stop the IO service
      io_service.stop();
      // Join the UDP thread that handle callbacks
      rx_udp_thread->join();

      // Close the socket
      p_socket->close();
      // Then delete it
      delete p_socket;
   };

   // Registers the handle_receive method as a callback for incoming datagrams
   void receive();
   // Returns pointer to the output buffer
   iq_stream* buffer(){return &output_buffer;};
   // Returns pointer to the mutex
   std::mutex* mutex() {return &out_mtx;};

private:
   // Handle datagram and buffers, outputting to a queue
   void handle_receive(const boost::system::error_code& error, unsigned int u_bytes_trans);
   // Start the IO service before receiving the IQ samples
   void run_io_service() { io_service.run(); };
};


class udp_transmitter
{
public:

   // Constructor
   udp_transmitter(iq_stream* p_input_buffer,
                   std::mutex* in_mtx,
                   const std::string &s_host,
                   const std::string &s_port);

   // Destructor
   ~udp_transmitter()
   {
      // Reset the IO service
      io_service.reset();
      // Stop the IO service
      io_service.stop();
      // Join the UDP thread
      tx_udp_thread->join();

      // Close the socket
      p_socket->close();
   };

   // Assign the handle receive callback when a datagram is received
   void transmit();

 private:
   // Default UDP buffer size
   const static unsigned  BUFFER_SIZE = 1024;
   // Default IQ samples size
   const static size_t IQ_SIZE = sizeof(iq_sample);


   boost::asio::io_service io_service;
   std::unique_ptr<boost::asio::ip::udp::socket> p_socket;
   boost::asio::ip::udp::endpoint endpoint_;

   // UDP thread to handle the transmitting datagrams
   std::unique_ptr<std::thread> tx_udp_thread;

   // Pointer to output buffer
   iq_stream* g_input_buffer;

   // Pointer to the input buffer mutex
   std::mutex* p_in_mtx;
};

typedef udp_receiver RxUDP;
typedef std::unique_ptr<udp_receiver> RxUDPPtr;
typedef udp_transmitter TxUDP;
typedef std::unique_ptr<udp_transmitter> TxUDPPtr;
} // namespace hydra

#endif
