#define BOOST_TEST_MODULE hydra_resource_tests
#include <boost/test/included/unit_test.hpp>

#include "hydra/hydra_resource.h"
#include "qa_helpers.h"

#include <initializer_list>

namespace {

void expect_chunks(hydra::rf_front_end &front_end,
                   std::initializer_list<hydra::chunk> expected)
{
  const auto actual = front_end.list_chunks();
  BOOST_REQUIRE_EQUAL(actual.size(), expected.size());

  auto entry = actual.begin();
  for (const auto &chunk : expected)
  {
    BOOST_CHECK_EQUAL(entry->second.get<unsigned int>("id"), chunk.u_id);
    BOOST_CHECK_EQUAL(entry->second.get<double>("centre_freq"), chunk.d_centre_freq);
    BOOST_CHECK_EQUAL(entry->second.get<double>("bandwidth"), chunk.d_bandwidth);
    ++entry;
  }
}

} // namespace

BOOST_AUTO_TEST_CASE(release_merges_free_neighbors)
{
  hydra::hydra_log logger;

  for (const auto &allocation : {hydra::chunk(100, 100, 1),
                                hydra::chunk(75, 50, 1),
                                hydra::chunk(125, 50, 1),
                                hydra::chunk(100, 20, 1)})
  {
    hydra::rf_front_end front_end(100, 100, 0, &logger);
    BOOST_REQUIRE_EQUAL(front_end.create_chunks(allocation.d_centre_freq,
                                                allocation.d_bandwidth,
                                                allocation.u_id), 0);
    front_end.delete_chunks(1);
    expect_chunks(front_end, {{100, 100, 0}});
  }
}

BOOST_AUTO_TEST_CASE(release_preserves_occupied_neighbors)
{
  hydra::hydra_log logger;

  for (unsigned int first : {1u, 2u})
  {
    hydra::rf_front_end front_end(100, 100, 0, &logger);
    BOOST_REQUIRE_EQUAL(front_end.create_chunks(75, 50, 1), 0);
    BOOST_REQUIRE_EQUAL(front_end.create_chunks(125, 50, 2), 0);

    front_end.delete_chunks(first);
    expect_chunks(front_end, {{75, 50, first == 1 ? 0u : 1u},
                              {125, 50, first == 2 ? 0u : 2u}});
    front_end.delete_chunks(3 - first);
    expect_chunks(front_end, {{100, 100, 0}});
  }
}

BOOST_AUTO_TEST_CASE(release_removes_all_chunks_owned_by_radio)
{
  hydra::hydra_log logger;
  hydra::rf_front_end front_end(100, 100, 0, &logger);
  BOOST_REQUIRE_EQUAL(front_end.create_chunks(75, 50, 1), 0);
  BOOST_REQUIRE_EQUAL(front_end.create_chunks(125, 50, 1), 0);

  front_end.delete_chunks(1);
  expect_chunks(front_end, {{100, 100, 0}});
}

BOOST_AUTO_TEST_CASE(unconfigured_direction_release_preserves_active_allocation)
{
  for (bool receiver : {false, true})
  {
    hydra::xvl_resource_manager manager;
    if (receiver)
    {
      manager.set_rx_resources(100, 100);
      BOOST_REQUIRE_EQUAL(manager.reserve_rx_resources(1, 100, 100), 0);
      BOOST_CHECK_EQUAL(manager.free_tx_resources(1), 0);
    }
    else
    {
      manager.set_tx_resources(100, 100);
      BOOST_REQUIRE_EQUAL(manager.reserve_tx_resources(1, 100, 100), 0);
      BOOST_CHECK_EQUAL(manager.free_rx_resources(1), 0);
    }

    const auto resources = manager.query_resources();
    const auto &chunks = resources.get_child(receiver ? "receiver" : "transmitter");
    BOOST_REQUIRE_EQUAL(chunks.size(), 1);
    BOOST_CHECK_EQUAL(chunks.front().second.get<unsigned int>("id"), 1);
  }
}

BOOST_AUTO_TEST_CASE(reconfiguration_commits_only_after_the_dsp_update_succeeds)
{
  hydra_test::QuietLogs quiet;
  for (const bool receiver : {false, true})
  {
    hydra::xvl_resource_manager manager;
    if (receiver) manager.set_rx_resources(100, 100);
    else manager.set_tx_resources(100, 100);
    auto reserve = [&](unsigned int id, double cf) {
      return receiver ? manager.reserve_rx_resources(id, cf, 50)
                      : manager.reserve_tx_resources(id, cf, 50);
    };
    auto replace = [&](double cf, const std::function<bool()>& apply) {
      return receiver ? manager.reconfigure_rx_resources(1, cf, 50, apply)
                      : manager.reconfigure_tx_resources(1, cf, 50, apply);
    };
    BOOST_REQUIRE_EQUAL(reserve(1, 75), 0);
    BOOST_REQUIRE_EQUAL(reserve(2, 125), 0);
    const auto occupied = manager.query_resources();
    bool called = false;
    BOOST_CHECK_EQUAL(replace(125, [&] { called = true; return true; }), 1);
    BOOST_CHECK(!called);
    BOOST_CHECK(manager.query_resources() == occupied);
    BOOST_CHECK(!(receiver ? manager.check_rx_free(125, 50, 1)
                          : manager.check_tx_free(125, 50, 1)));

    if (receiver) manager.free_rx_resources(2);
    else manager.free_tx_resources(2);
    const auto before = manager.query_resources();
    BOOST_CHECK_EQUAL(replace(125, [] { return false; }), 1);
    BOOST_CHECK(manager.query_resources() == before);
    BOOST_CHECK_THROW(replace(125, []() -> bool {
      throw std::runtime_error("DSP preparation failed");
    }), std::runtime_error);
    BOOST_CHECK(manager.query_resources() == before);
    BOOST_REQUIRE_EQUAL(replace(125, [] { return true; }), 0);
    const auto after = manager.query_resources();
    const auto& chunks = after.get_child(receiver ? "receiver" : "transmitter");
    BOOST_REQUIRE_EQUAL(chunks.size(), 2);
    BOOST_CHECK_EQUAL(chunks.front().second.get<unsigned int>("id"), 0);
    BOOST_CHECK_EQUAL(chunks.back().second.get<unsigned int>("id"), 1);
    BOOST_CHECK_EQUAL(chunks.back().second.get<double>("centre_freq"), 125);
  }
}
