#include "node.h"

#include "common/constants.h"
#include "common/logging.h"

namespace raftcpp::node {

RaftNode::RaftNode(std::shared_ptr<StateMachine> state_machine,
                   rest_rpc::rpc_service::rpc_server &rpc_server,
                   const common::Config &config, RaftcppLogLevel severity)
    : timer_manager_(
          /*election_timer_timeout_handler=*/[this]() { this->RequestPreVote(); },
          /*heartbeat_timer_timeout_handler=*/[this]() { this->RequestHeartbeat(); },
          /*vote_timer_timeout_handler=*/[this]() { this->RequestVote(); }),
      rpc_server_(rpc_server),
      config_(config),
      this_node_id_(config.GetThisEndpoint()),
      leader_log_manager_(std::make_unique<LeaderLogManager>(
          this_node_id_, [this]() -> AllRpcClientType { return all_rpc_clients_; })),
      non_leader_log_manager_(std::make_unique<NonLeaderLogManager>(
          [this]() {
              std::lock_guard<std::recursive_mutex> guard{mutex_};
              return curr_state_ == RaftState::LEADER;
          },
          [this]() -> std::shared_ptr<rest_rpc::rpc_client> {
              std::lock_guard<std::recursive_mutex> guard{mutex_};
              if (curr_state_ != RaftState::LEADER) {
                  return nullptr;
              }
              RAFTCPP_CHECK(leader_node_id_ != nullptr);
              RAFTCPP_CHECK(all_rpc_clients_.count(*leader_node_id_) == 1);
              return all_rpc_clients_[*leader_node_id_];
          })),

      state_machine_(std::move(state_machine)) {
    std::string log_name = "node-" + config_.GetThisEndpoint().ToString() + ".log";
    replace(log_name.begin(), log_name.end(), '.', '-');
    replace(log_name.begin(), log_name.end(), ':', '-');
    raftcpp::RaftcppLog::StartRaftcppLog(log_name, severity, 10, 3);
    InitRpcHandlers();
    ConnectToOtherNodes();
    // Starting timer manager should be invoked after all rpc initialization.
    timer_manager_.Start();
}

RaftNode::~RaftNode() {}

void RaftNode::Apply(const std::shared_ptr<raftcpp::RaftcppRequest> &request) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_CHECK(request != nullptr);
    RAFTCPP_CHECK(curr_state_ == RaftState::LEADER);
    // This is leader code path.
    leader_log_manager_->Push(curr_term_id_, request);
    //    AsyncAppendLogsToFollowers(entry);
    // TODO(qwang)
}

void RaftNode::RequestPreVote() {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_LOG(RLL_DEBUG) << "Node " << this->config_.GetThisEndpoint().ToString()
                           << " request prevote";
    // only follower can request pre_vote
    if (curr_state_ != RaftState::FOLLOWER) return;

    // Note that it's to clear the set.
    responded_pre_vote_nodes_.clear();
    curr_term_id_.setTerm(curr_term_id_.getTerm() + 1);
    // Pre vote for myself.
    responded_pre_vote_nodes_.insert(this->config_.GetThisEndpoint().ToString());
    for (auto &item : all_rpc_clients_) {
        auto &rpc_client = item.second;
        RAFTCPP_LOG(RLL_DEBUG) << "RequestPreVote Node "
                               << this->config_.GetThisEndpoint().ToString()
                               << "request_vote_callback client" << rpc_client.get();
        auto request_pre_vote_callback = [this](const boost::system::error_code &ec,
                                                string_view data) {
            this->OnPreVote(ec, data);
        };
        rpc_client->async_call<0>(RaftcppConstants::REQUEST_PRE_VOTE_RPC_NAME,
                                  std::move(request_pre_vote_callback),
                                  this->config_.GetThisEndpoint().ToString(),
                                  curr_term_id_.getTerm());
    }
}

