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
    std::string log_name = "/tmp/node-" + config_.GetThisEndpoint().ToString() + ".log";
    replace(log_name.begin(), log_name.end(), '.', '-');
    replace(log_name.begin(), log_name.end(), ':', '-');
    raftcpp::RaftcppLog::StartRaftcppLog(log_name, RaftcppLogLevel::RLL_DEBUG, 10, 3);
    InitRpcHandlers();
    ConnectToOtherNodes();
    // Starting timer manager should be invoked after all rpc initialization.
    timer_manager_.Start();
}

RaftNode::~RaftNode() {}

void RaftNode::RequestPreVote() {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    // Note that it's to clear the set.
    responded_pre_vote_nodes_.clear();
    // Pre vote for myself.
    responded_pre_vote_nodes_.insert(this->config_.GetThisEndpoint().ToString());
    for (const auto &rpc_client : rpc_clients_) {
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

void RaftNode::OnRequestPreVote(rpc::RpcConn conn, const std::string &endpoint_str,
                                int32_t termid) {
    RAFTCPP_LOG(RLL_DEBUG) << "Received a RequestPreVote from node " << endpoint_str
                           << " termid=" << termid;

    std::lock_guard<std::recursive_mutex> guard{mutex_};
    const auto req_id = conn.lock()->request_id();
    auto conn_sp = conn.lock();
    if (curr_state_ == RaftState::FOLLOWER) {
        if (termid > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(termid);
            timer_manager_.GetElectionTimerRef().Reset(
                RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        } else {
            if (conn_sp) {
                conn_sp->response(req_id, "reject");
            }
        }

    } else if (curr_state_ == RaftState::CANDIDATE) {
        // TODO(qwang): step down
        if (termid > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(termid);
            curr_state_ = RaftState::FOLLOWER;
            timer_manager_.GetElectionTimerRef().Reset(
                RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        } else {
            if (conn_sp) {
                conn_sp->response(req_id, "reject");
            }
        }
    } else if (curr_state_ == RaftState::LEADER) {
        // TODO(qwang):
        if (termid > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(termid);
            curr_state_ = RaftState::FOLLOWER;
            timer_manager_.GetElectionTimerRef().Reset(
                RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        } else {
            if (conn_sp) {
                conn_sp->response(req_id, "reject");
            }
        }
    }
}

void RaftNode::OnPreVote(const boost::system::error_code &ec, string_view data) {
    RAFTCPP_LOG(RLL_DEBUG) << "Received response of request_vote from node " << data
                           << ", error code=" << ec.message();
    if (ec.message() == "Transport endpoint is not connected") return;
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (strcmp(data.data(), "reject") != 0) responded_pre_vote_nodes_.insert(data.data());
    if (this->config_.GreaterThanHalfNodesNum(responded_pre_vote_nodes_.size()) &&
        this->curr_state_ == RaftState::FOLLOWER) {
        // There are greater than a half of the nodes responded the pre vote request,
        // so stop the election timer and send the vote rpc request to all nodes.
        //
        // TODO(qwang): We should post these rpc methods to a separated io service.
        curr_state_ = RaftState::CANDIDATE;
        RAFTCPP_LOG(RLL_INFO) << "This node has became a candidate now.";
        timer_manager_.GetElectionTimerRef().Stop();
        timer_manager_.GetVoteTimerRef().Start(
            RaftcppConstants::DEFAULT_VOTE_TIMER_TIMEOUT_MS);
        //        io_service_.post([this]() { this->RequestVote(); });
        this->RequestVote();
    } else {
    }
}

void RaftNode::RequestVote() {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_LOG(RLL_DEBUG) << "Request vote.";

    // Note that it's to clear the set.
    // TODO(qwang): Considering that whether it shouldn't clear this in every request,
    // because some nodes may responds the last request.
    responded_vote_nodes_.clear();
    // Vote for myself.
    responded_vote_nodes_.insert(this->config_.GetThisEndpoint().ToString());
    for (const auto &rpc_client : rpc_clients_) {
        auto request_vote_callback = [this](const boost::system::error_code &ec,
                                            string_view data) {
            //            io_service_.post([this, ec, data]() { this->OnVote(ec, data);
            //            });
            this->OnVote(ec, data);
        };
        rpc_client->async_call<0>(
            RaftcppConstants::REQUEST_VOTE_RPC_NAME, std::move(request_vote_callback),
            this->config_.GetThisEndpoint().ToString(), curr_term_id_.getTerm());
    }
}

void RaftNode::OnRequestVote(rpc::RpcConn conn, const std::string &endpoint_str,
                             int32_t termid) {
    RAFTCPP_LOG(RLL_DEBUG) << "OnRequestVote";
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    const auto req_id = conn.lock()->request_id();
    auto conn_sp = conn.lock();
    if (curr_state_ == RaftState::FOLLOWER) {
        if (termid > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(termid);
            timer_manager_.GetElectionTimerRef().Stop();
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        } else {
            if (conn_sp) {
                conn_sp->response(req_id, "reject");
            }
        }
    } else if (curr_state_ == RaftState::CANDIDATE) {
        // TODO(qwang):
        if (termid > curr_term_id_.getTerm()) {
            curr_state_ = RaftState::FOLLOWER;
            curr_term_id_.setTerm(termid);
            timer_manager_.GetElectionTimerRef().Stop();
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        } else {
            if (conn_sp) {
                conn_sp->response(req_id, "reject");
            }
        }

    } else if (curr_state_ == RaftState::LEADER) {
        // TODO(qwang):
        if (termid > curr_term_id_.getTerm()) {
            curr_state_ = RaftState::FOLLOWER;
            curr_term_id_.setTerm(termid);
            timer_manager_.GetElectionTimerRef().Stop();
            if (conn_sp) {
                conn_sp->response(req_id, config_.GetThisEndpoint().ToString());
            }
        } else {
            if (conn_sp) {
                conn_sp->response(req_id, "reject");
            }
        }
    }
}

void RaftNode::OnVote(const boost::system::error_code &ec, string_view data) {
    if (ec.message() == "Transport endpoint is not connected") return;
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (strcmp(data.data(), "reject") != 0) responded_vote_nodes_.insert(data.data());
    if (this->config_.GreaterThanHalfNodesNum(responded_vote_nodes_.size()) &&
        this->curr_state_ == RaftState::CANDIDATE) {
        // There are greater than a half of the nodes responded the pre vote request,
        // so stop the election timer and send the vote rpc request to all nodes.
        //
        // TODO(qwang): We should post these rpc methods to a separated io service.
        curr_state_ = RaftState::LEADER;
        RAFTCPP_LOG(RLL_INFO) << "This node has became a leader now";
        timer_manager_.GetVoteTimerRef().Stop();
        timer_manager_.GetHeartbeatTimerRef().Reset(
            RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS);
        this->RequestHeartbeat();
    } else {
    }
}

void RaftNode::RequestHeartbeat() {
    for (const auto &rpc_client : rpc_clients_) {
        RAFTCPP_LOG(RLL_DEBUG) << "Send a heartbeat to node.";
        rpc_client->async_call<0>(
            RaftcppConstants::REQUEST_HEARTBEAT,
            /*callback=*/[](const boost::system::error_code &ec, string_view data) {},
            curr_term_id_.getTerm());
    }
}

void RaftNode::OnRequestHeartbeat(rpc::RpcConn conn, int32_t termid) {
    RAFTCPP_LOG(RLL_DEBUG) << "Received a heartbeat from leader.";
    timer_manager_.GetElectionTimerRef().Start(
        RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
    if (termid > curr_term_id_.getTerm()) {
        curr_term_id_.setTerm(termid);
        if(curr_state_ == RaftState::LEADER)
            curr_state_ = RaftState::FOLLOWER;
    }
}

void RaftNode::ConnectToOtherNodes() {
    // Initial the rpc clients connecting to other nodes.
    for (const auto &endpoint : config_.GetOtherEndpoints()) {
        auto rpc_client = std::make_shared<rest_rpc::rpc_client>(endpoint.GetHost(),
                                                                 endpoint.GetPort());
        bool connected = rpc_client->connect();
        if (!connected) {
            RAFTCPP_LOG(RLL_DEBUG)
                << "Failed to connect to the node " << endpoint.ToString();
        }
        rpc_client->enable_auto_heartbeat();
        rpc_client->enable_auto_reconnect();
        RAFTCPP_LOG(RLL_DEBUG) << "Succeeded to connect to the node "
                               << endpoint.ToString();
        rpc_clients_.push_back(rpc_client);
    }
}

void RaftNode::InitRpcHandlers() {
    // Register RPC handles.
    rpc_server_.register_handler<rest_rpc::Async>(
        RaftcppConstants::REQUEST_PRE_VOTE_RPC_NAME, &RaftNode::OnRequestPreVote, this);
    rpc_server_.register_handler<rest_rpc::Async>(RaftcppConstants::REQUEST_VOTE_RPC_NAME,
                                                  &RaftNode::OnRequestVote, this);
    rpc_server_.register_handler<rest_rpc::Async>(RaftcppConstants::REQUEST_HEARTBEAT,
                                                  &RaftNode::OnRequestHeartbeat, this);
}

}  // namespace raftcpp::node
