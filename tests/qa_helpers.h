#ifndef HYDRA_QA_HELPERS_H
#define HYDRA_QA_HELPERS_H

#include "hydra/hydra_uhd_interface.h"

#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <chrono>
#include <thread>

namespace hydra_test {

struct QuietLogs
{
  bool previously_enabled = boost::log::core::get()->set_logging_enabled(false);
  ~QuietLogs()
  {
    boost::log::core::get()->set_logging_enabled(previously_enabled);
  }
};

class IdleDevice : public hydra::abstract_device
{
public:
  void send(const hydra::iq_window&, size_t) override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  size_t receive(hydra::iq_window&, size_t) override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return 0;
  }
};

inline unsigned short unused_tcp_port()
{
  boost::asio::io_service io;
  boost::asio::ip::tcp::acceptor reservation(
      io, {boost::asio::ip::address_v4::loopback(), 0});
  return reservation.local_endpoint().port();
}

}

#endif
