

#include "node.h"

#include <google/protobuf/empty.pb.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/support/channel_arguments.h>
#include <grpcpp/support/status.h>

#include <iostream>
#include <memory>
#include <mutex>

#include "src/common/constants.h"
#include "src/common/logging.h"
#include "src/common/type_def.h"

namespace raftcpp::node {

RaftNode::RaftNode(std::shared_ptr<StateMachine> state_machine,
                   // std::unique_ptr<grpc::Server> rpc_server,
                   const common::Config &config, RaftcppLogLevel severity)
    :  // rpc_server_(std::move(rpc_server)),
      raftrpc::Service(),
      config_(config),
      this_node_id_(config.GetThisEndpoint()),
      timer_manager_(std::make_shared<common::TimerManager>()),
      leader_log_manager_(std::make_unique<LeaderLogManager>(
          this_node_id_, [this]() -> AllRpcClientType { return all_rpc_clients_; },
          timer_manager_)),

      non_leader_log_manager_(std::make_unique<NonLeaderLogManager>(
          this_node_id_, state_machine,
          [this]() {
              std::lock_guard<std::recursive_mutex> guard{mutex_};
              return curr_state_ == RaftState::LEADER;
          },
          [this]() -> std::shared_ptr<raftrpc::Stub> {
              std::lock_guard<std::recursive_mutex> guard{mutex_};
              if (curr_state_ == RaftState::LEADER) {
                  return nullptr;
              }
              RAFTCPP_CHECK(leader_node_id_ != nullptr);
              RAFTCPP_CHECK(all_rpc_clients_.count(*leader_node_id_) == 1);
              return all_rpc_clients_[*leader_node_id_];
          },
          timer_manager_)) {
    std::string log_name = "node-" + config_.GetThisEndpoint().ToString() + ".log";
    replace(log_name.begin(), log_name.end(), '.', '-');
    replace(log_name.begin(), log_name.end(), ':', '-');
    raftcpp::RaftcppLog::StartRaftcppLog(log_name, severity, 10, 3);
}

void RaftNode::Init() {
    ConnectToOtherNodes();
    InitTimers();
}

bool RaftNode::IsLeader() const {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    return curr_state_ == RaftState::LEADER;
}

RaftNode::~RaftNode() {
    leader_log_manager_->Stop();
    non_leader_log_manager_->Stop();
}

void RaftNode::PushRequest(const std::shared_ptr<PushLogsRequest> &request) {
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
                               << " request_vote_callback client" << rpc_client;

        // Construct pre vote request to other nodes(followers and candidates).
        PreVoteRequest request;
        request.set_termid(curr_term_id_.getTerm());
        request.set_endpoint(config_.GetThisEndpoint().ToString());

        // We can receive response from other handle pre votes messages;
        PreVoteResponse response;
        grpc::ClientContext context;
        auto s = rpc_client->HandleRequestPreVote(&context, request, &response);
        if (s.ok()) {
            // TODO: Handle empty string
            const asio::error_code ec;
            OnPreVote(ec, response.data());
        } else {
            std::cout << s.error_code() << ": " << s.error_message() << std::endl;
        }
    }
}

grpc::Status RaftNode::HandleRequestPreVote(::grpc::ServerContext *context,
                                            const ::raftcpp::PreVoteRequest *request,
                                            ::raftcpp::PreVoteResponse *response) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (this->config_.GetThisEndpoint().ToString() == request->endpoint())
        return grpc::Status::OK;
    RAFTCPP_LOG(RLL_DEBUG) << "HandleRequestPreVote this node "
                           << this->config_.GetThisEndpoint().ToString()
                           << " Received a RequestPreVote from node "
                           << request->endpoint() << " term_id=" << request->termid();

    if (curr_state_ == RaftState::FOLLOWER) {
        if (request->termid() > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(request->termid());
            timer_manager_->ResetTimer(RaftcppConstants::TIMER_ELECTION,
                                       RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
                                           randomer_.TakeOne(1000, 2000));
            response->set_endpoint(config_.GetThisEndpoint().ToString());
        }
    } else if (curr_state_ == RaftState::CANDIDATE || curr_state_ == RaftState::LEADER) {
        if (request->termid() > curr_term_id_.getTerm()) {
            RAFTCPP_LOG(RLL_DEBUG)
                << "HandleRequestPreVote Received a RequestPreVote,now  step down";
            StepBack(request->termid());
            response->set_endpoint(config_.GetThisEndpoint().ToString());
        }
    }
    return grpc::Status::OK;
}

void RaftNode::OnPreVote(const asio::error_code &ec, std::string_view data) {
    RAFTCPP_LOG(RLL_DEBUG) << "Received response of request_vote from node " << data
                           << ", error code= " << ec.message();
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
        timer_manager_->StopTimer(RaftcppConstants::TIMER_ELECTION);
        timer_manager_->StartTimer(RaftcppConstants::TIMER_VOTE,
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
        VoteRequest request;
        request.set_termid(curr_term_id_.getTerm());
        request.set_endpoint(config_.GetThisEndpoint().ToString());

        VoteResponse response;
        grpc::ClientContext context;
        auto s = item.second->HandleRequestVote(&context, request, &response);

        if (s.ok()) {
            const asio::error_code ec;
            OnVote(ec, response.data());
        } else {
            std::cout << s.error_code() << ": " << s.error_message() << std::endl;
        }
    }
}

