#include "hydra/hydra_stats.h"


namespace hydra {

xvl_monitor::xvl_monitor(unsigned int u_port)
{
  // Get the server port
  s_server_port = std::to_string(u_port);
}

void xvl_monitor::start(void)
{
  //  Prepare our context and socket
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_PULL);
  socket.bind (("tcp://*:" + s_server_port).c_str());

  // Message type object
  zmq::message_t request;

  // Output info message
  std::cout << "Waiting for statistics" << std::endl;

  // Flag mark if it is a new virtual radio
  bool inside;
  // The list of virtual radio statistic
  std::list<std::string> x;
  // Message holder
  std::string s_message;
  // Print initial message
  std::cout << "Stats:" << std::endl;
  // Run indefinitely
  while(true)
  {
    //  Wait for next request from client -- this is blocking
    socket.recv (&request);

    // Receive one message
    s_message = std::string(static_cast<char*>(request.data()), request.size());

    // Reset the flag to its default value
    inside = false;
    // Iterate over the list of VRs
    for (std::list<std::string>::iterator a = x.begin(); a != x.end(); a++)
    {
      // TODO do a better check
      if (s_message.data()[0] == a->data()[0])
      {
        // Replace the content with the new value
        a->assign(s_message);
        // Toggle the flag
        inside = true;
      }
    }
    // If it is a new radio
    if (not inside)
    {
      // Append it to the list of VRs
      x.push_back(s_message);
    }

    // Iterate over the list of VRs
    for (std::list<std::string>::iterator a = x.begin(); a != x.end(); a++)
    {
      // Print the stats
      std::cout << "Radio:\t" << a->data() << "\t";
    }
    // Carriage return at the end to overwrite last line
    std::cout << "\r" << std::flush;

  }
}

void xvl_monitor::run(void)
{
  // Run the statistics reporting server
  stats_thread = std::make_unique<std::thread>(&xvl_monitor::start, this);
}

xvl_report::xvl_report(unsigned int u_type,
                       iq_stream* buffer,
                       unsigned int u_port)
{
  // Get the server port
  s_server_port = std::to_string(u_port);
  // Get the identifier
  s_id = std::to_string(u_type);

  //  Prepare our context and socket
  context = std::make_unique<zmq::context_t>(1);
  socket = std::make_unique<zmq::socket_t>(*context, ZMQ_PUSH);
  // Connect the socket to the stats server
  socket->connect(("tcp://127.0.0.1:" + s_server_port).c_str());
  // Create thread to receive messages
  message_thread = std::make_unique<std::thread>(&xvl_report::push,
                                                 this,
                                                 buffer);
}

void xvl_report::push(iq_stream* buffer)
{
  // Thread stop condition
  thr_stop = false;
  // Create string object
  std::string s_message;
  // Run until thread deletion
  while (true)
  {
    // Sleep for a second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // If the destructor has been called
    if (thr_stop){return;}
    // Get the size of the buffer as a string
    s_message = s_id + "\t" + std::to_string((long long int) buffer->size());
    // Message type object
    zmq::message_t request(s_message.size());
    // Cope content to the request message
    memcpy(request.data(), s_message.data(), s_message.size());
    // Send message
    socket->send(request);
  }
}


} // namespace hydra
