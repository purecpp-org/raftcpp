#include "../doctest/doctest.hpp"
#include "../../raft_node.hpp"
#include <chrono>

TEST_CASE("test heartbeat timer") {
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
    node.handle_vote_response({ 0, true });
    CHECK(node.state() == State::LEADER);

    conf.disable_election_timer = false;
    node.start_heartbeat_timer();
    node.stepdown(0);
}

TEST_CASE("test election timer") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };

    conf.self_addr = conf.all_peers[0];
    conf.disable_election_timer = false;
    raft_node node(conf);
    node.async_run();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_CASE("test vote timer") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };

    conf.self_addr = conf.all_peers[0];
    conf.disable_election_timer = false;
    raft_node node(conf);
    node.handle_prevote_response({ 0, true });
    CHECK(node.state() == State::CANDIDATE);
    node.async_run();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}