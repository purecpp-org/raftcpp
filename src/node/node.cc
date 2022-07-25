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
      this_node_id_(config.GetThisId()),
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
              RAFTCPP_CHECK(leader_node_id_ > 0);
              RAFTCPP_CHECK(all_rpc_clients_.count(leader_node_id_) == 1);
              return all_rpc_clients_[leader_node_id_];
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

void RaftNode::PushEntry(LogEntry &entry) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_CHECK(curr_state_ == RaftState::LEADER);
    // This is leader code path.
    leader_log_manager_->Push(curr_term_, entry);
    //    AsyncAppendLogsToFollowers(entry);
    // TODO(qwang)

    BroadcastHeartbeat();
}

void RaftNode::RequestPreVote() {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_LOG(RLL_DEBUG) << "Node " << this->config_.GetThisEndpoint().ToString()
                           << " request prevote";
    // only follower can request pre_vote
    if (curr_state_ != RaftState::FOLLOWER) return;

    // Note that it's to clear the set.
    responded_pre_vote_nodes_.clear();
    ++curr_term_;
    // Pre vote for myself.
    responded_pre_vote_nodes_.insert(this->config_.GetThisEndpoint().ToString());
    for (auto &item : all_rpc_clients_) {
        auto &rpc_client = item.second;
        RAFTCPP_LOG(RLL_DEBUG) << "RequestPreVote Node "
                               << this->config_.GetThisEndpoint().ToString()
                               << " request_vote_callback client" << rpc_client;

        // Construct pre vote request to other nodes(followers and candidates).
        PreVoteRequest request;
        request.set_candidate_id(config_.GetThisId());
        request.set_term(curr_term_);
        
        // TODO implment last log
        request.set_last_log_index(0);
        request.set_last_log_term(0);

        // We can receive response from other handle pre votes messages;
        PreVoteResponse response;
        grpc::ClientContext context;
        auto s = rpc_client->HandleRequestPreVote(&context, request, &response);
        if (s.ok()) {
            // TODO: Handle empty string
            const asio::error_code ec;
            OnPreVote(ec, response);
        } else {
            std::cout << s.error_code() << ": " << s.error_message() << std::endl;
        }
    }
}

grpc::Status RaftNode::HandleRequestPreVote(::grpc::ServerContext *context,
                                            const ::raftcpp::PreVoteRequest *request,
                                            ::raftcpp::PreVoteResponse *response) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (this->config_.GetThisId() == request->candidate_id())
        return grpc::Status::OK;
    RAFTCPP_LOG(RLL_DEBUG) << "HandleRequestPreVote this node "
                           << this->config_.GetThisEndpoint().ToString()
                           << " Received a RequestPreVote from node "
                           << request->candidate_id() << " term_id=" << request->term();

    if (curr_state_ == RaftState::FOLLOWER) {
        if (request->term() > curr_term_) {
            curr_term_ = request->term();
            timer_manager_->ResetTimer(RaftcppConstants::TIMER_ELECTION,
                                       RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS +
                                           randomer_.TakeOne(1000, 2000));

        }
    } else if (curr_state_ == RaftState::CANDIDATE || curr_state_ == RaftState::LEADER) {
        if (request->term() > curr_term_) {
            RAFTCPP_LOG(RLL_DEBUG)
                << "HandleRequestPreVote Received a RequestPreVote,now  step down";
            StepBack(request->term());
            
        }
    }
    return grpc::Status::OK;
}

void RaftNode::OnPreVote(const asio::error_code &ec, ::raftcpp::PreVoteResponse response) {
    // RAFTCPP_LOG(RLL_DEBUG) << "Received response of request_vote from node " << data
    //                        << ", error code= " << ec.message();
    if (ec.message() == "Transport endpoint is not connected") return;
    // Exclude itself under multi-thread
    // RAFTCPP_LOG(RLL_DEBUG) << "OnPreVote Response node: " << data.data()
    //                        << " this node:" << this->config_.GetThisEndpoint().ToString();
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    //responded_pre_vote_nodes_.insert(data.data());
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
        ++curr_term_;
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
        request.set_candidate_id(config_.GetThisId());
        request.set_term(curr_term_);
        
        // TODO implment last log
        request.set_last_log_index(0);
        request.set_last_log_term(0);

        VoteResponse response;
        grpc::ClientContext context;
        auto s = item.second->HandleRequestVote(&context, request, &response);

        if (s.ok()) {
            const asio::error_code ec;
            OnVote(ec, response);
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
        if (request->term() > curr_term_) {
            curr_term_ = request->term();
            RescheduleElection();
            
        }
    } else if (curr_state_ == RaftState::CANDIDATE || curr_state_ == RaftState::LEADER) {
        // TODO(qwang):
        if (request->term() > curr_term_) {
            StepBack(request->term());
            
        }
    }
    return grpc::Status::OK;
}

