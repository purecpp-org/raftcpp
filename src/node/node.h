#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/config.h"
#include "common/endpoint.h"
#include "common/id.h"
#include "common/logging.h"
#include "common/timer.h"
#include "common/timer_manager.h"
#include "common/type_def.h"
#include "log_manager/blocking_queue_interface.h"
#include "log_manager/blocking_queue_mutex_impl.h"
#include "log_manager/leader_log_manager.h"
#include "log_manager/log_entry.h"
#include "log_manager/non_leader_log_manager.h"
#include "rest_rpc/rpc_client.hpp"
#include "rest_rpc/rpc_server.h"
#include "rpc/common.h"
#include "rpc/services.h"
#include "statemachine/state_machine.h"

namespace raftcpp {
namespace node {

class RaftNode : public rpc::NodeService, public std::enable_shared_from_this<RaftNode> {
public:
    RaftNode(
        std::shared_ptr<StateMachine> state_machine,
        rest_rpc::rpc_service::rpc_server &rpc_server, const common::Config &config,
        const raftcpp::RaftcppLogLevel severity = raftcpp::RaftcppLogLevel::RLL_DEBUG);

    ~RaftNode();

    void Init();

    bool IsLeader() const;

    void PushRequest(const std::shared_ptr<raftcpp::RaftcppRequest> &request);

    void RequestPreVote();

    void HandleRequestPreVote(rpc::RpcConn conn, const std::string &endpoint_str,
                              int32_t term_id) override;

    void OnPreVote(const boost::system::error_code &ec, string_view data);

    void RequestVote();

    void HandleRequestVote(rpc::RpcConn conn, const std::string &endpoint_str,
                           int32_t term_id) override;

    void OnVote(const boost::system::error_code &ec, string_view data);

    void HandleRequestHeartbeat(rpc::RpcConn conn, int32_t term_id,
                                std::string node_id_binary) override;

    void HandleRequestPullLogs(rpc::RpcConn conn, std::string node_id_binary,
                               int64_t committed_log_index) override;

    void HandleRequestPushLogs(rpc::RpcConn conn, int64_t committed_log_index,
                               int32_t pre_log_term, LogEntry log_entry) override;

    void RequestHeartbeat();

    void OnHeartbeat(const boost::system::error_code &ec, string_view data);

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

    void InitRpcHandlers();

    void StepBack(int32_t term_id);

    void InitTimers();

private:
    // Current state of this node. This initial value of this should be a FOLLOWER.
    RaftState curr_state_ = RaftState::FOLLOWER;

    // Current term id in this node local view.
    TermID curr_term_id_;

    // The rpc server on this node to be connected from all other node in this raft group.
    rest_rpc::rpc_service::rpc_server &rpc_server_;

    // The rpc clients to all other nodes.
    std::unordered_map<NodeID, std::shared_ptr<rest_rpc::rpc_client>> all_rpc_clients_;

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
