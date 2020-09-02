#include "node.h"

#include "common/constants.h"
#include "common/logging.h"

namespace raftcpp::node {

RaftNode::RaftNode(rest_rpc::rpc_service::rpc_server &rpc_server,
                   const common::Config &config)
    : timer_manager_(
          /*election_timer_timeout_handler=*/[this]() { this->RequestPreVote(); },
          /*heartbeat_timer_timeout_handler=*/[this]() { this->RequestHeartbeat(); },
          /*vote_timer_timeout_handler=*/[this]() { this->RequestVote(); }),
      rpc_server_(rpc_server),
      config_(config) {
    {
        // Register RPC handles.
        rpc_server_.register_handler<rest_rpc::Async>("request_pre_vote",
                                                      &RaftNode::OnRequestPreVote, this);
        rpc_server_.register_handler<rest_rpc::Async>("request_vote", &RaftNode::OnRequestVote, this);
        rpc_server_.register_handler<rest_rpc::Async>("request_heartbeat", &RaftNode::OnRequestHeartbeat,
                                                      this);
    }
    ConnectToOtherNodes();
    // Starting timer manager should be invoked after all rpc initialization.
    timer_manager_.Start();
}

RaftNode::~RaftNode() {}

void RaftNode::RequestPreVote() {
    std::lock_guard<std::mutex> guard {mutex_};
    // Note that it's to clear the set.
    responded_pre_vote_nodes_.clear();
    // Pre vote for myself.
    responded_pre_vote_nodes_.insert(this->config_.GetThisEndpoint().ToString());
    for (const auto &rpc_client : rpc_clients_) {
        auto request_pre_vote_callback = [this](
                const boost::system::error_code &ec, string_view data) {
            this->OnPreVote(ec, data);
        };
        rpc_client->async_call<0>(
                "request_pre_vote",
                std::move(request_pre_vote_callback),
                this->config_.GetThisEndpoint().ToString());
    }
}

void RaftNode::OnRequestPreVote(rpc::RpcConn conn, const std::string &endpoint_str) {
    RAFTCPP_LOG(DEBUG) << "Received a RequestVote from node " << endpoint_str;

    std::lock_guard<std::mutex> guard {mutex_};
    if (curr_state_ == RaftState::FOLLOWER) {
        timer_manager_.GetElectionTimerRef().Reset(RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
        const auto req_id = conn.lock()->request_id();
        auto conn_sp = conn.lock();
        if (conn_sp) {
            conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
        }
    } else if (curr_state_ == RaftState::CANDIDATE) {
        // TODO(qwang): step down
    } else if (curr_state_ == RaftState::LEADER) {
        // TODO(qwang):
    }

}

void RaftNode::OnPreVote(const boost::system::error_code &ec, string_view data) {
    RAFTCPP_LOG(DEBUG) << "Received response of request_vote from node "
                       << data << ", error code=" << ec.message();

    std::lock_guard<std::mutex> guard {mutex_};
    responded_pre_vote_nodes_.insert(data.data());
    if (this->config_.GreaterThanHalfNodesNum(responded_pre_vote_nodes_.size())
        && this->curr_state_ == RaftState::FOLLOWER) {
        // There are greater than a half of the nodes responded the pre vote request,
        // so stop the election timer and send the vote rpc request to all nodes.
        //
        // TODO(qwang): We should post these rpc methods to a separated io service.
        curr_state_ = RaftState::CANDIDATE;
        RAFTCPP_LOG(INFO) << "This node has became a candidate now.";
        timer_manager_.GetElectionTimerRef().Stop();
        timer_manager_.GetVoteTimerRef().Start(RaftcppConstants::DEFAULT_VOTE_TIMER_TIMEOUT_MS);

        this->RequestVote();
    } else {

    }
}

void RaftNode::RequestVote() {
    RAFTCPP_LOG(DEBUG) << "Before request vote.";
    std::lock_guard<std::mutex> guard {mutex_};
    RAFTCPP_LOG(DEBUG) << "Request vote.";

    // Note that it's to clear the set.
    // TODO(qwang): Considering that whether it shouldn't clear this in every request,
    // because some nodes may responds the last request.
    responded_vote_nodes_.clear();
    // Vote for myself.
    responded_vote_nodes_.insert(this->config_.GetThisEndpoint().ToString());
    for (const auto &rpc_client : rpc_clients_) {
        auto request_vote_callback = [this] (
                const boost::system::error_code &ec, string_view data) {
            this->OnVote(ec, data);
        };
        rpc_client->async_call<0>("request_vote",
                                  std::move(request_vote_callback),
                                  this->config_.GetThisEndpoint().ToString());
    }
}

void RaftNode::OnRequestVote(rpc::RpcConn conn, const std::string &endpoint_str) {
    std::lock_guard<std::mutex> guard {mutex_};
    if (curr_state_ == RaftState::FOLLOWER) {
        timer_manager_.GetElectionTimerRef().Stop();
        const auto req_id = conn.lock()->request_id();
        auto conn_sp = conn.lock();
        if (conn_sp) {
            conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
        }
    } else if (curr_state_ == RaftState::CANDIDATE) {
        // TODO(qwang):
    } else if (curr_state_ == RaftState::LEADER) {
        // TODO(qwang):
    }
}

void RaftNode::OnVote(const boost::system::error_code &ec, string_view data) {
    std::lock_guard<std::mutex> guard {mutex_};
    responded_vote_nodes_.insert(data.data());
    if (this->config_.GreaterThanHalfNodesNum(responded_pre_vote_nodes_.size())
        && this->curr_state_ == RaftState::CANDIDATE) {
        // There are greater than a half of the nodes responded the pre vote request,
        // so stop the election timer and send the vote rpc request to all nodes.
        //
        // TODO(qwang): We should post these rpc methods to a separated io service.
        curr_state_ = RaftState::LEADER;
        timer_manager_.GetElectionTimerRef().Stop();
        timer_manager_.GetHeartbeatTimerRef().Start(RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS);
        this->RequestHeartbeat();
    } else {

    }
}

void RaftNode::RequestHeartbeat() {
    for (const auto &rpc_client : rpc_clients_) {
        RAFTCPP_LOG(DEBUG) << "Send a heartbeat to node.";
        rpc_client->async_call<0>("heartbeat", /*callback=*/nullptr);
    }
}

void RaftNode::OnRequestHeartbeat(rpc::RpcConn conn) {
    RAFTCPP_LOG(DEBUG) << "Received a heartbeat from leader.";
    timer_manager_.GetElectionTimerRef().Reset(
            RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
}

void RaftNode::ConnectToOtherNodes() {
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

}  // namespace raftcpp::node
