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
    RAFTCPP_LOG(RLL_DEBUG) << "Node[" << this_node_id_ << "] request pre-vote";
    PreVoteRequest request;
    request.set_candidate_id(config_.GetThisId());
    request.set_term(curr_term_ + 1);
    // TODO implment last log
    request.set_last_log_index(0);
    request.set_last_log_term(0);
    // Count the results of the vote and have one vote at the beginning
    std::shared_ptr<int64_t> granted_votes = std::make_shared<int64_t>(1);
    auto context = std::make_shared<grpc::ClientContext>();
    auto response = std::make_shared<PreVoteResponse>();
    for (auto &peer : config_.GetOtherEndpoints()) {
        all_rpc_clients_[peer.first]->async()->HandleRequestPreVote(
            context.get(), &request, response.get(),
            [context, request, response, granted_votes, this](grpc::Status s) {
                if (!s.ok()) {
                    RAFTCPP_LOG(RLL_DEBUG) << s.error_code() << ": " << s.error_message();
                    return;
                }
                std::lock_guard<std::recursive_mutex> guard{mutex_};
                if (curr_term_ != request.term() ||
                    curr_state_ != RaftState::PRECANDIDATE)
                    return;
                if (response->vote_granted()) {
                    ++(*granted_votes);
                    if (config_.GreaterThanHalfNodesNum(*granted_votes)) {
                        RAFTCPP_LOG(RLL_DEBUG)
                            << "Node[" << this_node_id_
                            << "] receives majority pre-votes in term " << curr_term_;
                        BecomeCandidate();
                        RescheduleElection();
                        RequestVote();
                    }
                } else if (response->term() > curr_term_) {
                    RAFTCPP_LOG(RLL_DEBUG)
                        << "Node[" << this_node_id_ << "] finds a new leader Node["
                        << response->leader_id() << "] with term " << response->term()
                        << " and steps down in term " << curr_term_;
                    BecomeFollower(response->term(), response->leader_id());
                    RescheduleElection();
                }
            });
    }
}

grpc::Status RaftNode::HandleRequestPreVote(::grpc::ServerContext *context,
                                            const ::raftcpp::PreVoteRequest *request,
                                            ::raftcpp::PreVoteResponse *response) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_LOG(RLL_DEBUG) << "Node[" << this_node_id_
                           << "] received a RequestPreVote from node["
                           << request->candidate_id() << "] at term " << curr_term_;
    // The current node is leader or in the leader's lease
    if (curr_state_ == RaftState::LEADER ||
        curr_state_ == RaftState::FOLLOWER && leader_node_id_ != -1) {
        response->set_vote_granted(false);
        response->set_term(curr_term_);
        response->set_leader_id(leader_node_id_);
        return grpc::Status::OK;
    }
    // Reject to pre-vote for pre-candidates whose terms are shorter than or equal to this
    // node
    if (request->term() <= curr_term_) {
        response->set_vote_granted(false);
        response->set_term(curr_term_);
        response->set_leader_id(leader_node_id_);
        return grpc::Status::OK;
    }

    // TODO log match
    // Reject requests that entry older than this node
    // if (!logs.isUpToDate(request->last_log_index(), request->last_log_term())) {
    //     response->set_vote_granted(false);
    //     response->set_term(curr_term_);
    //     response->set_leader_id(leader_node_id_);
    //     return grpc::Status::OK;
    // }

    // Note: The election timer is reset after successful voting, which will help the
    // liveness problem of choosing the master under the condition of network instability
    RescheduleElection();

    response->set_vote_granted(true);
    response->set_term(curr_term_);
    response->set_leader_id(leader_node_id_);

    return grpc::Status::OK;
}

void RaftNode::RequestVote() {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_LOG(RLL_DEBUG) << "Node[" << this_node_id_ << "] request vote";
    VoteRequest request;
    request.set_candidate_id(config_.GetThisId());
    request.set_term(curr_term_);
    // TODO implment last log
    request.set_last_log_index(0);
    request.set_last_log_term(0);
    vote_for_ = this_node_id_;
    // Count the results of the vote and have one vote at the beginning
    std::shared_ptr<int64_t> granted_votes = std::make_shared<int64_t>(1);
    auto context = std::make_shared<grpc::ClientContext>();
    auto response = std::make_shared<VoteResponse>();
    for (auto &peer : config_.GetOtherEndpoints()) {
        all_rpc_clients_[peer.first]->async()->HandleRequestVote(
            context.get(), &request, response.get(),
            [context, request, response, granted_votes, this](grpc::Status s) {
                if (!s.ok()) {
                    RAFTCPP_LOG(RLL_DEBUG) << s.error_code() << ": " << s.error_message();
                    return;
                }
                std::lock_guard<std::recursive_mutex> guard{mutex_};
                if (curr_term_ != request.term() || curr_state_ != RaftState::CANDIDATE)
                    return;
                if (response->vote_granted()) {
                    ++(*granted_votes);
                    if (config_.GreaterThanHalfNodesNum(*granted_votes)) {
                        RAFTCPP_LOG(RLL_DEBUG)
                            << "Node[" << this_node_id_
                            << "] receives majority votes in term " << curr_term_;
                        BecomeLeader();
                    }
                } else if (response->term() > curr_term_) {
                    RAFTCPP_LOG(RLL_DEBUG)
                        << "Node[" << this_node_id_ << "] finds a new leader Node["
                        << response->leader_id() << "] with term " << response->term()
                        << " and steps down in term " << curr_term_;
                    BecomeFollower(response->term(), response->leader_id());
                    RescheduleElection();
                }
            });
    }
}

