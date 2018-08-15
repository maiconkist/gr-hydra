#include "hydra/hydra_client.h"
#include <iostream>

using namespace std;
int main()
{

  double cf = 950e6;

  std::string lalala;
  // Request resources
  hydra::hydra_client s1 = hydra::hydra_client("127.0.0.1", 5000, 1, true);
  std::cout << s1.query_resources() << std::endl;
  std::cout << s1.request_rx_resources(cf + 200e3, 200e3, false) << std::endl;

  // Request resources
  hydra::hydra_client s2 = hydra::hydra_client("127.0.0.1", 5000, 2, true);
  std::cout << s2.request_rx_resources(cf - 200e3, 100e3, false) << std::endl;

#if 0
  hydra::hydra_client s3 = hydra::hydra_client("127.0.0.1", 5000, 3, true);
  std::cout << s3.request_rx_resources(cf, 100e3, false) << std::endl;
#endif


  // Free resources from a given service
  std::cout <<  s1.free_resources() << std::endl;
  std::cout << s1.query_resources() << std::endl;

  // Query available resources
  std::cout << s2.free_resources() << std::endl;
  std::cout << s2.query_resources() << std::endl;

  return 0;
}
