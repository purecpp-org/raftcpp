#pragma once

#include <grpcpp/client_context.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "proto/raft.grpc.pb.h"
#include "proto/raft.pb.h"
#include "src/common/config.h"
#include "src/common/endpoint.h"
#include "src/common/logging.h"
#include "src/common/timer.h"
#include "src/common/timer_manager.h"
#include "src/common/type_def.h"
#include "src/log_manager/blocking_queue_interface.h"
#include "src/log_manager/blocking_queue_mutex_impl.h"
#include "src/log_manager/leader_log_manager.h"
#include "src/log_manager/non_leader_log_manager.h"
#include "src/statemachine/state_machine.h"

namespace raftcpp {
namespace node {

class RaftNode : public raftrpc::Service, public std::enable_shared_from_this<RaftNode> {
public:
    RaftNode(
        std::shared_ptr<StateMachine> state_machine,
        // std::unique_ptr<grpc::Server> rpc_server,
        const common::Config &config,
        const raftcpp::RaftcppLogLevel severity = raftcpp::RaftcppLogLevel::RLL_DEBUG);

    // explicit RaftNode() = default;

    ~RaftNode();

    void Init();

    bool IsLeader() const;

    void PushEntry(LogEntry &entry);

    void RequestPreVote();

    grpc::Status HandleRequestPreVote(::grpc::ServerContext *context,
                                      const ::raftcpp::PreVoteRequest *request,
                                      ::raftcpp::PreVoteResponse *response);
                                      
    void RequestVote();

    grpc::Status HandleRequestVote(::grpc::ServerContext *context,
                                   const ::raftcpp::VoteRequest *request,
                                   ::raftcpp::VoteResponse *response);

    grpc::Status HandleRequestAppendEntries(::grpc::ServerContext *context,
                                       const ::raftcpp::AppendEntriesRequest *request,
                                       ::raftcpp::AppendEntriesResponse *response);

    RaftState GetCurrState() {
        std::lock_guard<std::recursive_mutex> guard{mutex_};
        return curr_state_;
    }

    int64_t CurrLogIndex() const {
        std::lock_guard<std::recursive_mutex> guard{mutex_};
        if (curr_state_ == RaftState::LEADER) {
            return leader_log_manager_->CurrLogIndex();
        } else {
            return non_leader_log_manager_->CurrLogIndex();
        }
    }

private:
    void ConnectToOtherNodes();

    // void InitRpcHandlers();

    void StepBack(int64_t term_id);

    void InitTimers();

    void BecomeFollower(int64_t term, int64_t leader_id = -1);

    void BecomePreCandidate();

    void BecomeCandidate();

    void BecomeLeader();

    uint64_t GetRandomizedElectionTimeout();

    // Reset the election timer
    void RescheduleElection();

    // Asynchronous replication to a raft node
    void ReplicateOneRound(int64_t node_id);

    // With the heartbeat, the follower's log will be replicated to the same location as the leader
    void BroadcastHeartbeat();

private:
    // Current state of this node. This initial value of this should be a FOLLOWER.
    RaftState curr_state_ = RaftState::FOLLOWER;

    // The ID of this node.
    int64_t this_node_id_;

    // The ID of this current term leader node, -1 means no leader has been elected.
    int64_t leader_node_id_ = -1;

    // Current term id in this node local view.
    int64_t curr_term_;

    // CandidateId voted for in the current term, or -1 if not voted for any candidate
    int64_t vote_for_;

    // The rpc clients to all other nodes.
    std::unordered_map<int64_t, std::shared_ptr<raftrpc::Stub>> all_rpc_clients_;

    common::Config config_;

    // The recursive mutex that protects all of the node state.
    mutable std::recursive_mutex mutex_;

    // Accept the heartbeat reset election time to be random,
    // otherwise the all followers will be timed out at one time.
    Randomer randomer_;

    std::shared_ptr<StateMachine> state_machine_;

    std::shared_ptr<common::TimerManager> timer_manager_;

    // LogManager for this node.
    std::unique_ptr<LeaderLogManager> leader_log_manager_;

    std::unique_ptr<NonLeaderLogManager> non_leader_log_manager_;
};

}  // namespace node
}  // namespace raftcpp
