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
    virtual void OnRequestPreVote(RpcConn conn, const std::string &endpoint_str,
                                  int32_t termid) = 0;

    virtual void OnRequestVote(RpcConn conn, const std::string &endpoint_str,
                               int32_t termid) = 0;

    virtual void OnRequestHeartbeat(RpcConn conn, int32_t termid) = 0;
};

}  // namespace rpc
}  // namespace raftcpp