void RaftNode::HandleRequestPreVote(rpc::RpcConn conn, const std::string &endpoint_str,
                                    int32_t term_id) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (this->config_.GetThisEndpoint().ToString() == endpoint_str) return;
    RAFTCPP_LOG(RLL_DEBUG) << "HandleRequestPreVote this node "
                           << this->config_.GetThisEndpoint().ToString()
                           << " Received a RequestPreVote from node " << endpoint_str
                           << " term_id=" << term_id;
    const auto req_id = conn.lock()->request_id();
    auto conn_sp = conn.lock();
    if (curr_state_ == RaftState::FOLLOWER) {
        if (term_id > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(term_id);
            timer_manager_.GetElectionTimerRef().Reset(
                RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
                randomer_.TakeOne(1000, 2000));
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        }
    } else if (curr_state_ == RaftState::CANDIDATE || curr_state_ == RaftState::LEADER) {
        // TODO(qwang): step down
        if (term_id > curr_term_id_.getTerm()) {
            RAFTCPP_LOG(RLL_DEBUG)
                << "HandleRequestPreVote Received a RequestPreVote,now  step down";
            StepBack(term_id);
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        }
    }
}

void RaftNode::OnPreVote(const boost::system::error_code &ec, string_view data) {
    RAFTCPP_LOG(RLL_DEBUG) << "Received response of request_vote from node " << data
                           << ", error code=" << ec.message();
    if (ec.message() == "Transport endpoint is not connected") return;
    // Exclude itself under multi-thread
    RAFTCPP_LOG(RLL_DEBUG) << "OnPreVote Response node： " << data.data()
                           << " this node:" << this->config_.GetThisEndpoint().ToString();
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    responded_pre_vote_nodes_.insert(data.data());
    if (this->config_.GreaterThanHalfNodesNum(responded_pre_vote_nodes_.size()) &&
        this->curr_state_ == RaftState::FOLLOWER) {
        // There are greater than a half of the nodes responded the pre vote request,
        // so stop the election timer and send the vote rpc request to all nodes.
        //
        // TODO(qwang): We should post these rpc methods to a separated io service.
        curr_state_ = RaftState::CANDIDATE;
        RAFTCPP_LOG(RLL_INFO) << "This node "
                              << this->config_.GetThisEndpoint().ToString()
                              << " has became a candidate now.";
        curr_term_id_.setTerm(curr_term_id_.getTerm() + 1);
        timer_manager_.GetElectionTimerRef().Stop();
        timer_manager_.GetVoteTimerRef().Start(
            RaftcppConstants::DEFAULT_VOTE_TIMER_TIMEOUT_MS);
        this->RequestVote();
    } else {
    }
}

void RaftNode::RequestVote() {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_LOG(RLL_DEBUG) << "Node " << this->config_.GetThisEndpoint().ToString()
                           << " request vote";
    // only candidate can request vote
    if (curr_state_ != RaftState::CANDIDATE) return;

    // Note that it's to clear the set.
    // TODO(qwang): Considering that whether it shouldn't clear this in every request,
    // because some nodes may responds the last request.
    responded_vote_nodes_.clear();
    // Vote for myself.
    responded_vote_nodes_.insert(this->config_.GetThisEndpoint().ToString());
    for (auto &item : all_rpc_clients_) {
        auto request_vote_callback = [this](const boost::system::error_code &ec,
                                            string_view data) { this->OnVote(ec, data); };
        item.second->async_call<0>(
            RaftcppConstants::REQUEST_VOTE_RPC_NAME, std::move(request_vote_callback),
            this->config_.GetThisEndpoint().ToString(), curr_term_id_.getTerm());
    }
}

