#include "hydra/hydra_socket.h"


namespace hydra {

udp_receiver::udp_receiver(
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
    endpoint = *resolver.resolve(query);

    // Create the socket
    p_socket = new boost::asio::ip::udp::socket(io_service);
    // Open the socket
    p_socket->open(endpoint.protocol());

    // Reuse the address and bind the port
    boost::asio::socket_base::reuse_address roption(true);
    p_socket->set_option(roption);
    p_socket->bind(endpoint);

    // Start the receive method
    receive();

    // Create a thread to receive the data
    rx_udp_thread = std::make_unique<std::thread>(&udp_receiver::run_io_service,
                                                  this);
}

  // Assign the handle receive callback when a datagram is received
void
udp_receiver::receive()
{
  p_socket->async_receive_from(
    boost::asio::buffer(input_buffer, BUFFER_SIZE),
    endpoint_rcvd,
    boost::bind(&udp_receiver::handle_receive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
    );
}

void
udp_receiver::handle_receive(
  const boost::system::error_code& error,
  unsigned int u_bytes_trans)
{

  std::cout << "Received something from the fucking udp. bytes:  " << u_bytes_trans << std::endl;

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
      if (output_buffer.size() > 1e8)
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


udp_transmitter::udp_transmitter(
  iq_stream* input_buffer,
  std::mutex* in_mtx,
  const std::string& s_host,
  const std::string& s_port)
{
  // Get pointer to the input buffer
  p_input_buffer = input_buffer;
  // Reinterpret the cast to the input buffer to pass it to the output buffer
  p_reinterpreted_cast = reinterpret_cast<char*>(p_input_buffer);
  // Get the mutex
  p_in_mtx = in_mtx;

  // Create an IP resolver
  boost::asio::ip::udp::resolver resolver(io_service);
  // Query routes to the host
  boost::asio::ip::udp::resolver::query query(
    s_host, s_port, boost::asio::ip::resolver_query_base::passive);

  // Resolve the address
  endpoint = *resolver.resolve(query);

  // Create the socket
  p_socket = new boost::asio::ip::udp::socket(io_service);
  // Open the socket
  p_socket->open(endpoint.protocol());

  // Reuse the address
  boost::asio::socket_base::reuse_address roption(true);
  p_socket->set_option(roption);

  // Create a thread to transmit the data
  tx_udp_thread = std::make_unique<std::thread>(&udp_transmitter::transmit,
                                                this);
}

  // Assign the handle receive callback when a datagram is received
void
udp_transmitter::transmit()
{
  // Variables to keep track of the buffer size and the transferable bytes
  long long int ll_cur_size_bytes;
  unsigned int u_trans_bytes;

  while (true)
  {
    // Lock access to the input buffer
    p_in_mtx->lock();
    // Get the current size of the queue in bytes
    ll_cur_size_bytes = p_input_buffer->size() * IQ_SIZE;

    // If there is anything to transmit
    if (ll_cur_size_bytes > 0)
    {
      // Get the minimum amount of data that we can transfer now
      u_trans_bytes = std::min((long long int) BUFFER_SIZE, ll_cur_size_bytes);
      // Copy this amount of bytes to the output buffer
      std::copy(p_reinterpreted_cast,
                p_reinterpreted_cast + u_trans_bytes,
                output_buffer.begin());

      // Number of bytes that couldn't be transferred but are still part of a single IQ sample
      u_remainder = u_trans_bytes % IQ_SIZE;

      // If there is a remainder
      if (u_remainder > 0)
      {
        // Add this amount of bytes to the remainder buffer
        std::copy(p_reinterpreted_cast + u_trans_bytes,
                  p_reinterpreted_cast + u_trans_bytes + IQ_SIZE - u_remainder,
                  remainder_buffer.begin());
      }

      // Remove the number of transferred elements from the input buffer
      p_input_buffer->erase(
        p_input_buffer->begin(),
        p_input_buffer->begin() +
          (u_trans_bytes + IQ_SIZE - u_remainder) / IQ_SIZE);
      // Unlock access to the input buffer
      p_in_mtx->unlock();

      // Try to transmit
      try
      {
        // Send the output buffer through the socket
        p_socket->send_to(
          boost::asio::buffer(output_buffer.begin(), u_trans_bytes),
          endpoint);
        // If there is a remainder
        if (u_remainder > 0)
        {
          // Send the remainder buffer through the socket
          p_socket->send_to(
            boost::asio::buffer(remainder_buffer.begin(), u_remainder),
            endpoint);
        }
      }
      // Not much we can do now :/
      catch(std::exception& e)
      {
        std::cerr << "Wops, could't send datagram :(" << std::endl;
      }
    } // End of check for data

    // If there isn't any data in the buffer yet
    else
    {
      // Unlock access to the input buffer
      p_in_mtx->unlock();
      // Sleep for a bit
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } // End of loop
} // End of method


// Test module
int
test_socket()
{
  // Default variables
  std::string host = "localhost";
  std::string port = "5000";

  // Initialise the UDP client
	udp_receiver server(host, port);

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