void RaftNode::OnVote(const asio::error_code &ec, ::raftcpp::VoteResponse response) {
    if (ec.message() == "Transport endpoint is not connected") return;
    // Exclude itself under multi-thread
    // RAFTCPP_LOG(RLL_DEBUG) << "OnVote Response node: " << data.data()
    //                        << " this node:" << this->config_.GetThisEndpoint().ToString();
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    // responded_vote_nodes_.insert(data.data());
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
        ++curr_term_;
        timer_manager_->StopTimer(RaftcppConstants::TIMER_VOTE);
        timer_manager_->StopTimer(RaftcppConstants::TIMER_ELECTION);
        timer_manager_->ResetTimer(RaftcppConstants::TIMER_HEARTBEAT,
                                   RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS);

        BroadcastHeartbeat();

        // This node became the leader, so run the leader log manager.
        leader_log_manager_->Run(non_leader_log_manager_->Logs(),
                                 non_leader_log_manager_->CommittedLogIndex());
        LogEntry entry;
        leader_log_manager_->Push(curr_term_, entry);  // no-op
        non_leader_log_manager_->Stop();
    } else {
    }
}


grpc::Status RaftNode::HandleRequestAppendEntries(::grpc::ServerContext *context,
                                       const ::raftcpp::AppendEntriesRequest *request,
                                       ::raftcpp::AppendEntriesResponse *response) {
    // RAFTCPP_LOG(RLL_INFO) << "HandleRequestPushLogs: log_entry.term_id="
    //                       << request->log().termid()
    //                       << ", committed_log_index=" << request->commited_log_index()
    //                       << ", log_entry.log_index=" << request->log().log_index()
    //                       << ", log_entry.data=" << request->log().data();

    std::lock_guard<std::recursive_mutex> guard{mutex_};

    // Reject log replication requests with a leader whose term is less than this node
    if (request->term() < curr_term_) {
        response->set_success(false);
        response->set_term(curr_term_);
        response->set_leader_id(leader_node_id_);
        return grpc::Status::OK;
    }

    // If the other node's term is greater than this node, 
    // or this node are a candidate who lost the election in the same term,
    // then become a follower.
    if (request->term() > curr_term_ ||
                (request->term() == curr_term_ && curr_state_ == RaftState::CANDIDATE)) {
        BecomeFollower(request->term(), request->leader_id());
    }

    RescheduleElection();

    // TODO Reject erroneous log append requests

    // TODO Fast fallback to speed up the resolution of log conflicts between nodes

    response->set_success(true);
    response->set_term(curr_term_);
    response->set_leader_id(leader_node_id_);
    return grpc::Status::OK;
}

void RaftNode::ConnectToOtherNodes() {
    // Initial the rpc clients connecting to other nodes.
    for (auto& [id, endpoint] : config_.GetOtherEndpoints()) {
        grpc::ChannelArguments args;
        auto channel = grpc::CreateChannel(endpoint.ToString(), grpc::InsecureChannelCredentials());
        all_rpc_clients_[id] = std::make_shared<raftrpc::Stub>(channel);
        RAFTCPP_LOG(RLL_INFO) << "This node " << config_.GetThisEndpoint().ToString()
                              << " succeeded to connect to the node "
                              << endpoint.ToString();
    }
}

void RaftNode::InitTimers() {
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_ELECTION,
                                  std::bind(&RaftNode::RequestPreVote, this));
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_HEARTBEAT,
                                  std::bind(&RaftNode::BroadcastHeartbeat, this));
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_VOTE,
                                  std::bind(&RaftNode::RequestVote, this));

    timer_manager_->StartTimer(RaftcppConstants::TIMER_ELECTION,
                               randomer_.TakeOne(1000, 2000));
    timer_manager_->Run();
}

void RaftNode::StepBack(int64_t term_id) {
    timer_manager_->StopTimer(RaftcppConstants::TIMER_HEARTBEAT);
    timer_manager_->StopTimer(RaftcppConstants::TIMER_VOTE);
    RescheduleElection();

    curr_state_ = RaftState::FOLLOWER;
    leader_log_manager_->Stop();
    non_leader_log_manager_->Run(leader_log_manager_->Logs(),
                                 leader_log_manager_->CommittedLogIndex());
    curr_term_ = term_id;
}

void RaftNode::BecomeFollower(int64_t term, int64_t leader_id = -1) {

}

void RaftNode::BecomePerCandidate() {

}

void RaftNode::BecomeCandidate() {

}

void RaftNode::BecomeLeader() {

}

uint64_t RaftNode::GetRandomizedElectionTimeout() {
    return randomer_.TakeOne(RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_BASE_MS,
                            RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_TOP_MS);
}

void RaftNode::RescheduleElection() {
    timer_manager_->ResetTimer(RaftcppConstants::TIMER_ELECTION, GetRandomizedElectionTimeout());
}

void RaftNode::ReplicateOneRound(int64_t node_id) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (curr_state_ != RaftState::LEADER) {
        return;
    }
    AppendEntriesRequest request;
    auto context = std::make_shared<grpc::ClientContext>();
    auto response = std::make_shared<AppendEntriesResponse>();
    all_rpc_clients_[node_id]->async()
        ->HandleRequestAppendEntries(context.get(), &request, response.get(),
        [context, response](grpc::Status s){
        // TODO asynchronous replication
    });
}

// With the heartbeat, the follower's log will be replicated to the same location as the leader
void RaftNode::BroadcastHeartbeat() {
    for ([[maybe_unused]] auto& [id, _] : config_.GetOtherEndpoints()) {
        // This is asynchronous replication
        ReplicateOneRound(id);
    }
}


}  // namespace raftcpp::node
