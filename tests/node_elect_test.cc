#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <thread>

#include "../examples/counter/counter_service_def.h"
#include "../examples/counter/counter_state_machine.h"
#include "common/config.h"
#include "common/logging.h"
#include "node/node.h"
#include "rest_rpc/rpc_server.h"

class CounterServiceImpl {
public:
    // TODO(qwang): Are node and fsm uncopyable?
    CounterServiceImpl(std::shared_ptr<raftcpp::node::RaftNode> node,
                       std::shared_ptr<examples::counter::CounterStateMachine> &fsm)
        : node_(std::move(node)), fsm_(std::move(fsm)) {}

    void Incr(rpc_conn conn, int delta) {
        examples::counter::IncrRequest request = examples::counter::IncrRequest(delta);
        node_->Apply(request);
    }

    int64_t Get(rpc_conn conn) {
        // There is no need to gurantee the write-read consistency,
        // so we can get the value directly from this fsm instead of
        // apply it to all nodes.
        return fsm_->GetValue();
    }

private:
    std::shared_ptr<raftcpp::node::RaftNode> node_;
    std::shared_ptr<examples::counter::CounterStateMachine> fsm_;
};

void node_run(std::shared_ptr<raftcpp::node::RaftNode> &node, const std::string &conf_str,
              rpc_server *server) {
    const auto config = raftcpp::common::Config::From(conf_str);

    node = std::make_shared<raftcpp::node::RaftNode>((*server), config,
                                                     raftcpp::RaftcppLogLevel::RLL_DEBUG);
    auto fsm = std::make_shared<examples::counter::CounterStateMachine>();

    CounterServiceImpl service(node, fsm);
    server->register_handler("incr", &CounterServiceImpl::Incr, &service);
    server->register_handler("get", &CounterServiceImpl::Get, &service);
    server->run();

    return;
}

TEST_CASE("TestNodeElect") {
    int LeaderFlag = 0;
    std::vector<raftcpp::RaftState> nodeStateLeader;
    std::vector<raftcpp::RaftState> nodeStateFollower;

    std::shared_ptr<raftcpp::node::RaftNode> node1 = nullptr;
    std::shared_ptr<raftcpp::node::RaftNode> node2 = nullptr;
    std::shared_ptr<raftcpp::node::RaftNode> node3 = nullptr;

    rpc_server *server1 = new rpc_server(10001, std::thread::hardware_concurrency());
    rpc_server *server2 = new rpc_server(10002, std::thread::hardware_concurrency());
    rpc_server *server3 = new rpc_server(10003, std::thread::hardware_concurrency());

    std::thread t1(node_run, std::ref(node1),
                   "127.0.0.1:10001,127.0.0.1:10002,127.0.0.1:10003", std::ref(server1));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::thread t2(node_run, std::ref(node2),
                   "127.0.0.1:10002,127.0.0.1:10001,127.0.0.1:10003", std::ref(server2));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::thread t3(node_run, std::ref(node3),
                   "127.0.0.1:10003,127.0.0.1:10001,127.0.0.1:10002", std::ref(server3));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (true) {
        if (node1 && node2 && node3) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    nodeStateFollower.clear();
    nodeStateLeader.clear();
    if (node1->GetCurrState() == raftcpp::RaftState::FOLLOWER)
        nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
    else if (node1->GetCurrState() == raftcpp::RaftState::LEADER) {
        LeaderFlag = 1;
        nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
    }

    if (node2->GetCurrState() == raftcpp::RaftState::FOLLOWER)
        nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
    else if (node2->GetCurrState() == raftcpp::RaftState::LEADER) {
        LeaderFlag = 2;
        nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
    }

    if (node3->GetCurrState() == raftcpp::RaftState::FOLLOWER)
        nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
    else if (node3->GetCurrState() == raftcpp::RaftState::LEADER) {
        LeaderFlag = 3;
        nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
    }

    REQUIRE_EQ(nodeStateLeader.size(), 1);
    REQUIRE_EQ(nodeStateFollower.size(), 2);

    nodeStateFollower.clear();
    nodeStateLeader.clear();
    if (LeaderFlag == 1) {
        delete server1;
        server1 = nullptr;
        node1.reset();
        if (t1.joinable()) {
            t1.detach();
            t1.std::thread::~thread();
        }
    } else if (LeaderFlag == 2) {
        delete server2;
        server2 = nullptr;
        node2.reset();
        if (t2.joinable()) {
            t2.detach();
            t2.std::thread::~thread();
        }
    } else if (LeaderFlag == 3) {
        delete server3;
        server3 = nullptr;
        node3.reset();
        if (t3.joinable()) {
            t3.detach();
            t3.std::thread::~thread();
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    if (LeaderFlag == 1) {
        if (node2->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node2->GetCurrState() == raftcpp::RaftState::LEADER) {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }

        if (node3->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node3->GetCurrState() == raftcpp::RaftState::LEADER) {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    } else if (LeaderFlag == 2) {
        if (node1->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node1->GetCurrState() == raftcpp::RaftState::LEADER) {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }

        if (node3->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node3->GetCurrState() == raftcpp::RaftState::LEADER) {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    } else if (LeaderFlag == 3) {
        if (node1->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node1->GetCurrState() == raftcpp::RaftState::LEADER) {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }

        if (node2->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node2->GetCurrState() == raftcpp::RaftState::LEADER) {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    }

    REQUIRE_EQ(nodeStateLeader.size(), 1);
    REQUIRE_EQ(nodeStateFollower.size(), 1);

    if (server1) {
        delete server1;
        server1 = nullptr;
        node1.reset();
    }
    if (server2) {
        delete server2;
        server2 = nullptr;
        node2.reset();
    }
    if (server3) {
        delete server3;
        server3 = nullptr;
        node3.reset();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (t1.joinable()) {
        t1.detach();
        t1.std::thread::~thread();
    }
    if (t2.joinable()) {
        t2.detach();
        t2.std::thread::~thread();
    }
    if (t3.joinable()) {
        t3.detach();
        t3.std::thread::~thread();
    }
}
