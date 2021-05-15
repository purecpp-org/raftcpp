#pragma once

#include "rest_rpc/rpc_server.h"

namespace raftcpp {
namespace rpc {

using RpcConn = rest_rpc::rpc_service::rpc_conn;

/**
 * The rpc service that communicates with other nodes.
 */
class NodeService {
public:
    virtual ~NodeService() = default;

protected:
    virtual void HandleRequestPreVote(RpcConn conn, const std::string &endpoint_str,
                                      int32_t termid) = 0;

    virtual void HandleRequestVote(RpcConn conn, const std::string &endpoint_str,
                                   int32_t termid) = 0;

    virtual void HandleRequestHeartbeat(RpcConn conn, int32_t termid) = 0;
};

}  // namespace rpc
}  // namespace raftcpp