void RaftNode::HandleRequestVote(rpc::RpcConn conn, const std::string &endpoint_str,
                                 int32_t term_id) {
    RAFTCPP_LOG(RLL_DEBUG) << "Node " << this->config_.GetThisEndpoint().ToString()
                           << " response vote";
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    const auto req_id = conn.lock()->request_id();
    auto conn_sp = conn.lock();
    if (curr_state_ == RaftState::FOLLOWER) {
        if (term_id > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(term_id);
            timer_manager_.GetElectionTimerRef().Reset(
                RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        }
    } else if (curr_state_ == RaftState::CANDIDATE || curr_state_ == RaftState::LEADER) {
        // TODO(qwang):
        if (term_id > curr_term_id_.getTerm()) {
            StepBack(term_id);
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        }
    }
}

void RaftNode::OnVote(const boost::system::error_code &ec, string_view data) {
    if (ec.message() == "Transport endpoint is not connected") return;
    // Exclude itself under multi-thread
    RAFTCPP_LOG(RLL_DEBUG) << "OnVote Response node： " << data.data()
                           << " this node:" << this->config_.GetThisEndpoint().ToString();
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    responded_vote_nodes_.insert(data.data());
    if (this->config_.GreaterThanHalfNodesNum(responded_vote_nodes_.size()) &&
        this->curr_state_ == RaftState::CANDIDATE) {
        // There are greater than a half of the nodes responded the pre vote request,
        // so stop the election timer and send the vote rpc request to all nodes.
        //
        // TODO(qwang): We should post these rpc methods to a separated io service.
        curr_state_ = RaftState::LEADER;
        RAFTCPP_LOG(RLL_INFO) << "This node "
                              << this->config_.GetThisEndpoint().ToString()
                              << " has became a leader now";
        curr_term_id_.setTerm(curr_term_id_.getTerm() + 1);
        timer_manager_.GetVoteTimerRef().Stop();
        timer_manager_.GetElectionTimerRef().Stop();
        timer_manager_.GetHeartbeatTimerRef().Reset(
            RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS);
        this->RequestHeartbeat();
    } else {
    }
}

void RaftNode::RequestHeartbeat() {
    for (auto &item : all_rpc_clients_) {
        RAFTCPP_LOG(RLL_DEBUG) << "Send a heartbeat to node.";
        item.second->async_call<0>(
            RaftcppConstants::REQUEST_HEARTBEAT,
            /*callback=*/[](const boost::system::error_code &ec, string_view data) {},
            curr_term_id_.getTerm(), this_node_id_.ToBinary());
    }
}

void RaftNode::HandleRequestHeartbeat(rpc::RpcConn conn, int32_t term_id,
                                      std::string node_id_binary) {
    auto source_node_id = NodeID::FromBinary(node_id_binary);
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (curr_state_ == RaftState::FOLLOWER || curr_state_ == RaftState::CANDIDATE) {
        leader_node_id_ = std::make_unique<NodeID>(source_node_id);
        RAFTCPP_LOG(RLL_DEBUG) << "HandleRequestHeartbeat node "
                               << this->config_.GetThisEndpoint().ToString()
                               << "received a heartbeat from leader(node_id="
                               << source_node_id.ToHex() << ")."
                               << " curr_term_id_:" << curr_term_id_.getTerm()
                               << " receive term_id:" << term_id << " update term_id";
        timer_manager_.GetElectionTimerRef().Start(
            RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
            randomer_.TakeOne(1000, 2000));
        curr_term_id_.setTerm(term_id);
    } else {
        if (term_id >= curr_term_id_.getTerm()) {
            leader_node_id_ = std::make_unique<NodeID>(source_node_id);
            RAFTCPP_LOG(RLL_DEBUG) << "HandleRequestHeartbeat node "
                                   << this->config_.GetThisEndpoint().ToString()
                                   << "received a heartbeat from leader."
                                   << " curr_term_id_:" << curr_term_id_.getTerm()
                                   << " receive term_id:" << term_id << " StepBack";
            curr_term_id_.setTerm(term_id);
            timer_manager_.GetVoteTimerRef().Stop();
            timer_manager_.GetHeartbeatTimerRef().Stop();
            curr_state_ = RaftState::FOLLOWER;
            timer_manager_.GetElectionTimerRef().Start(
                RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
                randomer_.TakeOne(1000, 2000));
        } else {
            /// Code path of myself is leader.
            RAFTCPP_LOG(RLL_DEBUG)
                << "HandleRequestHeartbeat node "
                << this->config_.GetThisEndpoint().ToString()
                << "received a heartbeat from leader and send response";
            const auto req_id = conn.lock()->request_id();
            auto conn_sp = conn.lock();
            if (conn_sp) {
                conn_sp->response(req_id, std::to_string(curr_term_id_.getTerm()));
            }
        }
    }
}

void RaftNode::OnHeartbeat(const boost::system::error_code &ec, string_view data) {
    if (ec.message() == "Transport endpoint is not connected") return;
    RAFTCPP_LOG(RLL_DEBUG) << "Received a response heartbeat from node.term_id："
                           << std::stoi(data.data())
                           << "more than currentid:" << curr_term_id_.getTerm();
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    int32_t term_id = std::stoi(data.data());
    if (term_id > curr_term_id_.getTerm()) {
        curr_state_ = RaftState::FOLLOWER;
        timer_manager_.GetVoteTimerRef().Stop();
        timer_manager_.GetElectionTimerRef().Start(
            RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
            randomer_.TakeOne(1000, 2000));
        timer_manager_.GetHeartbeatTimerRef().Stop();
    }
}

void RaftNode::ConnectToOtherNodes() {
    // Initial the rpc clients connecting to other nodes.
    for (const auto &endpoint : config_.GetOtherEndpoints()) {
        auto rpc_client = std::make_shared<rest_rpc::rpc_client>(endpoint.GetHost(),
                                                                 endpoint.GetPort());
        bool connected = rpc_client->connect();
        if (!connected) {
            RAFTCPP_LOG(RLL_INFO)
                << "Failed to connect to the node " << endpoint.ToString();
        }
        rpc_client->enable_auto_reconnect();
        all_rpc_clients_[NodeID(endpoint)] = rpc_client;
        RAFTCPP_LOG(RLL_INFO) << "This node " << config_.GetThisEndpoint().ToString()
                              << " succeeded to connect to the node "
                              << endpoint.ToString();
    }
}

void RaftNode::InitRpcHandlers() {
    // Register RPC handles.
    rpc_server_.register_handler<rest_rpc::Async>(
        RaftcppConstants::REQUEST_PRE_VOTE_RPC_NAME, &RaftNode::HandleRequestPreVote,
        this);
    rpc_server_.register_handler<rest_rpc::Async>(RaftcppConstants::REQUEST_VOTE_RPC_NAME,
                                                  &RaftNode::HandleRequestVote, this);
    rpc_server_.register_handler<rest_rpc::Async>(
        RaftcppConstants::REQUEST_HEARTBEAT, &RaftNode::HandleRequestHeartbeat, this);
    rpc_server_.register_handler<rest_rpc::Async>(RaftcppConstants::REQUEST_PULL_LOGS,
                                                  &RaftNode::HandleRequestPullLogs, this);
}

void RaftNode::StepBack(int32_t term_id) {
    timer_manager_.GetHeartbeatTimerRef().Stop();
    timer_manager_.GetVoteTimerRef().Stop();
    timer_manager_.GetElectionTimerRef().Reset(
        RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
    curr_state_ = RaftState::FOLLOWER;
    curr_term_id_.setTerm(term_id);
}

void RaftNode::HandleRequestPullLogs(rpc::RpcConn conn, std::string node_id_binary,
                                     int64_t committed_log_index) {
    RAFTCPP_LOG(RLL_INFO) << "HandleRequestPullLogs: committed_log_index="
                          << committed_log_index;
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (curr_state_ == RaftState::LEADER) {
        auto logs_to_be_sync = leader_log_manager_->PullLogs(
            NodeID::FromBinary(node_id_binary), committed_log_index);

    } else {
        // Log errors.
    }
}

void RaftNode::HandleRequestPushLogs(rpc::RpcConn conn, LogEntry log_entry) {
    RAFTCPP_LOG(RLL_INFO) << "HandleRequestPushLogs: log_entry.term_id="
                          << log_entry.term_id.ToHex()
                          << ", log_entry.log_index=" << log_entry.log_index
                          << ", log_entry.data=" << log_entry.data;
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    state_machine_->OnApply(log_entry.data);
    if (curr_state_ == RaftState::FOLLOWER) {
        /// Check log_index and term from RAFT protocol.
    } else {
        // handle candidate and leader.
        // Log errors.
    }
}

}  // namespace raftcpp::node
