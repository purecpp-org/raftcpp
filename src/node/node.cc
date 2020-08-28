#include "node.h"

namespace raftcpp::node {

RaftNode::RaftNode(const std::string &address, const int &port, const RaftState &state)
    : address_{address}, port_{port}, state_{state} {
    switch (state) {
    case RaftState::LEADER:
        server = std::make_unique<rest_rpc::rpc_service::rpc_server>(
            port_, std::thread::hardware_concurrency());
        server->register_handler("heartbeat", heartbeat);
        break;
    case RaftState::FOLLOWER:
        client = std::make_unique<rest_rpc::rpc_client>(address_, port_);
        {
            bool r = client->connect();
            if (!r) {
                std::cout << "connect timeout" << std::endl;
            }
        }
        break;
    default:
        throw std::runtime_error("error state parameter");
    }
}

void RaftNode::start() {
    if (state_ == RaftState::LEADER) {
        server->run();
    } else {
        try {
            bool result = client->call<bool>("heartbeat");
            std::cout << "heartbeat result: " << std::boolalpha << result << std::endl;
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
        }
        while (1)
            ;
    }
};

}  // namespace raftcpp::node
