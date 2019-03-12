#include "hydra/hydra_client.h"

#include <iostream>
#include <signal.h>
#include <boost/format.hpp>



using namespace std;

int main()
{
  double cf = 1.1e9;

  std::string lalala;
  // Request resources
  std::cout << boost::format("------------- Requesting %1%, %2%") % (cf+200e3) % 200e3 << std::endl;
  hydra::hydra_client s1 = hydra::hydra_client("127.0.0.1", 5000, 90, true);
  std::cout<< s1.check_connection() << std::endl;
  std::cout<< s1.query_resources() << std::endl;

  hydra::rx_configuration tx1{cf + 200e3, 200e3, false};
  std::cout << s1.request_tx_resources(tx1) << std::endl;

#if 2
  // Request resources
  std::cout << boost::format("------------- Requesting %1%, %2%") % (cf-200e3) % 100e3 << std::endl;
  hydra::hydra_client s2 = hydra::hydra_client("127.0.0.1", 5000, 91, true);

  hydra::rx_configuration tx2{cf - 200e3, 100e3, false};
  std::cout << s2.check_connection() << std::endl;
  std::cout << s2.request_tx_resources(tx2) << std::endl;
#endif

  // Free resources from a given service
  sleep(1);
  std::cout << s1.free_resources() << std::endl;
  std::cout << s1.query_resources() << std::endl;

#if 2
  // Query available resources
  std::cout << s2.free_resources() << std::endl;
  std::cout << s2.query_resources() << std::endl;
#endif


  std::cout << "Press CTRL-C to quit" << std::endl;
  while (1) usleep(1000);

  return 0;
}
