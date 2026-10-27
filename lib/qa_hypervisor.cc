#include "hydra/hydra_hypervisor.h"
#include "hydra/hydra_core.h"

#include <iostream>
#include <stdexcept>

int main()
{
  try
  {
    hydra::Hypervisor hypervisor;
    if (hypervisor.detach_virtual_radio(42))
      throw std::runtime_error("Detaching an unknown radio should return false");

    auto radio = std::make_shared<hydra::VirtualRadio>(42, &hypervisor);
    hypervisor.attach_virtual_radio(radio);
    if (not hypervisor.detach_virtual_radio(42))
      throw std::runtime_error("Unable to detach an attached radio");
    if (hypervisor.detach_virtual_radio(42))
      throw std::runtime_error("Detaching the same radio again should return false");

    hydra::HydraCore core;
    if (core.request_rx_resources(1, 100, 10, "127.0.0.1", "127.0.0.1") != 0 ||
        core.request_tx_resources(1, 100, 10, "127.0.0.1", "127.0.0.1") != 0)
      throw std::runtime_error("An unconfigured core accepted an allocation");
    core.stop();
    if (core.free_resources(1) != 1)
      throw std::runtime_error("Releasing an unconfigured core should succeed");
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << std::endl;
    return 1;
  }
  return 0;
}
