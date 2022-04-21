#pragma once

#include <grpcpp/client_context.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "protos/raft.grpc.pb.h"
#include "protos/raft.pb.h"
#include "src/common/config.h"
#include "src/common/endpoint.h"
#include "src/common/id.h"
#include "src/common/logging.h"
#include "src/common/timer.h"
#include "src/common/timer_manager.h"
#include "src/common/type_def.h"
#include "src/log_manager/blocking_queue_interface.h"
#include "src/log_manager/blocking_queue_mutex_impl.h"
#include "src/log_manager/leader_log_manager.h"
#include "src/log_manager/non_leader_log_manager.h"
#include "src/statemachine/state_machine_impl.h"

namespace raftcpp {
namespace node {

class RaftNode : public raftrpc::Service, public std::enable_shared_from_this<RaftNode> {
public:
    RaftNode(
        std::shared_ptr<RaftStateMachine> state_machine,
        // std::unique_ptr<grpc::Server> rpc_server,
        const common::Config &config,
        const raftcpp::RaftcppLogLevel severity = raftcpp::RaftcppLogLevel::RLL_DEBUG);

    // explicit RaftNode() = default;

    ~RaftNode();

    void Init();

    bool IsLeader() const;

    void PushRequest(const std::shared_ptr<PushLogsRequest> &request);

    void RequestPreVote();

    grpc::Status HandleRequestPreVote(::grpc::ServerContext *context,
                                      const ::raftcpp::PreVoteRequest *request,
                                      ::raftcpp::PreVoteResponse *response);

    void OnPreVote(const asio::error_code &ec, std::string_view data);

    void RequestVote();

    grpc::Status HandleRequestVote(::grpc::ServerContext *context,
                                   const ::raftcpp::VoteRequest *request,
                                   ::raftcpp::VoteResponse *response);

    void OnVote(const asio::error_code &ec, std::string_view data);

    grpc::Status HandleRequestHeartbeat(::grpc::ServerContext *context,
                                        const ::raftcpp::HeartbeatRequest *request,
                                        ::raftcpp::HeartbeatResponse *response);

    grpc::Status HandleRequestPushLogs(::grpc::ServerContext *context,
                                       const ::raftcpp::PushLogsRequest *request,
                                       ::google::protobuf::Empty *response);

    void RequestHeartbeat();

    void OnHeartbeat(const asio::error_code &ec, std::string_view data);

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

    void StepBack(int32_t term_id);

    void InitTimers();

private:
    // Current state of this node. This initial value of this should be a FOLLOWER.
    RaftState curr_state_ = RaftState::FOLLOWER;

    // Current term id in this node local view.
    TermID curr_term_id_;

    // The rpc server on this node to be connected from all other node in this raft group.
    // std::unique_ptr<grpc::Server> rpc_server_;

    // The rpc clients to all other nodes.
    std::unordered_map<NodeID, std::shared_ptr<raftrpc::Stub>> all_rpc_clients_;

    common::Config config_;

    // This set is used to cache the endpoints of the nodes which is responded for the pre
    // vote request.
    std::unordered_set<std::string> responded_pre_vote_nodes_;

    // This set is used to cache the endpoints of the nodes which is reponded for the vote
    // request.
    std::unordered_set<std::string> responded_vote_nodes_;

    // The recursive mutex that protects all of the node state.
    mutable std::recursive_mutex mutex_;

    // Accept the heartbeat reset election time to be random,
    // otherwise the all followers will be timed out at one time.
    Randomer randomer_;

    // The ID of this node.
    NodeID this_node_id_;

    std::shared_ptr<StateMachine> state_machine_;

    std::unique_ptr<NodeID> leader_node_id_ = nullptr;

    std::shared_ptr<common::TimerManager> timer_manager_;

    // LogManager for this node.
    std::unique_ptr<LeaderLogManager> leader_log_manager_;

    std::unique_ptr<NonLeaderLogManager> non_leader_log_manager_;
};

}  // namespace node
}  // namespace raftcpp
