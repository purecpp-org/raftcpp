#include "node.h"

// TODO(qwang): Refine this as a component logging.
#include "nanolog.hpp"

namespace raftcpp::node {

RaftNode::RaftNode(rest_rpc::rpc_service::rpc_server &rpc_server,
                   const common::Config &config)
    : rpc_server_(rpc_server), config_(config) {
    // Initial logging
    nanolog::initialize(nanolog::GuaranteedLogger(), "/tmp/raftcpp", "node.log", 10);
    nanolog::set_log_level(nanolog::LogLevel::DEBUG);

    // Register RPC handles.
    rpc_server_.register_handler("request_vote", &RaftNode::RequestVote, this);

    {
        // Initial the rpc clients connecting to other nodes.
        for (const auto &endpoint : config_.GetOtherEndpoints()) {
            auto rpc_client = std::make_shared<rest_rpc::rpc_client>(endpoint.GetHost(),
                                                                     endpoint.GetPort());
            bool connected = rpc_client->connect();
            if (!connected) {
                // TODO(qwang): Use log instead.
                std::cout << "Failed to connect to the node " << endpoint.ToString()
                          << std::endl;
            }
            rpc_clients_.push_back(rpc_client);
        }
    }
    // Initial all connections and regiester fialures.
}

RaftNode::~RaftNode() {}

}  // namespace raftcpp::node
