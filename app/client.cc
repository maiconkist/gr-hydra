#include "hydra/hydra_client.h"
#include <iostream>

using namespace std;
int main()
{
  std::string lalala;
#if 0
  // Request resources
  hydra::hydra_client s1 = hydra::hydra_client("127.0.0.1", 5000, 1, true);
  lalala = s1.request_tx_resources(2e9, 1e3, false);
  std::cout << lalala << std::endl;

  // Request resources
  hydra::hydra_client s2 = hydra::hydra_client("127.0.0.1", 5000, 2, true);
  lalala = s2.request_tx_resources(2e9 + 2e3, 1e3, false);
  std::cout << lalala << std::endl;
#endif

  hydra::hydra_client s3 = hydra::hydra_client("127.0.0.1", 5000, 1, true);
  // Request resources
  lalala = s3.request_rx_resources(2e9 - 10e3, 100e3, false);
  std::cout << lalala << std::endl;

#if 0
  // Query available resources
  lalala = s3.query_resources();
  std::cout << lalala << std::endl;

  // Free resources from a given service
  lalala = s1.free_resources();
  std::cout << lalala << std::endl;

  // Query available resources
  lalala = s2.query_resources();
  std::cout << lalala << std::endl;

#endif

  return 0;
}
