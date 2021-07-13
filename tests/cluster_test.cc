#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "util.h"

// TODO cluster tests should be independent of the raft
TEST_CASE("test_cluster") {
    Cluster cl(3);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    REQUIRE_EQ(cl.CheckOneLeader(), true);

    // block the leader
    std::vector<int> first_leader = cl.GetLeader();
    cl.BlockNode(first_leader[0]);

    // wait for the election
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // check blocked nodes
    std::set<int> blocked_nodes = cl.GetBlockedNodes();
    REQUIRE_EQ(blocked_nodes.size(), 1);
    REQUIRE_NE(blocked_nodes.find(first_leader[0]), blocked_nodes.end());

    // get another leader
    REQUIRE_EQ(cl.CheckOneLeader(), true);
    std::vector<int> second_leader = cl.GetLeader();

    // ensure they are not the same leader
    REQUIRE_NE(first_leader[0], second_leader[0]);

    // block the second leader
    cl.BlockNode(second_leader[0]);

    // wait for the election
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // check blocked nodes
    blocked_nodes = cl.GetBlockedNodes();
    REQUIRE_EQ(blocked_nodes.size(), 2);
    REQUIRE_NE(blocked_nodes.find(first_leader[0]), blocked_nodes.end());
    REQUIRE_NE(blocked_nodes.find(second_leader[0]), blocked_nodes.end());

    // there should be no leader
    REQUIRE_FALSE(cl.CheckOneLeader());

    // unblock node
    cl.UnblockNode(first_leader[0]);
    blocked_nodes = cl.GetBlockedNodes();
    REQUIRE_EQ(blocked_nodes.size(), 1);
    REQUIRE_EQ(blocked_nodes.find(first_leader[0]), blocked_nodes.end());
    REQUIRE_NE(blocked_nodes.find(second_leader[0]), blocked_nodes.end());

    // wait for the election
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // check leader
    REQUIRE_EQ(cl.CheckOneLeader(), true);

    // unblock node
    cl.UnblockNode(second_leader[0]);
    blocked_nodes = cl.GetBlockedNodes();
    REQUIRE_EQ(blocked_nodes.size(), 0);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    // ensure it's a stable cluster
    REQUIRE_EQ(cl.CheckOneLeader(), true);
}