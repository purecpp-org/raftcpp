#include <iostream>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "../src/node/node.h"
#include "common/type_def.h"
#include "doctest.h"

// nanolog INFO (WARN) and doctest INFO (WARN) conflict
#ifdef INFO
#undef INFO
#endif

#ifdef WARN
#undef WARN
#endif

class TestRaftNode {
    public:
    static void SetUpTestCase() {
        std::cout << "TestRaftNode SetUpTestCase" << std::endl;
    }

    static void TearDownTestCase() {
        std::cout << "TestRaftNode TearDownTestCase" << std::endl;
    }
};

void TestErrStateParameter() {
    std::string address = "127.0.0.1 ";
    int port = 9000;

    const auto state = raftcpp::RaftState::CANDIDATE;
    raftcpp::node::RaftNode node{address, port, state};
}

void LeaderNode() {
    std::string address = "127.0.0.1 ";
    int port = 9000;

    const auto state = raftcpp::RaftState::LEADER;
    raftcpp::node::RaftNode node{address, port, state};
    node.start();
}

void FollowerNode() {
    std::string address = "127.0.0.1 ";
    int port = 9000;

    const auto state = raftcpp::RaftState::FOLLOWER;
    raftcpp::node::RaftNode node{address, port, state};
    node.start();
}

TEST_CASE_FIXTURE(TestRaftNode, "TestErrStateParameter") {
    // LeaderNode();   //Endless loop
    // FollowerNode(); //Endless loop
    REQUIRE_THROWS(TestErrStateParameter());
}
