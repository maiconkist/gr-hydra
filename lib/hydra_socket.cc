#include "hydra/hydra_socket.h"

#include <numeric>
#include <complex>
#include <boost/format.hpp>

namespace hydra
{


tcp_source::tcp_source(
  const std::string& host,
  const std::string& port):
  s_host(host),
  s_port(port)
{
    // Reinterpret the cast to the input buffer to pass it to the output buffer
    p_reinterpreted_cast = reinterpret_cast<iq_sample*>(&input_buffer);
    // Set the remainder counter to zero
    u_remainder = 0;

    // Create a thread to receive the data
    rx_udp_thread = std::make_unique<std::thread>(&tcp_source::connect, this);
}


void
tcp_source::connect()
{
  std::this_thread::sleep_for(std::chrono::seconds(5));
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(s_host), std::stoi(s_port));
  p_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_service);
  p_socket->connect(endpoint);
  std::cout << boost::format("Source %s:%s connected successfully.") % s_host % s_port << std::endl;

  receive();
  io_service.run();
}

void
tcp_source::receive()
{
  p_socket->async_receive(
                          boost::asio::buffer(input_buffer, BUFFER_SIZE),
                          boost::bind(&tcp_source::handle_receive, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred)
                          );
}

void
tcp_source::handle_receive(
  const boost::system::error_code& error,
  unsigned int u_bytes_trans)
{
  if (!error)
  {
    {
      std::lock_guard<std::mutex> _l(out_mtx);
      output_buffer.insert(output_buffer.end(),
                           input_buffer.begin(),
                           input_buffer.end());
    }
    receive();
  }
}


tcp_sink::tcp_sink(
  iq_stream *input_buffer,
  std::mutex* in_mtx,
  const std::string& s_host,
  const std::string& s_port):
  g_input_buffer(input_buffer),
  g_th_run(true)
{
  // Get the mutex
  p_in_mtx = in_mtx;

  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(s_host), std::stoi(s_port));

  p_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_service);
  p_socket->connect(endpoint);

  tx_udp_thread = std::make_unique<std::thread>(&tcp_sink::transmit, this);
}

