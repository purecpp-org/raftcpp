//#include <thread>
//#include <vector>
//
//#include "grpcpp/grpcpp.h"
//#include "gtest/gtest.h"
//#include "node/node.h"
//#include "src/common/config.h"
//#include "src/common/logging.h"
//#include "src/statemachine/state_machine_impl.h"
//#include "util.h"
//
// TEST(LogManagerTest, TestLeaderPushLog) {
//    int leaderFlag = 0;  // mark the leader node
//    int nodeNum = 3;
//    int basePort = 10001;
//    std::string address("127.0.0.1");
//
//    std::vector<raftcpp::RaftState> nodeStateLeader;
//    std::vector<raftcpp::RaftState> nodeStateFollower;
//
//    std::vector<std::shared_ptr<raftcpp::node::RaftNode>> nodes(nodeNum);
//    // std::vector<std::shared_ptr<grpc::Server>> servers(nodeNum);
//    std::vector<std::unique_ptr<grpc::Server>> servers(nodeNum);
//    std::vector<std::thread> threads(nodeNum);
//
//    for (int i = 0; i < nodeNum; i++) {
//        std::string config_str = init_config(address, basePort, nodeNum, i);
//        const auto config = raftcpp::common::Config::From(config_str);
//        std::cout << "Raft node run on: " << config.GetThisEndpoint() << std::endl;
//        auto node = std::make_shared<raftcpp::node::RaftNode>(
//            std::make_shared<raftcpp::RaftStateMachine>(), config,
//            raftcpp::RaftcppLogLevel::RLL_DEBUG);
//        auto server = BuildServer(config.GetThisEndpoint(), node);
//        node->Init();
//        nodes[i] = node;
//
//        threads[i] = std::thread([server = std::move(server)] {
//            // Wait for the server to shutdown. Note that some other thread must be
//            // responsible for shutting down the server for this call to ever return.
//            server->Wait();
//            std::cout << "stop\n";
//        });
//    }
//
//    for (auto &thread : threads) {
//        thread.detach();
//    }
//
//    // wait for their initialization
//    std::this_thread::sleep_for(std::chrono::seconds(4));
//
//    for (int i = 0; i < nodeNum; ++i) {
//        if (nodes[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
//            leaderFlag = i;
//        }
//
//        ASSERT_EQ(nodes[i]->CurrLogIndex(), 0);
//    }
//    ASSERT_NE(leaderFlag, -1);
//
//    int delta = 1;
//    std::shared_ptr<examples::counter::IncrRequest> request =
//        std::make_shared<examples::counter::IncrRequest>(delta);
//    nodes[leaderFlag]->PushRequest(request);
//
//    delta++;
//    request = std::make_shared<examples::counter::IncrRequest>(delta);
//    nodes[leaderFlag]->PushRequest(request);
//
//    std::this_thread::sleep_for(std::chrono::seconds(4));
//    for (int i = 0; i < nodeNum; ++i) {
//        ASSERT_EQ(nodes[i]->CurrLogIndex(), delta);
//    }
//
//    // shutdown leader
//    servers[leaderFlag] = nullptr;
//    nodes[leaderFlag].reset();
//    std::this_thread::sleep_for(std::chrono::seconds(4));
//
//    delta++;
//    for (int i = 0; i < nodeNum; ++i) {
//        if (servers[i] == nullptr) {
//            continue;
//        }
//
//        if (nodes[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
//            leaderFlag = i;
//        }
//
//        ASSERT_EQ(nodes[i]->CurrLogIndex(), delta);
//    }
//    ASSERT_NE(leaderFlag, -1);
//
//    // end
//    std::this_thread::sleep_for(std::chrono::seconds(1));
//    servers.clear();
//    for (int i = 0; i < nodeNum; i++) {
//        if (threads[i].joinable()) {
//            threads[i].join();
//        }
//    }
//}
//
// int main(int argc, char **argv) {
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//}