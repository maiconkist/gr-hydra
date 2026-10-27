#define BOOST_TEST_MODULE hydra_hypervisor_tests
#include <boost/test/included/unit_test.hpp>

#include "hydra/hydra_core.h"
#include "hydra/hydra_hypervisor.h"

#include <memory>

BOOST_AUTO_TEST_CASE(unknown_and_repeated_detach_are_safe)
{
  hydra::Hypervisor hypervisor;
  BOOST_CHECK(not hypervisor.detach_virtual_radio(42));

  auto radio = std::make_shared<hydra::VirtualRadio>(42, &hypervisor);
  hypervisor.attach_virtual_radio(radio);
  BOOST_REQUIRE(hypervisor.detach_virtual_radio(42));
  BOOST_CHECK(not hypervisor.detach_virtual_radio(42));
}

BOOST_AUTO_TEST_CASE(unconfigured_core_cleanup_is_safe)
{
  hydra::HydraCore core;
  BOOST_CHECK_EQUAL(core.request_rx_resources(1, 100, 10, "127.0.0.1", "127.0.0.1"), 0);
  BOOST_CHECK_EQUAL(core.request_tx_resources(1, 100, 10, "127.0.0.1", "127.0.0.1"), 0);
  core.stop();
  BOOST_CHECK_EQUAL(core.free_resources(1), 1);
}
