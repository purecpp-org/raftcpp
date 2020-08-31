#include "node.h"

// TODO(qwang): Refine this as a component logging.
#include "nanolog.hpp"

namespace raftcpp::node {

RaftNode::RaftNode(rest_rpc::rpc_service::rpc_server &rpc_server,
                   const common::Config &config)
    : timer_manager_(/*election_timer_timeout_handler=*/[this]() {
          for (const auto &rpc_client : rpc_clients_) {
              // TODO(qwang):
              // 1. Add a lock to protect rpc_clients.
              // 2. Refine this call as a async call.
              // 3. Use log instead.
              auto result = rpc_client->call<std::string>(
                  "request_vote", this->config_.GetThisEndpoint().ToString());
              std::cout << "Received response: " << result << std::endl;
          }
      }),
      rpc_server_(rpc_server),
      config_(config) {
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

    // Starting timer manager should be invoked after all rpc initialization.
    timer_manager_.Start();
}

RaftNode::~RaftNode() {}

void RaftNode::RequestVote(rpc::RpcConn conn, const std::string &endpoint_str) {
    // TODO(qwang): Use log instead.
    std::cout << "Received a RequestVote from node" << endpoint_str << std::endl;

    const auto req_id = conn.lock()->request_id();
    auto conn_sp = conn.lock();
    if (conn_sp) {
        // TODO(qwang): What does this `if` do?
        conn_sp->pack_and_response(req_id, "OK");
    }
}

}  // namespace raftcpp::node