grpc::Status RaftNode::HandleRequestVote(::grpc::ServerContext *context,
                                         const ::raftcpp::VoteRequest *request,
                                         ::raftcpp::VoteResponse *response) {
    RAFTCPP_LOG(RLL_DEBUG) << "Node " << this->config_.GetThisEndpoint().ToString()
                           << " response vote";
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (curr_state_ == RaftState::FOLLOWER) {
        if (request->termid() > curr_term_id_.getTerm()) {
            curr_term_id_.setTerm(request->termid());
            timer_manager_->ResetTimer(
                RaftcppConstants::TIMER_ELECTION,
                RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);
            response->set_endpoint(config_.GetThisEndpoint().ToString());
        }
    } else if (curr_state_ == RaftState::CANDIDATE || curr_state_ == RaftState::LEADER) {
        // TODO(qwang):
        if (request->termid() > curr_term_id_.getTerm()) {
            StepBack(request->termid());
            response->set_endpoint(config_.GetThisEndpoint().ToString());
        }
    }
    return grpc::Status::OK;
}

void RaftNode::OnVote(const asio::error_code &ec, std::string_view data) {
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

        timer_manager_->StopTimer(RaftcppConstants::TIMER_VOTE);
        timer_manager_->StopTimer(RaftcppConstants::TIMER_ELECTION);
        timer_manager_->ResetTimer(RaftcppConstants::TIMER_HEARTBEAT,
                                   RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS);

        this->RequestHeartbeat();
        // This node became the leader, so run the leader log manager.
        leader_log_manager_->Run(non_leader_log_manager_->Logs(),
                                 non_leader_log_manager_->CommittedLogIndex());
        leader_log_manager_->Push(curr_term_id_,
                                  std::make_shared<PushLogsRequest>());  // no-op
        non_leader_log_manager_->Stop();
    } else {
    }
}

void RaftNode::RequestHeartbeat() {
    for (auto &item : all_rpc_clients_) {
        RAFTCPP_LOG(RLL_DEBUG) << "Send a heartbeat to node.";
        HeartbeatRequest request;
        request.set_termid(curr_term_id_.getTerm());
        request.set_node_id(this_node_id_.ToBinary());

        grpc::ClientContext context;
        HeartbeatResponse response;
        auto s = item.second.get()->HandleRequestHeartbeat(&context, request, &response);
        if (s.ok()) {
            const asio::error_code ec;
            OnHeartbeat(ec, std::to_string(response.termid()));
        } else {
            std::cout << s.error_code() << ": " << s.error_message() << std::endl;
        }
        if (response.termid() > curr_term_id_.getTerm()) {
            // stepdown
        }
    }
}

grpc::Status RaftNode::HandleRequestHeartbeat(::grpc::ServerContext *context,
                                              const ::raftcpp::HeartbeatRequest *request,
                                              ::raftcpp::HeartbeatResponse *response) {
    auto source_node_id = NodeID::FromBinary(request->node_id());
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (curr_state_ == RaftState::FOLLOWER || curr_state_ == RaftState::CANDIDATE) {
        leader_node_id_ = std::make_unique<NodeID>(source_node_id);
        RAFTCPP_LOG(RLL_DEBUG) << "HandleRequestHeartbeat node "
                               << this->config_.GetThisEndpoint().ToString()
                               << "received a heartbeat from leader(node_id="
                               << source_node_id.ToHex() << ")."
                               << " curr_term_id_:" << curr_term_id_.getTerm()
                               << " receive term_id:" << request->termid()
                               << " update term_id";
        timer_manager_->StartTimer(RaftcppConstants::TIMER_ELECTION,
                                   RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
                                       randomer_.TakeOne(1000, 2000));
        curr_term_id_.setTerm(request->termid());
        // only for follower first start
        if (!non_leader_log_manager_->IsRunning()) {
            non_leader_log_manager_->Run(leader_log_manager_->Logs(),
                                         leader_log_manager_->CommittedLogIndex());
        }
    } else {
        if (request->termid() >= curr_term_id_.getTerm()) {
            RAFTCPP_LOG(RLL_DEBUG)
                << "HandleRequestHeartbeat node "
                << this->config_.GetThisEndpoint().ToString()
                << "received a heartbeat from leader."
                << " curr_term_id_:" << curr_term_id_.getTerm()
                << " receive term_id:" << request->termid() << " StepBack";
            curr_term_id_.setTerm(request->termid());
            timer_manager_->StopTimer(RaftcppConstants::TIMER_VOTE);
            timer_manager_->StopTimer(RaftcppConstants::TIMER_HEARTBEAT);
            curr_state_ = RaftState::FOLLOWER;
            leader_log_manager_->Stop();
            non_leader_log_manager_->Run(leader_log_manager_->Logs(),
                                         leader_log_manager_->CommittedLogIndex());
            timer_manager_->StartTimer(RaftcppConstants::TIMER_ELECTION,
                                       RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
                                           randomer_.TakeOne(1000, 2000));

        } else {
            /// Code path of myself is leader.
            RAFTCPP_LOG(RLL_DEBUG)
                << "HandleRequestHeartbeat node "
                << this->config_.GetThisEndpoint().ToString()
                << "received a heartbeat from leader and send response";
            response->set_termid(curr_term_id_.getTerm());
        }
    }

    return grpc::Status::OK;
}

