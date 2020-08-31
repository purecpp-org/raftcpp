#pragma once

#include <rpc_server.h>

namespace raftcpp {
namespace rpc {

using RpcConn = rest_rpc::rpc_service::rpc_conn;

/**
 * The rpc service that communicates with other nodes.
 */
class NodeService {
protected:
    virtual void RequestVote(RpcConn conn, const std::string &node_id_binary) = 0;
};

}  // namespace rpc
}  // namespace raftcpp
