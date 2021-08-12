#include "gtest/gtest.h"
#include "util.h"

// TODO cluster tests should be independent of the raft
TEST(ClusterTest, TestBasic) {
    Cluster cl(3);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    ASSERT_EQ(cl.CheckOneLeader(), true);

    // block the leader
    std::vector<int> first_leader = cl.GetLeader();
    cl.BlockNode(first_leader[0]);

    // wait for the election
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // check blocked nodes
    std::set<int> blocked_nodes = cl.GetBlockedNodes();
    ASSERT_EQ(blocked_nodes.size(), 1);
    ASSERT_NE(blocked_nodes.find(first_leader[0]), blocked_nodes.end());

    // get another leader
    ASSERT_EQ(cl.CheckOneLeader(), true);
    std::vector<int> second_leader = cl.GetLeader();

    // ensure they are not the same leader
    ASSERT_NE(first_leader[0], second_leader[0]);

    // block the second leader
    cl.BlockNode(second_leader[0]);

    // wait for the election
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // check blocked nodes
    blocked_nodes = cl.GetBlockedNodes();
    ASSERT_EQ(blocked_nodes.size(), 2);
    ASSERT_NE(blocked_nodes.find(first_leader[0]), blocked_nodes.end());
    ASSERT_NE(blocked_nodes.find(second_leader[0]), blocked_nodes.end());

    // there should be no leader
    ASSERT_FALSE(cl.CheckOneLeader());

    // unblock node
    cl.UnblockNode(first_leader[0]);
    blocked_nodes = cl.GetBlockedNodes();
    ASSERT_EQ(blocked_nodes.size(), 1);
    ASSERT_EQ(blocked_nodes.find(first_leader[0]), blocked_nodes.end());
    ASSERT_NE(blocked_nodes.find(second_leader[0]), blocked_nodes.end());

    // wait for the election
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // check leader
    ASSERT_EQ(cl.CheckOneLeader(), true);

    // unblock node
    cl.UnblockNode(second_leader[0]);
    blocked_nodes = cl.GetBlockedNodes();
    ASSERT_EQ(blocked_nodes.size(), 0);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    // ensure it's a stable cluster
    ASSERT_EQ(cl.CheckOneLeader(), true);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
