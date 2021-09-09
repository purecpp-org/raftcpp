#include <thread>
#include <vector>

#include "../examples/counter/counter_state_machine.h"
#include "common/config.h"
#include "common/logging.h"
#include "gtest/gtest.h"
#include "mock_state_machine.h"
#include "node/node.h"
#include "rest_rpc/rpc_server.h"

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

TEST(LogManagerTest, TestLeaderPushLog) {
    int leaderFlag = -1;  // mark the leader node
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
        threads[i] = std::thread([i, &server] {
            server->run();
            std::cout << "stop\n";
        });
    }

    // wait for their initialization
    std::this_thread::sleep_for(std::chrono::seconds(4));

    for (int i = 0; i < nodeNum; ++i) {
        if (nodes[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
            leaderFlag = i;
        }

        ASSERT_EQ(nodes[i]->CurrLogIndex(), 0);
    }
    ASSERT_NE(leaderFlag, -1);

    int delta = 1;
    std::shared_ptr<examples::counter::IncrRequest> request =
        std::make_shared<examples::counter::IncrRequest>(delta);
    nodes[leaderFlag]->PushRequest(request);

    delta++;
    request = std::make_shared<examples::counter::IncrRequest>(delta);
    nodes[leaderFlag]->PushRequest(request);

    std::this_thread::sleep_for(std::chrono::seconds(4));
    for (int i = 0; i < nodeNum; ++i) {
        ASSERT_EQ(nodes[i]->CurrLogIndex(), delta);
    }

    // shutdown leader
    servers[leaderFlag] = nullptr;
    nodes[leaderFlag].reset();
    std::this_thread::sleep_for(std::chrono::seconds(4));

    delta++;
    for (int i = 0; i < nodeNum; ++i) {
        if (servers[i] == nullptr) {
            continue;
        }

        if (nodes[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
            leaderFlag = i;
        }

        ASSERT_EQ(nodes[i]->CurrLogIndex(), delta);
    }
    ASSERT_NE(leaderFlag, -1);

    // end
    std::this_thread::sleep_for(std::chrono::seconds(1));
    servers.clear();
    for (int i = 0; i < nodeNum; i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}