void RaftNode::OnHeartbeat(const asio::error_code &ec, std::string_view data) {
    if (ec.message() == "Transport endpoint is not connected") return;
    RAFTCPP_LOG(RLL_DEBUG) << "Received a response heartbeat from node.term_id："
                           << std::stoi(data.data())
                           << "more than currentid:" << curr_term_id_.getTerm();
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    int32_t term_id = std::stoi(data.data());
    if (term_id > curr_term_id_.getTerm()) {
        curr_state_ = RaftState::FOLLOWER;
        leader_log_manager_->Stop();
        non_leader_log_manager_->Run(leader_log_manager_->Logs(),
                                     leader_log_manager_->CommittedLogIndex());
        timer_manager_->StopTimer(RaftcppConstants::TIMER_VOTE);
        timer_manager_->StartTimer(RaftcppConstants::TIMER_ELECTION,
                                   RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
                                       randomer_.TakeOne(1000, 2000));
        timer_manager_->StopTimer(RaftcppConstants::TIMER_HEARTBEAT);
    }
}

void RaftNode::ConnectToOtherNodes() {
    // Initial the rpc clients connecting to other nodes.
    for (const auto &endpoint : config_.GetOtherEndpoints()) {
        grpc::ChannelArguments args;
        auto channel =
            grpc::CreateChannel(endpoint.ToString(), grpc::InsecureChannelCredentials());
        all_rpc_clients_[NodeID(endpoint)] = std::make_shared<raftrpc::Stub>(channel);
        RAFTCPP_LOG(RLL_INFO) << "This node " << config_.GetThisEndpoint().ToString()
                              << " succeeded to connect to the node "
                              << endpoint.ToString();
    }
}

void RaftNode::InitTimers() {
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_ELECTION,
                                  std::bind(&RaftNode::RequestPreVote, this));
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_HEARTBEAT,
                                  std::bind(&RaftNode::RequestHeartbeat, this));
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_VOTE,
                                  std::bind(&RaftNode::RequestVote, this));

    timer_manager_->StartTimer(RaftcppConstants::TIMER_ELECTION,
                               randomer_.TakeOne(1000, 2000));
    timer_manager_->Run();
}

void RaftNode::StepBack(int32_t term_id) {
    timer_manager_->StopTimer(RaftcppConstants::TIMER_HEARTBEAT);
    timer_manager_->StopTimer(RaftcppConstants::TIMER_VOTE);
    timer_manager_->ResetTimer(RaftcppConstants::TIMER_ELECTION,
                               RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_MS);

    curr_state_ = RaftState::FOLLOWER;
    leader_log_manager_->Stop();
    non_leader_log_manager_->Run(leader_log_manager_->Logs(),
                                 leader_log_manager_->CommittedLogIndex());
    curr_term_id_.setTerm(term_id);
}

grpc::Status RaftNode::HandleRequestPushLogs(::grpc::ServerContext *context,
                                             const ::raftcpp::PushLogsRequest *request,
                                             ::google::protobuf::Empty *response) {
    RAFTCPP_LOG(RLL_INFO) << "HandleRequestPushLogs: log_entry.term_id="
                          << request->log().termid()
                          << ", committed_log_index=" << request->commited_log_index()
                          << ", log_entry.log_index=" << request->log().log_index()
                          << ", log_entry.data=" << request->log().data();
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    //    non_leader_log_manager_->Push(committed_log_index, log_entry);
    if (curr_state_ == RaftState::FOLLOWER) {
        // check sender's term

        /// Check log_index and term from RAFT protocol.
        auto request_term = request->log().termid();
        auto cur_term = curr_term_id_.getTerm();
        if (request_term < cur_term) {  // outdate
            return grpc::Status::OK;    // XXX: Maybe Cancel
        } else if (request_term > cur_term) {
            curr_term_id_.setTerm(request_term);
        }
        non_leader_log_manager_->Push(request->commited_log_index(),
                                      request->pre_log_term(), request->log());
    } else {
        // handle candidate and leader.
        // Log errors.
    }
    response->CopyFrom(google::protobuf::Empty());
    return grpc::Status::OK;
}

}  // namespace raftcpp::node
