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
#include "common/type_def.h"
#include "node/timer_manager.h"
#include "rest_rpc/rpc_client.hpp"
#include "rest_rpc/rpc_server.h"
#include "rpc/common.h"
#include "rpc/services.h"

namespace raftcpp {
namespace node {

class RaftNode : public rpc::NodeService {
public:
    RaftNode(
        rest_rpc::rpc_service::rpc_server &rpc_server, const common::Config &config,
        const raftcpp::RaftcppLogLevel severity = raftcpp::RaftcppLogLevel::RLL_DEBUG);

    ~RaftNode();

    void Apply(std::shared_ptr<raftcpp::RaftcppRequest> request);

    void RequestPreVote();

    void HandleRequestPreVote(rpc::RpcConn conn, const std::string &endpoint_str,
                              int32_t term_id) override;

    void OnPreVote(const boost::system::error_code &ec, string_view data);

    void RequestVote();

    void HandleRequestVote(rpc::RpcConn conn, const std::string &endpoint_str,
                           int32_t term_id) override;

    void OnVote(const boost::system::error_code &ec, string_view data);

    void HandleRequestHeartbeat(rpc::RpcConn conn, int32_t term_id) override;

    void RequestHeartbeat();

    void OnHeartbeat(const boost::system::error_code &ec, string_view data);

    RaftState GetCurrState() const { return curr_state_; }

private:
    void ConnectToOtherNodes();

    void InitRpcHandlers();

    void StepBack(int32_t term_id);

private:
    TimerManager timer_manager_;

    // Current state of this node. This initial value of this should be a FOLLOWER.
    RaftState curr_state_ = RaftState::FOLLOWER;

    // Current term id in this node local view.
    TermID curr_term_id_;

    // The rpc server on this node to be connected from all other node in this raft group.
    rest_rpc::rpc_service::rpc_server &rpc_server_;

    // The rpc clients to all other nodes.
    std::unordered_map<std::string, std::shared_ptr<rest_rpc::rpc_client>> rpc_clients_;

    common::Config config_;

    // This set is used to cache the endpoints of the nodes which is responded for the pre
    // vote request.
    std::unordered_set<std::string> responded_pre_vote_nodes_;

    // This set is used to cache the endpoints of the nodes which is reponded for the vote
    // request.
    std::unordered_set<std::string> responded_vote_nodes_;

    // The recursive mutex that protects all of the node state.
    std::recursive_mutex mutex_;

    // Accept the heartbeat reset election time to be random,
    // otherwise the all followers will be timed out at one time.
    Randomer randomer_;

    // The ID of this node.
    NodeID this_node_id_;
};

}  // namespace node
}  // namespace raftcpp
