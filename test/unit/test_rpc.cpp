#include "../doctest/doctest.hpp"
#include "../../raft_node.hpp"
#include <chrono>

TEST_CASE("test heartbeat") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };

    conf.self_addr = conf.all_peers[0];
    raft_node node(conf);
    conf.self_addr = conf.all_peers[1];
    raft_node node2(conf);
    node.async_run();
    node2.async_run();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    node.send_heartbeat();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    node.request_vote();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    node.request_prevote();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

TEST_CASE("test election") {
    std::cout << "-----test election-----\n";
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.disable_election_timer = false;
    conf.self_addr = conf.all_peers[0];
    raft_node node(conf);

    raft_config conf2 = conf;
    conf2.self_addr = conf2.all_peers[1];
    raft_node node2(conf2);

    node.async_run();
    node2.async_run();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

TEST_CASE("test always prevote") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.disable_election_timer = false;
    conf.self_addr = conf.all_peers[0];
    raft_node node(conf);

    raft_config conf2 = conf;
    conf2.self_addr = conf2.all_peers[1];
    raft_node node2(conf2);
    node2.always_prevote_granted = false;
    node.async_run();
    node2.async_run();

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}

TEST_CASE("test always vote") {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.disable_election_timer = false;
    conf.self_addr = conf.all_peers[0];
    raft_node node(conf);

    raft_config conf2 = conf;
    conf2.self_addr = conf2.all_peers[1];
    raft_node node2(conf2);
    node2.always_vote_granted = false;
    node.async_run();
    node2.async_run();

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}

TEST_CASE("test heartbeat timeout") {
    std::cout << "-----test heartbeat timeout; will block, need two or three nodes to test!-----\n";
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = {
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1},
        {"127.0.0.1", 8003, 2},
    };
    conf.disable_election_timer = false;
    std::string select;
    std::cin >> select;
    int sel = atoi(select.data());
    std::cout << "select " << sel << "\n";
    conf.self_addr = conf.all_peers[sel];
    raft_node node(conf);
    if(sel==0)
        node.let_heartbeat_timeout = true;

    node.run();
}