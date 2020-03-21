#pragma once
#include "../doctest/doctest.hpp"
#include "../../raft_node.hpp"
#include <chrono>

TEST_CASE("test request prevote no peers") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.self_addr = conf.all_peers[0];
    raft_node node(conf);
    bool prevote = true;
    for (size_t i = 0; i < 3; i++) {
        node.request_vote(prevote);
        CHECK(node.state() == State::FOLLOWER);
        CHECK(node.current_term() == 0);
    }
}

TEST_CASE("test request vote no peers") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.self_addr = conf.all_peers[0];
    raft_node node(conf);
    bool prevote = false;
    for (size_t i = 0; i < 3; i++) {
        node.request_vote(prevote);
        CHECK(node.current_term() == i + 1);
    }
}

TEST_CASE("test handle vote response modify parameters") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.self_addr = conf.all_peers[0];

    raft_node node(conf);
    node.handle_prevote_response({ 0, true });
    CHECK(node.state() == State::CANDIDATE);
    CHECK(node.current_term() == 1);

    //test exception
    node.handle_prevote_response({ 0, true });
    CHECK(node.state() == State::CANDIDATE);
    CHECK(node.current_term() == 1);

    node.handle_vote_response({ 0, true });
    CHECK(node.state() == State::LEADER);
    CHECK(node.current_term() == 1);

    //test exception
    node.handle_vote_response({ 0, true });
    CHECK(node.state() == State::LEADER);
    CHECK(node.current_term() == 1);
}

TEST_CASE("test stepdown when follower") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.self_addr = conf.all_peers[0];

    raft_node node(conf);

    node.handle_prevote_response({ 0, false });
    CHECK(node.state() == State::FOLLOWER);
    CHECK(node.current_term() == 0);

    //test stepdown
    node.handle_prevote_response({ 1, false });
    CHECK(node.state() == State::FOLLOWER);
    CHECK(node.current_term() == 1); 

    node.handle_prevote_response({ 2, true });
    CHECK(node.state() == State::FOLLOWER);
    CHECK(node.current_term() == 2);
}

TEST_CASE("test stepdown when cadidate") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.self_addr = conf.all_peers[0];

    raft_node node(conf);

    node.handle_prevote_response({ 0, true });
    CHECK(node.state() == State::CANDIDATE);
    CHECK(node.current_term() == 1);

    node.handle_vote_response({ 2, false });
    CHECK(node.state() == State::FOLLOWER);
    CHECK(node.current_term() == 2);
}

TEST_CASE("test stepdown when leader") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.self_addr = conf.all_peers[0];

    raft_node node(conf);

    node.handle_prevote_response({ 0, true });
    CHECK(node.state() == State::CANDIDATE);
    CHECK(node.current_term() == 1);

    node.handle_vote_response({ 0, true });
    CHECK(node.state() == State::LEADER);
    CHECK(node.current_term() == 1);

    node.stepdown(2);
    CHECK(node.state() == State::FOLLOWER);
    CHECK(node.current_term() == 2);
}