#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/config.h"
#include "common/endpoint.h"
#include "common/id.h"
#include "common/timer.h"
#include "common/type_def.h"
#include "node/timer_manager.h"
#include "rpc/common.h"
#include "rpc/services.h"
#include "rpc_client.hpp"
#include "rpc_server.h"

namespace raftcpp {
namespace node {

class RaftNode : public rpc::NodeService {
public:
    RaftNode(rest_rpc::rpc_service::rpc_server &rpc_server, const common::Config &config);

    ~RaftNode();

    void Apply(raftcpp::RaftcppRequest request) {}

    void RequestVote(rpc::RpcConn conn, const std::string &endpoint_str) override;

private:
    void ConnectToOtherNodes() {}

private:
    TimerManager timer_manager_;

    // Current state of this node. This initial value of this should be a FOLLOWER.
    RaftState curr_state_ = RaftState::FOLLOWER;

    // Current term id in this node local view.
    TermID curr_term_id_;

    // The rpc server on this node to be connected from all other node in this raft group.
    rest_rpc::rpc_service::rpc_server &rpc_server_;

    // The rpc clients to all other nodes.
    // NodeID instead
    //    std::unordered_map<int, std::shared_ptr<rest_rpc::rpc_client>> rpc_clients_;
    std::vector<std::shared_ptr<rest_rpc::rpc_client>> rpc_clients_;

    common::Config config_;
};

}  // namespace node
}  // namespace raftcpp