grpc::Status RaftNode::HandleRequestVote(::grpc::ServerContext *context,
                                         const ::raftcpp::VoteRequest *request,
                                         ::raftcpp::VoteResponse *response) {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    RAFTCPP_LOG(RLL_DEBUG) << "Node[" << this_node_id_
                           << "] received a RequestVote from node["
                           << request->candidate_id() << "] at term " << curr_term_;
    // Reject to vote for candidates whose terms are shorter than this node
    if (request->term() < curr_term_ ||
        // A situation in which more than one candidate request vote
        (request->term() == curr_term_ && vote_for_ != -1 &&
         vote_for_ != request->candidate_id())) {
        response->set_vote_granted(false);
        response->set_term(curr_term_);
        response->set_leader_id(leader_node_id_);
        return grpc::Status::OK;
    }

    if (request->term() > curr_term_) {
        BecomeFollower(request->term());
    }

    // TODO log match
    // Reject requests that entry older than this node
    // if (!logs.isUpToDate(request->last_log_index(), request->last_log_term())) {
    //     response->set_vote_granted(false);
    //     response->set_term(curr_term_);
    //     response->set_leader_id(leader_node_id_);
    //     return grpc::Status::OK;
    // }

    vote_for_ = request->candidate_id();

    // Note: The election timer is reset after successful voting, which will help the
    // liveness problem of choosing the master under the condition of network instability
    RescheduleElection();

    response->set_vote_granted(true);
    response->set_term(curr_term_);
    response->set_leader_id(leader_node_id_);

    return grpc::Status::OK;
}

grpc::Status RaftNode::HandleRequestAppendEntries(
    ::grpc::ServerContext *context, const ::raftcpp::AppendEntriesRequest *request,
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
    for (auto &[id, endpoint] : config_.GetOtherEndpoints()) {
        grpc::ChannelArguments args;
        auto channel =
            grpc::CreateChannel(endpoint.ToString(), grpc::InsecureChannelCredentials());
        all_rpc_clients_[id] = std::make_shared<raftrpc::Stub>(channel);
        RAFTCPP_LOG(RLL_INFO) << "This node " << config_.GetThisEndpoint().ToString()
                              << " succeeded to connect to the node "
                              << endpoint.ToString();
    }
}

void RaftNode::InitTimers() {
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_ELECTION, [this] {
        std::lock_guard<std::recursive_mutex> guard{mutex_};
        if (curr_state_ == RaftState::FOLLOWER) {
            BecomePreCandidate();
            RequestPreVote();
        } else if (curr_state_ == RaftState::PRECANDIDATE) {
            BecomeFollower(curr_term_);
        } else if (curr_state_ == RaftState::CANDIDATE) {
            RequestVote();
        } else if (curr_state_ == RaftState::LEADER) {
            if (QuorumActive()) {
                RAFTCPP_LOG(RLL_DEBUG)
                    << "Node[" << this_node_id_
                    << "] stepped down to follower since quorum is not active";
            }
        }
        RescheduleElection();
    });

    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_HEARTBEAT,
                                  std::bind(&RaftNode::BroadcastHeartbeat, this));

    timer_manager_->StartTimer(RaftcppConstants::TIMER_ELECTION,
                               GetRandomizedElectionTimeout());
    timer_manager_->Run();
}

void RaftNode::BecomeFollower(int64_t term, int64_t leader_id) {
    // In order to better control the time of resetting the election timer,
    // do not RescheduleElection() here first
    // RescheduleElection();
    timer_manager_->StopTimer(RaftcppConstants::TIMER_HEARTBEAT);
    curr_state_ = RaftState::FOLLOWER;
    curr_term_ = term;
    leader_node_id_ = leader_id;
    vote_for_ = -1;
    leader_log_manager_->Stop();
    non_leader_log_manager_->Run(leader_log_manager_->Logs(),
                                 leader_log_manager_->CommittedLogIndex());
    RAFTCPP_LOG(RLL_INFO) << "Node[" << this_node_id_ << "] became follower at term "
                          << term;
}

