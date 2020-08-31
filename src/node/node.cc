#include "node.h"

// TODO(qwang): Refine this as a component logging.
#include "nanolog.hpp"

namespace raftcpp::node {

RaftNode::RaftNode(rest_rpc::rpc_service::rpc_server &rpc_server, const std::string &address, const int &port)
    : rpc_server_(rpc_server),
    endpoint_(address, port) {
    // Initial logging
    nanolog::initialize(nanolog::GuaranteedLogger(), "/tmp/raftcpp", "node.log", 10);
    nanolog::set_log_level(nanolog::LogLevel::DEBUG);

    // Register RPC handles.
    rpc_server_.register_handler("request_vote", &RaftNode::RequestVote, this);

    // Initial all connections and regiester fialures.
}

RaftNode::~RaftNode() {}

}  // namespace raftcpp::node
