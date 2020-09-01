#include "node.h"

#include "common/constants.h"

// TODO(qwang): Refine this as a component logging.
#include "nanolog.hpp"

namespace raftcpp::node {

RaftNode::RaftNode(rest_rpc::rpc_service::rpc_server &rpc_server,
                   const common::Config &config)
    : timer_manager_(/*election_timer_timeout_handler=*/[this]() {
          for (const auto &rpc_client : rpc_clients_) {
              // TODO(qwang):
              // 1. Add a lock to protect rpc_clients.
              // 2. Use log instead.
              auto request_vote_callback = [this, rpc_client](const boost::system::error_code &ec,
                                                  string_view data) {
                  std::cout << "Received response of request_vote from node " << data
                            << ", error code=" << ec.message() << std::endl;
                  rpc_client->async_call<0>("heartbeat", /*callback=*/nullptr);
                  // TOD(qwang): This should be refined to Stop().
                  timer_manager_.GetElectionTimerRef().Reset(RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
              };
              rpc_client->async_call<0>("request_vote", request_vote_callback,
                                        this->config_.GetThisEndpoint().ToString());
          }
      }),
      rpc_server_(rpc_server),
      config_(config) {
    // Initial logging
    nanolog::initialize(nanolog::GuaranteedLogger(), "/tmp/raftcpp", "node.log", 10);
    nanolog::set_log_level(nanolog::LogLevel::DEBUG);

    // Register RPC handles.
    rpc_server_.register_handler<rest_rpc::Async>("request_vote", &RaftNode::OnRequestVote,this);
    rpc_server_.register_handler<rest_rpc::Async>("heartbeat", &RaftNode::OnHeartbeat, this);

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
            rpc_client->enable_auto_heartbeat();
            rpc_client->enable_auto_reconnect();
            std::cout << "Succeeded to connect to the node " << endpoint.ToString()
                      << std::endl;
            rpc_clients_.push_back(rpc_client);
        }
    }

    // Starting timer manager should be invoked after all rpc initialization.
    timer_manager_.Start();
}

RaftNode::~RaftNode() {}

// TODO(qwang): Move this handles to the `NodeService` and use the lambda to register onto it.
void RaftNode::OnRequestVote(rpc::RpcConn conn, const std::string &endpoint_str) {
    // TODO(qwang): Use log instead.
    std::cout << "Received a RequestVote from node " << endpoint_str << std::endl;

    timer_manager_.GetElectionTimerRef().Stop();
    const auto req_id = conn.lock()->request_id();
    auto conn_sp = conn.lock();
    if (conn_sp) {
        conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
    }
}

void RaftNode::OnHeartbeat(rpc::RpcConn conn) {
    std::cout << "Received a heartbeat from leader." << std::endl;
    timer_manager_.GetElectionTimerRef().Reset(RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
}

}  // namespace raftcpp::node
