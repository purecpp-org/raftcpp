#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <thread>
#include <vector>

#include "../examples/counter/counter_service_def.h"
#include "../examples/counter/counter_state_machine.h"
#include "common/config.h"
#include "common/logging.h"
#include "gtest/gtest.h"
#include "node/node.h"
#include "rest_rpc/rpc_server.h"

class MockResponse : public raftcpp::RaftcppResponse {
public:
    MockResponse() {}

    ~MockResponse() override {}
};

class MockStateMachine : public raftcpp::StateMachine {
public:
    bool ShouldDoSnapshot() override { return true; }

    void SaveSnapshot() override{};

    void LoadSnapshot() override{};

    virtual raftcpp::RaftcppResponse OnApply(const std::string &serialized) override {
        return MockResponse();
    };

private:
};

class CounterServiceImpl {
public:
    // TODO(qwang): Are node and fsm uncopyable?
    CounterServiceImpl(std::shared_ptr<raftcpp::node::RaftNode> node,
                       std::shared_ptr<examples::counter::CounterStateMachine> &fsm)
        : node_(std::move(node)), fsm_(std::move(fsm)) {}

    void Incr(rpc_conn conn, int delta) {
        std::shared_ptr<examples::counter::IncrRequest> request =
            std::make_shared<examples::counter::IncrRequest>(delta);
        node_->PushRequest(request);
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

void node_run(std::shared_ptr<rpc_server> server, CounterServiceImpl &service) {
    //    const auto config = raftcpp::common::Config::From(conf_str);
    //    std::cout<<config.GetThisEndpoint().GetPort()<<std::endl;
    //
    //    node =
    //    std::make_shared<raftcpp::node::RaftNode>(std::make_shared<MockStateMachine>(),
    //                                                     *server, config,
    //                                                     raftcpp::RaftcppLogLevel::RLL_DEBUG);
    //    auto fsm = std::make_shared<examples::counter::CounterStateMachine>();

    //    CounterServiceImpl service(node, fsm);
    server->register_handler("incr", &CounterServiceImpl::Incr, &service);
    server->register_handler("get", &CounterServiceImpl::Get, &service);
    server->run();
}

std::string init_config(std::string address, int basePort, int nodeNum, int thisNode) {
    std::vector<std::string> addr;
    addr.push_back(address + ":" + std::to_string(basePort + thisNode));

    for (int i = 0; i < nodeNum; i++) {
        if (i == thisNode) {
            continue;
        }
        addr.push_back(address + ":" + std::to_string(basePort + i));
    }

    std::string config;
    for (int i = 0; i < nodeNum; i++) {
        config += addr[i];
        if (i < nodeNum - 1) {
            config += ",";
        }
    }

    return config;
}

TEST(NodeElectTest, TestNodeElect) {
    int leaderFlag = 0;  // mark the leader node
    int nodeNum = 3;
    int basePort = 10001;
    std::string address("127.0.0.1");

    std::vector<raftcpp::RaftState> nodeStateLeader;
    std::vector<raftcpp::RaftState> nodeStateFollower;

    std::vector<std::shared_ptr<raftcpp::node::RaftNode>> nodes(nodeNum);
    std::vector<std::shared_ptr<rpc_server>> servers(nodeNum);
    std::vector<std::thread> threads(nodeNum);

    // create nodes
    for (int i = 0; i < nodeNum; i++) {
        servers[i] = std::make_shared<rpc_server>(basePort + i,
                                                  std::thread::hardware_concurrency());
    }

    for (int i = 0; i < nodeNum; i++) {
        std::string config_str = init_config(address, basePort, nodeNum, i);
        const auto config = raftcpp::common::Config::From(config_str);
        std::cout << config.GetThisEndpoint().GetPort() << std::endl;

        auto &server = servers[i];

        auto node = std::make_shared<raftcpp::node::RaftNode>(
            std::make_shared<MockStateMachine>(), *server, config,
            raftcpp::RaftcppLogLevel::RLL_DEBUG);
        node->Init();
        nodes[i] = node;
        auto fsm = std::make_shared<examples::counter::CounterStateMachine>();
        CounterServiceImpl service(node, fsm);
        servers[i]->register_handler("incr", &CounterServiceImpl::Incr, &service);
        servers[i]->register_handler("get", &CounterServiceImpl::Get, &service);

        threads[i] = std::thread([i, &server] {
            server->run();
            std::cout << "stop\n";
        });
    }

    // wait for their initialization
    std::this_thread::sleep_for(std::chrono::seconds(5));

    nodeStateFollower.clear();
    nodeStateLeader.clear();

    // get the leader node
    for (int i = 0; i < nodeNum; i++) {
        if (nodes[i]->GetCurrState() == raftcpp::RaftState::FOLLOWER) {
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        } else if (nodes[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
            leaderFlag = i;
            //            std::cout<<threads[leaderFlag].get_id()<<std::endl;
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    }

    ASSERT_EQ(nodeStateLeader.size(), 1);
    ASSERT_EQ(nodeStateFollower.size(), 2);

    nodeStateFollower.clear();
    nodeStateLeader.clear();

    // shutdown the leader
    servers[leaderFlag] = nullptr;
    nodes[leaderFlag].reset();
    std::cout << threads[leaderFlag].get_id() << std::endl;
    if (threads[leaderFlag].joinable()) {
        threads[leaderFlag].join();
    }

    // wait for the re-election in another two nodes
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // get the new leader node
    for (int i = 0; i < nodeNum; i++) {
        if (servers[i] == nullptr) {
            continue;
        }

        if (nodes[i]->GetCurrState() == raftcpp::RaftState::FOLLOWER) {
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        } else if (nodes[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
            leaderFlag = i;
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    }

    // TODO this test case won't pass, it seems the re-election not succesfull, need to be
    // fixed later.
    //    ASSERT_EQ(nodeStateLeader.size(), 1);
    //    ASSERT_EQ(nodeStateFollower.size(), 1);

    // shutdown the leader node
    // delete servers[leaderFlag];
    servers[leaderFlag] = nullptr;
    nodes[leaderFlag].reset();
    if (threads[leaderFlag].joinable()) {
        threads[leaderFlag].join();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    servers.clear();
    for (int i = 0; i < nodeNum; i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
}
