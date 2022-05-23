#include <grpcpp/grpcpp.h>

#include <atomic>
#include <memory>

#include "gtest/gtest.h"
#include "src/common/endpoint.h"
#include "src/common/type_def.h"
#include "src/node/node.h"
#include "src/statemachine/state_machine_impl.h"
#include "util.h"

TEST(raftPaperTests, selectTest) {
    int leaderFlag = 0;  // mark the leader node
    int nodeNum = 3;
    int basePort = 10001;
    std::string address("127.0.0.1");

    std::vector<raftcpp::RaftState> nodeStateLeader;
    std::vector<raftcpp::RaftState> nodeStateFollower;

    std::vector<std::shared_ptr<raftcpp::node::RaftNode>> nodes(nodeNum);
    // std::vector<std::shared_ptr<grpc::Server>> servers(nodeNum);
    std::vector<std::unique_ptr<grpc::Server>> servers(nodeNum);
    std::vector<std::thread> threads(nodeNum);

    for (int i = 0; i < nodeNum; i++) {
        std::string config_str = init_config(address, basePort, nodeNum, i);
        const auto config = raftcpp::common::Config::From(config_str);
        std::cout << "Raft node run on: " << config.GetThisEndpoint() << std::endl;
        auto node = std::make_shared<raftcpp::node::RaftNode>(
            std::make_shared<raftcpp::RaftStateMachine>(), config,
            raftcpp::RaftcppLogLevel::RLL_DEBUG);
        auto server = BuildServer(config.GetThisEndpoint(), node);
        node->Init();
        nodes[i] = node;

        threads[i] = std::thread([server = std::move(server)] {
            // Wait for the server to shutdown. Note that some other thread must be
            // responsible for shutting down the server for this call to ever return.
            server->Wait();
            std::cout << "stop\n";
        });
    }

    for (auto &thread : threads) {
        thread.detach();
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));

    nodeStateFollower.clear();
    nodeStateLeader.clear();
    for (int i = 0; i < nodeNum; ++i) {
        if (nodes[i]->GetCurrState() == raftcpp::RaftState::FOLLOWER) {
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        } else if (nodes[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
            leaderFlag = i;
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    }
    ASSERT_EQ(nodeStateLeader.size(), 1);
    ASSERT_EQ(nodeStateFollower.size(), 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}