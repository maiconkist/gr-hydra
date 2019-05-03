#include "hydra/hydra_socket.h"

namespace hydra
{

// ZMQ Source
zmq_source::zmq_source(const std::string& server_addr,
                       const std::string& remote_addr,
                       const std::string& port,
                       const unsigned int& buf_size):
  s_host(remote_addr),
  s_port(port),
  g_th_run(true),
  socket(context, ZMQ_PULL)
{
  // Create the outbut buffer with the proper buffer size
  p_output_buffer = std::make_shared<hydra_buffer<iq_sample>>(buf_size);
  // Create a thread to receive the data
  g_rx_thread = std::make_unique<std::thread>(&zmq_source::run, this);

  logger = hydra_log("zmq|source");
};

void
zmq_source::stop()
{
  // Toggle thread stop condition
  g_th_run = false;
  // Join event loop thread
  g_rx_thread->join();
};

void
zmq_source::run()
{
  // Construct the URI
  std::string addr = "tcp://" + s_host + ":" + s_port;
  logger.info("Remote client address: " + addr);

  // Specify a timeout and configure the socket
  socket.setsockopt(ZMQ_RCVTIMEO, 100);
  // Connect to remote client
  socket.connect(addr.c_str());

  // Create temporary variables outsize the loop
  div_t type_size_div;
  // Received message flag
  int rc;

  // Event loop
  while (g_th_run)
  {
    // Receive information from the client
    rc = socket.recv(&message);

    // If we received a message and the thread should be alive
    if (rc and g_th_run)
    {
      // Divide the message size by the size of an IQ sample
      type_size_div = std::div(static_cast<int>(message.size()), sizeof(iq_sample));

      // If there's a remainder
      if (type_size_div.rem)
      {
        logger.warning("Incomplete message.");
      }

      // Write the amount of IQ samples to the buffer
      p_output_buffer->write(
          static_cast<iq_sample *>(message.data()),
          type_size_div.quot);
    }
    // Clear message data
    message.rebuild();
  }

  // Close the ZMQ primitives
  socket.close();
  // context.close();

  // Output debug information
  logger.info("Stopped socket.");
};


zmq_sink::zmq_sink(std::shared_ptr<hydra_buffer<iq_sample>> input_buffer,
                   const std::string& server_addr,
                   const std::string& remote_addr,
                   const std::string& port):
  p_input_buffer(input_buffer),
  s_host(server_addr),
  s_port(port),
  g_th_run(true),
  socket(context, ZMQ_PUSH)
{
  // Create a thread to receive the data
  g_rx_thread = std::make_unique<std::thread>(&zmq_sink::run, this);

  logger = hydra_log("zmq|sink");
};

void
zmq_sink::stop()
{
  // Toggle thread stop condition
  g_th_run = false;
  // Join event loop thread
  g_rx_thread->join();
};

// Assign the handle receive callback when a datagram is received
void
zmq_sink::run()
{
  // Construct the URI
  std::string addr = "tcp://" + s_host + ":" + s_port;
  logger.info("Local server address: " + addr);

  // Set timeout for send operation
  socket.setsockopt(ZMQ_SNDTIMEO, 100);
  // Bind address
  socket.bind(addr.c_str());

  // Initialize temporary variables outside the loop
  unsigned int u_buffer_size;
  unsigned int u_message_size;

  // Event loop
  while (g_th_run)
  {
    // Get the buffer size
    u_buffer_size = p_input_buffer->size();
    // If there is anything to transmit
    if (u_buffer_size)
    {
      // Calculate the message size
      u_message_size = u_buffer_size * sizeof(iq_sample);
      // Allocate enough memory for this message
      message.rebuild(u_message_size);

      // Copy the samples onto the message object
      std::memcpy(message.data(),
          &p_input_buffer->read(u_buffer_size).front(),
          u_message_size);

      // If we are still running
      if (g_th_run)
      {
        // Send message to client
        socket.send(message);
      }
    } // If buffer
    // There's nothing to trasmit
    else
    {
      // Sleep for a microssecond and prevent this thread to explode
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  } // while

  // Close the ZMQ primitives
  socket.close();
  // context.close();

  // Output debug information
  logger.info("Stopped socket.");
}

tcp_sink::tcp_sink(
  sample_stream *input_buffer,
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
      boost::asio::write(*p_socket, boost::asio::buffer(output_buffer));
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
  sample_stream *input_buffer,
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
  iq_sample output_buffer[BUFFER_SIZE];

  while (g_th_run)
  {
    // If there is anything to transmit
    if (g_input_buffer->size() > 0)
    {
      size_t n_elemns = 0;

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
      size_t total_size_bytes =  n_elemns * IQ_SIZE;

      // Send from output_buffer
      while (bytes_sent < total_size_bytes)
      {
        size_t bytes_to_send = std::min(BUFFER_SIZE * IQ_SIZE, (total_size_bytes - bytes_sent));
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

  sample_stream* buffer = server.buffer();

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
