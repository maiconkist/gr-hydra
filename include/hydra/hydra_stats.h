#ifndef HYDRA_STATS_INCLUDE_H
#define HYDRA_STATS_INCLUDE_H

#include <string>
#include <deque>
#include <complex>
#include <thread>
#include <iostream>
#include <zmq.hpp>
#include <list>


#include "hydra/types.h"
#include "hydra/hydra_log.h"

namespace hydra {

// Class that creates a monitoring service
class xvl_monitor
{
 private:
  // Server port
  std::string s_server_port;
  // Receive measurements
  void start();
  // Stats thread
  std::unique_ptr<std::thread> stats_thread;

  hydra_log logger;

 public:
  // Constructor
  xvl_monitor(unsigned int u_port = 4996);
  // Destructor
  ~xvl_monitor(){};

  // Run a thread to start receiving the measurements
  void run(void);
};

// Report messages back to the stats server
class xvl_report
{
 private:
  // Server port
  std::string s_server_port;
  // Identifier
  std::string s_id;
  // Pointers to the context and socket
  std::unique_ptr<zmq::context_t> context;
  std::unique_ptr<zmq::socket_t> socket;

  // Pointer to the message thread
  std::unique_ptr<std::thread> message_thread;
  // Thread stop condition
  bool thr_stop;

public:
  // Default constructor
  xvl_report(){};
  // Constructor
  xvl_report(unsigned int u_type,
             sample_stream* buffer,
             unsigned int u_port = 4996);
  // Destructor
  ~xvl_report()
  {
    // Stop the thread
    thr_stop = true;
    // Join thread
    message_thread->join();
    // Reset pointer
    message_thread.reset();

    // Reset the socket and the context pointers
    socket.reset();
    context.reset();
  };
  // Publish a given message to the server
  void push(sample_stream* buffer);
};

typedef std::unique_ptr<xvl_report> ReportPtr;

} // namespace hydra

#endif
