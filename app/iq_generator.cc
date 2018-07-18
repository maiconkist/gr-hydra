#include <iostream>
#include <complex>
#include <string>
#include <chrono>
#include <array>
#include <thread>
#include <math.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "hydra/hydra_client.h"

using boost::asio::ip::udp;

class UDPClient
{
private:
	boost::asio::io_service& io_service_;
	udp::socket socket_;
	udp::endpoint endpoint_;

public:
  // Constructor
	UDPClient(boost::asio::io_service& io_service,
            const std::string& host,
            const std::string& port
	) : io_service_(io_service), socket_(io_service, udp::endpoint(udp::v4(), 0))
  {
		udp::resolver resolver(io_service_);
		udp::resolver::query query(udp::v4(), host, port);
		udp::resolver::iterator iter = resolver.resolve(query);
		endpoint_ = *iter;
	}
  // Destructor
	~UDPClient()
	{
		socket_.close();
	}

  // Send data constantly
  template<long unsigned int SIZE> // Using a template as we don t know the
	void send(std::array<std::complex<double>, SIZE>& msg, double rate)
  {
    // Get the time between frame
    long int threshold = lrint(msg.size() / rate);

    size_t counter = 0;
    while (true)
    {
      // Sleep and wait for the right time

      std::this_thread::sleep_for(std::chrono::microseconds(threshold));
      // Send the information
      std::cout << "Sending samples " << counter++
                << ", msg size: " << msg.size()
                << std::endl;
      socket_.send_to(boost::asio::buffer(msg, sizeof(msg[0]) * msg.size()), endpoint_);
    }
	}
};

int main(int argc, char* argv[])
{
  // Default variables
  std::string host = "localhost";
  std::string port = "5000";
  std::string rate = "1e6"; // In Msps - defaults to 1 sample per second
  const unsigned int p_size = 100;

  // Shittiest way to parse CLI argument ever, but does the drill
  if (argc % 2 == 0)
  {
    std::cout << "Wrong number of arguments" << std::endl;
    return 1;
  }
  // Iterate over the arguments and change the default variables accordingly
  for (int i = 1; i < argc; i+=2)
  {
    switch (argv[i][1])
    {
    case 'h':
      host = argv[i + 1];
      break;
    case 'p':
      port = argv[i + 1];
      break;
    case 'r':
      rate = argv[i + 1];
      break;

    default: break;
    }
  }

  // Request resources
  std::string lalala;
  xvl_client s1 = xvl_client("127.0.0.1", 5000, 1, true);
  lalala = s1.request_tx_resources(2e9 + 1e6, std::stof(rate));
  std::cout << lalala << std::endl;

  // Initialise the async IO service
  boost::asio::io_service io_service;
	UDPClient client(io_service, host, "7000");

  std::cout << "FFT Size: " << p_size << "\tSampling rate: " << rate << "\tThreshold: " << p_size * 1e6 / std::stod(rate) << std::endl;
  // Construct the payload array
  std::cout << "1"  << std::endl;
  std::array<std::complex<double>, p_size> payload;
  // Fill the array with IQ samples
  std::cout << "2"  << std::endl;
  payload.fill(std::complex<double>(2.0, 1.0));
  std::cout << "3"  << std::endl;
  // Send the payload at the given rate
	client.send(payload, std::stod(rate));
}
