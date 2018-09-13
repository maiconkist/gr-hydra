#include "hydra/hydra_client.h"


#include <iostream>
#include <signal.h>
#include <boost/format.hpp>


hydra::hydra_client s1, s2, s3;

void my_handler(int s)
{
  printf("Caught signal %d\n",s);
  s3.free_resources();
  exit(1);
}


using namespace std;

int main()
{
  signal (SIGINT, my_handler);

  double cf = 950e6;

  std::string lalala;
  // Request resources
  std::cout << boost::format("------------- Requesting %1%, %2%") % (cf+200e3) % 200e3 << std::endl;
  s1 = hydra::hydra_client("127.0.0.1", "127.0.0.1", 5000, 90, true);
  std::cout << s1.query_resources() << std::endl;
  std::cout << s1.request_rx_resources(cf + 200e3, 200e3, false) << std::endl;

  // Request resources
  std::cout << boost::format("------------- Requesting %1%, %2%") % (cf-200e3) % 100e3 << std::endl;
  s2 = hydra::hydra_client("127.0.0.1", "127.0.0.1", 5000, 91, true);
  std::cout << s2.request_rx_resources(cf - 200e3, 100e3, false) << std::endl;

  std::cout << boost::format("------------- Requesting %1%, %2%") % (cf) % 100e3 << std::endl;
  s3 = hydra::hydra_client("127.0.0.1", "127.0.0.1", 5000, 93, true);
  std::cout << s3.request_rx_resources(cf, 100e3, false) << std::endl;

  // Free resources from a given service
  std::cout <<  s1.free_resources() << std::endl;
  std::cout << s1.query_resources() << std::endl;

  // Query available resources
  std::cout << s2.free_resources() << std::endl;
  std::cout << s2.query_resources() << std::endl;


  std::cout << "Press CTRL-C to quit" << std::endl;
  while (1) usleep(1000);

  return 0;
}
