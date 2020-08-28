#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "common/endpoint.h"
#include "common/id.h"
#include "common/type_def.h"
#include "rpc/common.h"
#include "rpc_client.hpp"
#include "rpc_server.h"

namespace raftcpp {
namespace node {

/**
 * 这里需要注意的是， 我们的append_logs到底是pull还是push， 如果是pull的话，
 * 那么是否和leader发心跳给其他节点相冲突？ 难道心跳也是由follower发给leader吗？
 * 或者是follower发lease给leader。
 *
 * @param conn
 * @return
 */
inline bool heartbeat(rpc_conn conn) {
    std::cout << "receive heartbeat" << std::endl;
    return true;
}

inline void ShowUsage() {
    std::cerr << "Usage: <address> <port> <role> [leader follower]" << std::endl;
}

class RaftNode {
    public:
    RaftNode(const std::string &address, const int &port);

    void start();

    void Apply(raftcpp::RaftcppRequest request) {}

    private:
    // TODO(qwang): Should this be io context?
    // The io service for timers.
    asio::io_service io_service_;

    // The timer that trigger a election request for this node.
    asio::steady_timer election_timer_;

    // The endpoint that this node listening on.
    const Endpoint endpoint_;

    // Current state of this node. This initial value of this should be a FOLLOWER.
    RaftState curr_state_ = RaftState::FOLLOWER;

    // Current term id in this node local view.
    TermID curr_term_id_;

    // The rpc server on this node to be connected from all other node in this raft group.
    std::unique_ptr<rest_rpc::rpc_service::rpc_server> rpc_server_;

    // The rpc clients to all other nodes.
    std::unordered_map<NodeID, std::shared_ptr<rest_rpc::rpc_client>> rpc_clients_;
};

}  // namespace node
}  // namespace raftcpp
