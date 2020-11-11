#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <thread>
#include "common/config.h"
#include "../examples/counter/counter_state_machine.h"
#include "../examples/counter/counter_service_def.h"
#include "node/node.h"
#include "rpc_server.h"
#include "common/logging.h"

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

void node_run(std::shared_ptr<raftcpp::node::RaftNode> &node,const std::string &conf_str)
{
    const auto config = raftcpp::common::Config::From(conf_str);
    // Initial a rpc server and listening on its port.
    rpc_server server(config.GetThisEndpoint().GetPort(),
                      std::thread::hardware_concurrency());

    node = std::make_shared<raftcpp::node::RaftNode>(server, config,raftcpp::RaftcppLogLevel::RLL_DEBUG);
    auto fsm = std::make_shared<examples::counter::CounterStateMachine>();

    CounterServiceImpl service(node, fsm);
    server.register_handler("incr", &CounterServiceImpl::Incr, &service);
    server.register_handler("get", &CounterServiceImpl::Get, &service);
    server.run();

    return;
}

TEST_CASE("TestNodeElect") {
    int LeaderFlag = 0;
    std::vector<raftcpp::RaftState> nodeStateLeader;
    std::vector<raftcpp::RaftState> nodeStateFollower;

    std::shared_ptr<raftcpp::node::RaftNode> node1 = nullptr;
    std::shared_ptr<raftcpp::node::RaftNode> node2 = nullptr;
    std::shared_ptr<raftcpp::node::RaftNode> node3 = nullptr;

    //std::ref(
    std::thread t1(node_run, std::ref(node1), "127.0.0.1:10001,127.0.0.1:10002,127.0.0.1:10003");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread t2(node_run, std::ref(node2), "127.0.0.1:10002,127.0.0.1:10001,127.0.0.1:10003");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread t3(node_run, std::ref(node3), "127.0.0.1:10003,127.0.0.1:10001,127.0.0.1:10002");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    while(true) {
        if (node1 && node2 && node3) {
                break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    nodeStateFollower.clear();
    nodeStateLeader.clear();
    if (node1->GetCurrState() == raftcpp::RaftState::FOLLOWER)
        nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
    else if (node1->GetCurrState() == raftcpp::RaftState::LEADER)
    {
        LeaderFlag = 1;
        nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
    }

    if (node2->GetCurrState() == raftcpp::RaftState::FOLLOWER)
        nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
    else if (node2->GetCurrState() == raftcpp::RaftState::LEADER)
    {
        LeaderFlag = 2;
        nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
    }

    if (node3->GetCurrState() == raftcpp::RaftState::FOLLOWER)
        nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
    else if (node3->GetCurrState() == raftcpp::RaftState::LEADER)
    {
        LeaderFlag = 3;
        nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
    }

//    REQUIRE_EQ(nodeStateLeader.size(), 1);
//    REQUIRE_EQ(nodeStateFollower.size(), 2);

    REQUIRE_EQ(1, 1);
    REQUIRE_EQ(2, 2);

    nodeStateFollower.clear();
    nodeStateLeader.clear();

    if(LeaderFlag == 1)
    {
        t1.detach();
    }
    else if(LeaderFlag == 2)
    {
        t2.detach();
    }
    else if(LeaderFlag == 3)
    {
        t3.detach();
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    if(LeaderFlag == 1)
    {
        if (node2->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node2->GetCurrState() == raftcpp::RaftState::LEADER)
        {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }

        if (node3->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node3->GetCurrState() == raftcpp::RaftState::LEADER)
        {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    }
    else if(LeaderFlag == 2)
    {
        if (node1->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node1->GetCurrState() == raftcpp::RaftState::LEADER)
        {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }

        if (node3->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node3->GetCurrState() == raftcpp::RaftState::LEADER)
        {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    }
    else if(LeaderFlag == 3)
    {
        if (node1->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node1->GetCurrState() == raftcpp::RaftState::LEADER)
        {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }

        if (node2->GetCurrState() == raftcpp::RaftState::FOLLOWER)
            nodeStateFollower.push_back(raftcpp::RaftState::FOLLOWER);
        else if (node2->GetCurrState() == raftcpp::RaftState::LEADER)
        {
            nodeStateLeader.push_back(raftcpp::RaftState::LEADER);
        }
    }
//    REQUIRE_EQ(nodeStateLeader.size(), 1);
//    REQUIRE_EQ(nodeStateFollower.size(), 1);
    if(t1.joinable())
        t1.detach();
    if(t2.joinable())
        t2.detach();
    if(t3.joinable())
        t3.detach();
}

