#include "hydra/hydra_client.h"
#include <iostream>

using namespace std;
int main()
{
  std::string lalala;
  // Request resources
  hydra::hydra_client s1 = hydra::hydra_client("127.0.0.1", 5000, 1, true);
  lalala = s1.request_tx_resources(2007.5e6, 5e6, false);
  std::cout << lalala << std::endl;

  // Request resources
  hydra::hydra_client s2 = hydra::hydra_client("127.0.0.1", 5000, 2, true);
  lalala = s2.request_tx_resources(1992.5e6, 5e6, false);
  std::cout << lalala << std::endl;

  hydra::hydra_client s3 = hydra::hydra_client("127.0.0.1", 5000, 3, true);
  // Request resources
  lalala = s3.request_tx_resources(2000.0e6, 10e6, false);
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
