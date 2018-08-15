#include "hydra/hydra_client.h"
#include <iostream>
#include <signal.h>


void my_handler(int s)
{
  printf("Caught signal %d\n",s);
  exit(1);
}


using namespace std;

int main()
{
  signal (SIGINT, my_handler);

  double cf = 950e6;

  std::string lalala;
  // Request resources
  hydra::hydra_client s1 = hydra::hydra_client("127.0.0.1", 5000, 90, true);
  std::cout << s1.query_resources() << std::endl;
  std::cout << s1.request_rx_resources(cf + 200e3, 200e3, false) << std::endl;

  // Request resources
  hydra::hydra_client s2 = hydra::hydra_client("127.0.0.1", 5000, 91, true);
  std::cout << s2.request_rx_resources(cf - 200e3, 100e3, false) << std::endl;

  hydra::hydra_client s3 = hydra::hydra_client("127.0.0.1", 5000, 93, true);
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