// Assign the handle receive callback when a datagram is received
void
tcp_sink::transmit()
{

  std::vector< std::complex<float> > output_buffer;
  while (g_th_run)
  {
    // If there is anything to transmit
    if (g_input_buffer->size() > 0)
    {
      size_t n_elemns;

      /* Local scope lock */
      {
        // Copy everything to output_buffer. Clear input
        std::lock_guard<std::mutex> _inmtx(*p_in_mtx);

        output_buffer = std::vector< std::complex<float> >(g_input_buffer->begin(), g_input_buffer->end());
        g_input_buffer->erase(g_input_buffer->begin(), g_input_buffer->end());
      }

      // Send from output_buffer
      boost::asio::write(*p_socket,
                         boost::asio::buffer(output_buffer)
                         );
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }
}

udp_source::udp_source(
  const std::string& s_host,
  const std::string& s_port)
{
    // Reinterpret the cast to the input buffer to pass it to the output buffer
    p_reinterpreted_cast = reinterpret_cast<iq_sample*>(&input_buffer);
    // Set the remainder counter to zero
    u_remainder = 0;

    // Create an IP resolver
    boost::asio::ip::udp::resolver resolver(io_service);
    // Query routes to the host
    boost::asio::ip::udp::resolver::query query(
      s_host, s_port, boost::asio::ip::resolver_query_base::passive);

    // Resolve the address
    endpoint_ = *resolver.resolve(query);

    // Create the socket
    p_socket = new boost::asio::ip::udp::socket(io_service);
    // Open the socket
    p_socket->open(endpoint_.protocol());

    // Reuse the address and bind the port
    boost::asio::socket_base::reuse_address roption(true);
    p_socket->set_option(roption);
    p_socket->bind(endpoint_);

    // Start the receive method
    receive();

    // Create a thread to receive the data
    rx_udp_thread = std::make_unique<std::thread>(&udp_source::run_io_service, this);
}

  // Assign the handle receive callback when a datagram is received
void
udp_source::receive()
{
  p_socket->async_receive_from(
    boost::asio::buffer(input_buffer, BUFFER_SIZE),
    endpoint_rcvd,
    boost::bind(&udp_source::handle_receive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
    );
}

void
udp_source::handle_receive(
  const boost::system::error_code& error,
  unsigned int u_bytes_trans)
{
  std::cout << "u_bytes_trans:" << u_bytes_trans << std::endl;

  if (!error)
  {
    // If there isn't enough data for a single element
    if (u_bytes_trans + u_remainder < IQ_SIZE)
    {
      // Copy from the input to the remainder buffer
      std::copy(input_buffer.begin(),
                input_buffer.begin() + u_bytes_trans,
                remainder_buffer.begin() + u_remainder);
      // Update the remainder bytes count
      u_remainder += u_bytes_trans;
    }
    // Hooray, we have elements
    else
    {
      // Data not being consumed
      if (output_buffer.size() > 1e6)
      {
        std::cerr << "Too much data!" << std::endl;
      }
      // Plenty of space
      else
      {
        // Lock the mutex
        {
           std::lock_guard<std::mutex> _l(out_mtx);
           // If there is data from a previous transfer
           if (u_remainder > 0)
           {
              // Copy the missing bytes from the input buffer to the remainder buffer
              std::copy(input_buffer.begin(),
                        input_buffer.begin() + IQ_SIZE - u_remainder,
                        remainder_buffer.begin() + u_remainder);
              // Append this element to the output buffer
              output_buffer.insert(output_buffer.end(),
                                   remainder_buffer.begin(),
                                   remainder_buffer.begin() + 1);
              // Clear the remainder
              u_remainder = 0;
           }
           // Calculate the new remainder
           u_remainder = u_bytes_trans % IQ_SIZE;

           // Insert new elements in the output buffer
           output_buffer.insert(output_buffer.end(),
                                p_reinterpreted_cast,
                                p_reinterpreted_cast +
                                (u_bytes_trans - u_remainder)/IQ_SIZE);
        }

        // If there is a new remainder
        if (u_remainder > 0)
        {
           // Save it in the remainder buffer
           std::copy(input_buffer.begin() + u_bytes_trans - u_remainder,
                     input_buffer.begin() + u_bytes_trans ,
                     remainder_buffer.begin());
        }
      } // end data else
    } // end no data
  } // end !error
  // Succeeding or not, receive again
  receive();
}


udp_sink::udp_sink(
  iq_stream *input_buffer,
  std::mutex* in_mtx,
  const std::string& s_host,
  const std::string& s_port):
  g_input_buffer(input_buffer),
  g_th_run(true)
{
  // Get the mutex
  p_in_mtx = in_mtx;

  boost::asio::ip::udp::resolver resolver(io_service);
  boost::asio::ip::udp::resolver::query query(s_host, s_port);
  endpoint_ = *resolver.resolve(query);

  p_socket = std::make_unique<boost::asio::ip::udp::socket>(io_service);
  p_socket->open(endpoint_.protocol());

  tx_udp_thread = std::make_unique<std::thread>(&udp_sink::transmit, this);
}

// Assign the handle receive callback when a datagram is received
void
udp_sink::transmit()
{
  gr_complex output_buffer[BUFFER_SIZE];

  while (g_th_run)
  {
    // If there is anything to transmit
    if (g_input_buffer->size() > 0)
    {
      size_t n_elemns;

      /* Local scope lock */
      {
        // Copy everything to output_buffer. Clear input
        std::lock_guard<std::mutex> _inmtx(*p_in_mtx);

        n_elemns = std::min(g_input_buffer->size(), BUFFER_SIZE);

        for (size_t i = 0; i < n_elemns; ++i)
          output_buffer[i] = (*g_input_buffer)[i];

        g_input_buffer->erase(g_input_buffer->begin(), g_input_buffer->begin() + n_elemns);
      }

      // Get the current size of the queue in bytes
      size_t bytes_sent = 0;
      //size_t total_size_bytes = output_buffer.size() * IQ_SIZE;
      size_t total_size_bytes =  n_elemns * IQ_SIZE;

      // Send from output_buffer
      while (bytes_sent < total_size_bytes)
      {
        size_t bytes_to_send = std::min((size_t)BUFFER_SIZE * IQ_SIZE, (total_size_bytes - bytes_sent));
        try
        {
          size_t r = p_socket->send_to(
            boost::asio::buffer(reinterpret_cast<char *>(&output_buffer[0]) + bytes_sent,
                                bytes_to_send), endpoint_);
          bytes_sent += r;
        }
        catch (std::exception &e)
        {
          std::cerr << "error sending udp packet: " << e.what() << std::endl;
        }
      }
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  } // while
}


// Test module
int
test_socket()
{
  // Default variables
  std::string host = "localhost";
  std::string port = "5000";

  // Initialise the UDP client
	udp_source server(host, port);

  iq_stream* buffer = server.buffer();

  int i = 0;
  while (true)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      std::cout << i << "\t";
      for(int j = 0; j < 10; j++)
        {
          std::cout << (*buffer)[j+i] << "\t";
        }
      std::cout << std::endl;

      i+=10;

    }

  return 0;
}

} // namespace hydra
