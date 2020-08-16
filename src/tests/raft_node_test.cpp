#include <iostream>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "../raft_node.hpp"

//nanolog INFO (WARN) and doctest INFO (WARN) conflict
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

    raftcpp::node::State state = raftcpp::node::Candidate;
    raftcpp::node::RaftNode node{address, port, state};
}

void LeaderNode() {
    std::string address = "127.0.0.1 ";
    int port = 9000;

    raftcpp::node::State state = raftcpp::node::Leader;
    raftcpp::node::RaftNode node{address, port, state};
    node.start();
}

void FollowerNode() {
    std::string address = "127.0.0.1 ";
    int port = 9000;

    raftcpp::node::State state = raftcpp::node::Follower;
    raftcpp::node::RaftNode node{address, port, state};
    node.start();
}

TEST_CASE_FIXTURE (TestRaftNode, "TestErrStateParameter") {
    //LeaderNode();   //Endless loop
    //FollowerNode(); //Endless loop
    REQUIRE_THROWS(TestErrStateParameter());
}
