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

namespace hydra
{

class zmq_source
{
 public:
  /* CTOR
   */
  zmq_source(const std::string &server_addr,
             const std::string &remote_addr,
             const std::string& port);

  static std::unique_ptr<zmq_source> make(const std::string &server_addr,
                                          const std::string &remote_addr,
                                          const std::string& port)
  {
    return std::make_unique<zmq_source>(server_addr, remote_addr, port);
  }

  // Returns pointer to the output buffer
  iq_stream* buffer(){ return &output_buffer; };

  // Returns pointer to the mutex
  std::mutex* mutex() {return &out_mtx;};

 private:
  std::string s_host;
  std::string s_port;

  // UDP thread to handle the receiving of datagrams
  std::unique_ptr<std::thread> rx_thread;

  // Create output buffer
  iq_stream output_buffer;

  // Lock access to the deque
  std::mutex out_mtx;

  void connect();
};


class zmq_sink
{
public:
  /* CTOR
   */
  zmq_sink(iq_stream *p_input_buffer,
           std::mutex *in_mtx,
           const std::string& server_addr,
           const std::string& remote_addr,
           const std::string& port);

  static std::unique_ptr<zmq_sink> make(iq_stream *p_input_buffer,
                                        std::mutex *in_mtx,
                                        const std::string& server_addr,
                                        const std::string& remote_addr,
                                        const std::string& port)
  {
    return std::make_unique<zmq_sink>(p_input_buffer, in_mtx, server_addr, remote_addr, port);
  }

  void transmit();

private:
  std::string s_host;
  std::string s_port;

  // UDP thread to handle the receiving of datagrams
  std::unique_ptr<std::thread> tx_thread;

  bool g_th_run;
  iq_stream * g_input_buffer;
  std::mutex * p_in_mtx;
};


class tcp_sink
{
public:

  /* CTOR
   */
  tcp_sink(iq_stream* p_input_buffer,
           std::mutex* in_mtx,
           const std::string &s_host,
           const std::string &s_port);

  /* DTOR
   */
  ~tcp_sink()
  {
    g_th_run = false;
    tx_udp_thread->join();

    io_service.reset();
    io_service.stop();
    p_socket->close();
  };

  // Assign the handle receive callback when a datagram is received
  void transmit();

  static std::unique_ptr<tcp_sink> make(iq_stream* p_input_buffer,
                           std::mutex* in_mtx,
                           const std::string &s_host,
                           const std::string &s_port)
  {
    return std::make_unique<tcp_sink>(p_input_buffer, in_mtx, s_host, s_port);
  }


 private:
  // Default UDP buffer size
  const size_t BUFFER_SIZE = 100000;
  // Default IQ samples size
  const size_t IQ_SIZE = sizeof(iq_sample);

  boost::asio::io_service io_service;
  std::unique_ptr<boost::asio::ip::tcp::socket> p_socket;

  bool g_th_run;

  // UDP thread to handle the transmitting datagrams
  std::unique_ptr<std::thread> tx_udp_thread;

  // Pointer to output buffer
  iq_stream* g_input_buffer;

  // Pointer to the input buffer mutex
  std::mutex* p_in_mtx;
};

class udp_source
{
 public:
  /* CTOR
   */
  udp_source(const std::string& host, const std::string& port);

  /* DTOR
   */
  ~udp_source()
    {
      io_service.reset();
      io_service.stop();
      rx_udp_thread->join();

      p_socket->close();
      delete p_socket;
    };

  static std::unique_ptr<udp_source> make(const std::string& host, const std::string& port)
  {
    return std::make_unique<udp_source>(host, port);
  }

  // Registers the handle_receive method as a callback for incoming datagrams
  void receive();

  // Returns pointer to the output buffer
  iq_stream* buffer(){ return &output_buffer; };

  // Returns pointer to the mutex
  std::mutex* mutex() {return &out_mtx;};

 private:
  // Default UDP buffer size
  const static size_t BUFFER_SIZE = 100000;
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

  // Handle datagram and buffers, outputting to a queue
  void handle_receive(const boost::system::error_code& error, unsigned int u_bytes_trans);
  // Start the IO service before receiving the IQ samples
  void run_io_service() { io_service.run(); };
};


class udp_sink
{
public:

  /* CTOR
   */
  udp_sink(iq_stream* p_input_buffer,
           std::mutex* in_mtx,
           const std::string &s_host,
           const std::string &s_port);

  /* DTOR
   */
  ~udp_sink()
  {
    g_th_run = false;
    tx_udp_thread->join();

    io_service.reset();
    io_service.stop();
    p_socket->close();
  };

  // Assign the handle receive callback when a datagram is received
  void transmit();

  static std::unique_ptr<udp_sink> make(iq_stream* p_input_buffer,
                           std::mutex* in_mtx,
                           const std::string &s_host,
                           const std::string &s_port)
  {
    return std::make_unique<udp_sink>(p_input_buffer, in_mtx, s_host, s_port);
  }


 private:
  // Default UDP buffer size
  const size_t BUFFER_SIZE = 1300;
  // Default IQ samples size
  const size_t IQ_SIZE = sizeof(iq_sample);

  boost::asio::io_service io_service;
  std::unique_ptr<boost::asio::ip::udp::socket> p_socket;
  boost::asio::ip::udp::endpoint endpoint_;

  bool g_th_run;

  // UDP thread to handle the transmitting datagrams
  std::unique_ptr<std::thread> tx_udp_thread;

  // Pointer to output buffer
  iq_stream* g_input_buffer;

  // Pointer to the input buffer mutex
  std::mutex* p_in_mtx;
};

typedef std::unique_ptr<zmq_source> zmq_source_ptr;
typedef std::unique_ptr<zmq_sink> zmq_sink_ptr;
typedef std::unique_ptr<udp_source> udp_source_ptr;
typedef std::unique_ptr<udp_sink> udp_sink_ptr;

} // namespace hydra

#endif
