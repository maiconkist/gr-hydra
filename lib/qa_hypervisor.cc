#include "hydra/hydra_hypervisor.h"

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
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << std::endl;
    return 1;
  }
  return 0;
}
