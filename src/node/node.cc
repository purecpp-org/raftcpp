#include "node.h"

#include "common/constants.h"
#include "common/logging.h"

namespace raftcpp::node {

RaftNode::RaftNode(rest_rpc::rpc_service::rpc_server &rpc_server,
                   const common::Config &config)
    : timer_manager_(
          /*election_timer_timeout_handler=*/
          [this]() {
              for (const auto &rpc_client : rpc_clients_) {
                  // TODO(qwang):
                  // 1. Add a lock to protect rpc_clients.
                  auto request_vote_callback = [this, rpc_client](
                                                   const boost::system::error_code &ec,
                                                   string_view data) {
                      RAFTCPP_LOG(DEBUG) << "Received response of request_vote from node "
                                         << data << ", error code=" << ec.message();
                      timer_manager_.GetHeartbeatTimerRef().Start(
                          RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS);
                      timer_manager_.GetElectionTimerRef().Stop();
                  };
                  rpc_client->async_call<0>("request_vote", request_vote_callback,
                                            this->config_.GetThisEndpoint().ToString());
              }
          },
          [this]() {
              for (const auto &rpc_client : rpc_clients_) {
                  RAFTCPP_LOG(DEBUG) << "Send a heartbeat to node.";
                  rpc_client->async_call<0>("heartbeat", /*callback=*/nullptr);
              }
          }),
      rpc_server_(rpc_server),
      config_(config) {
    // Register RPC handles.
    rpc_server_.register_handler<rest_rpc::Async>("request_vote",
                                                  &RaftNode::OnRequestVote, this);
    rpc_server_.register_handler<rest_rpc::Async>("heartbeat", &RaftNode::OnHeartbeat,
                                                  this);

    {
        // Initial the rpc clients connecting to other nodes.
        for (const auto &endpoint : config_.GetOtherEndpoints()) {
            auto rpc_client = std::make_shared<rest_rpc::rpc_client>(endpoint.GetHost(),
                                                                     endpoint.GetPort());
            bool connected = rpc_client->connect();
            if (!connected) {
                RAFTCPP_LOG(DEBUG)
                    << "Failed to connect to the node " << endpoint.ToString();
            }
            rpc_client->enable_auto_heartbeat();
            rpc_client->enable_auto_reconnect();
            RAFTCPP_LOG(DEBUG) << "Succeeded to connect to the node "
                               << endpoint.ToString();
            rpc_clients_.push_back(rpc_client);
        }
    }

    // Starting timer manager should be invoked after all rpc initialization.
    timer_manager_.Start();
}

RaftNode::~RaftNode() {}

void RaftNode::OnRequestVote(rpc::RpcConn conn, const std::string &endpoint_str) {
    RAFTCPP_LOG(DEBUG) << "Received a RequestVote from node " << endpoint_str;
    timer_manager_.GetElectionTimerRef().Stop();
    const auto req_id = conn.lock()->request_id();
    auto conn_sp = conn.lock();
    if (conn_sp) {
        conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
    }
}

void RaftNode::OnHeartbeat(rpc::RpcConn conn) {
    RAFTCPP_LOG(DEBUG) << "Received a heartbeat from leader.";
    timer_manager_.GetElectionTimerRef().Reset(
        RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
}

}  // namespace raftcpp::node
