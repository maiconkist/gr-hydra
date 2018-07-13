#include "hydra/hydra_client.h"
#include <iostream>

using namespace std;
int main()
{
  std::string lalala;

  // Request resources
  xvl_client s1 = xvl_client("127.0.0.1", 5000, 1, true);
  lalala = s1.request_rx_resources(2007.5e6, 5e6);
  std::cout << lalala << std::endl;

  // Request resources
  xvl_client s2 = xvl_client("127.0.0.1", 5000, 2, true);
  lalala = s2.request_rx_resources(1992.5e6, 5e6);
  std::cout << lalala << std::endl;


  xvl_client s3 = xvl_client("127.0.0.1", 5000, 3, true);
  // Request resources
  lalala = s3.request_tx_resources(2000.0e6, 10e6);
  std::cout << lalala << std::endl;

  // Query available resources
  lalala = s3.query_resources();
  std::cout << lalala << std::endl;

  // Free resources from a given service
  lalala = s1.free_resources();
  std::cout << lalala << std::endl;

  // Query available resources
  lalala = s2.query_resources();
  std::cout << lalala << std::endl;

  return 0;
}