void RaftNode::BecomePreCandidate() {
    // Becoming a pre-candidate does not increase curr_term_ or change vote_for_.
    curr_state_ = RaftState::PRECANDIDATE;
    leader_node_id_ = -1;
    RAFTCPP_LOG(RLL_INFO) << "Node[" << this_node_id_ << "] became PreCandidate at term "
                          << curr_term_;
}

void RaftNode::BecomeCandidate() {
    curr_state_ = RaftState::CANDIDATE;
    ++curr_term_;
    vote_for_ = this_node_id_;
    RAFTCPP_LOG(RLL_INFO) << "Node[" << this_node_id_ << "] became Candidate at term "
                          << curr_term_;
}

void RaftNode::BecomeLeader() {
    curr_state_ = RaftState::LEADER;
    leader_node_id_ = this_node_id_;

    // After becoming a leader, the leader does not know the logs of other nodes,
    // so he needs to synchronize the logs with other nodes. The leader does not know
    // the status of other nodes in the cluster, so he chooses to keep trying.
    // Nextindex and matchindex are used to save the next log index to be synchronized
    // and the matched log index of other nodes respectively. The initialization value
    // of nextindex is lastindex+1, that is, the leader's last log sequence number +1.
    // Therefore, in fact, this log sequence number does not exist. Obviously, the leader
    // does not expect to synchronize successfully at one time, but takes out a value
    // to test. The initialization value of matchindex is 0, which is easy to understand.
    // Because it has not been synchronized with any node successfully, it is directly 0.
    for ([[maybe_unused]] auto &[id, _] : config_.GetOtherEndpoints()) {
        // TODO
        // nextIndex[id] = lastIndex() + 1
        // matchIndex[id] = 0;
    }

    RAFTCPP_LOG(RLL_INFO) << "Node[" << this_node_id_ << "] became Leader at term "
                          << curr_term_;

    leader_log_manager_->Run(non_leader_log_manager_->Logs(),
                             non_leader_log_manager_->CommittedLogIndex());
    // The leader cannot submit the entry of non current term, so submit an empty entry
    // to indirectly submit the entry of previous term
    LogEntry empty;
    leader_log_manager_->Push(curr_term_, empty);
    non_leader_log_manager_->Stop();

    BroadcastHeartbeat();

    timer_manager_->StartTimer(RaftcppConstants::TIMER_HEARTBEAT,
                               RaftcppConstants::DEFAULT_HEARTBEAT_INTERVAL_MS);
}

uint64_t RaftNode::GetRandomizedElectionTimeout() {
    return randomer_.TakeOne(RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_BASE_MS,
                             RaftcppConstants::DEFAULT_ELECTION_TIMER_TIMEOUT_TOP_MS);
}

void RaftNode::RescheduleElection() {
    timer_manager_->ResetTimer(RaftcppConstants::TIMER_ELECTION,
                               GetRandomizedElectionTimeout());
}

void RaftNode::ReplicateOneRound(int64_t node_id) {
    AppendEntriesRequest request;
    auto context = std::make_shared<grpc::ClientContext>();
    auto response = std::make_shared<AppendEntriesResponse>();
    all_rpc_clients_[node_id]->async()->HandleRequestAppendEntries(
        context.get(), &request, response.get(),
        [context, response, node_id, this](grpc::Status s) {
            if (!s.ok()) {
                std::lock_guard<std::recursive_mutex> guard{mutex_};
                actives_[node_id] = false;
                return;
            }
            std::lock_guard<std::recursive_mutex> guard{mutex_};
            // TODO asynchronous replication

            actives_[node_id] = true;
        });
}

// With the heartbeat, the follower's log will be replicated to the same location as the
// leader
void RaftNode::BroadcastHeartbeat() {
    std::lock_guard<std::recursive_mutex> guard{mutex_};
    if (curr_state_ != RaftState::LEADER) {
        return;
    }
    for ([[maybe_unused]] auto &[id, _] : config_.GetOtherEndpoints()) {
        // This is asynchronous replication
        ReplicateOneRound(id);
    }
}

bool RaftNode::QuorumActive() {
    size_t active = 1;
    for (auto &item : actives_) {
        if (item.second) {
            active++;
            item.second = false;
        }
    }
    return config_.GreaterThanHalfNodesNum(active);
}

}  // namespace raftcpp::node
