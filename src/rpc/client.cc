
#include "client.h"

#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <raft.pb.h>
#include <sys/_types/_int32_t.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <utility>

#include "common/logging.h"

namespace raftcpp {

std::string raftclient::request_pre_vote(const std::string &endpoint,
                                         const term_t &term_id) {
    PreVoteRequest request;
    request.set_endpoint(endpoint);
    request.set_termid(term_id);

    PreVoteResponse resp;
    grpc::ClientContext ccxt;

    grpc::Status s = stub_->HandleRequestPreVote(&ccxt, request, &resp);
    if (s.ok()) {
        return resp.endpoint();
    } else {
        RAFTCPP_LOG(RLL_ERROR) << s.error_code() << ": " << s.error_message();
        return "";
    }
}

std::string raftclient::request_vote(const std::string &endpoint, const term_t &term_id) {
    VoteRequest request;
    request.set_endpoint(endpoint);
    request.set_termid(term_id);

    VoteResponse resp;
    grpc::ClientContext ccxt;
    grpc::Status s = stub_->HandleRequestVote(&ccxt, request, &resp);
    if (s.ok()) {
        return resp.endpoint();
    } else {
        RAFTCPP_LOG(RLL_ERROR) << s.error_code() << ": " << s.error_message();
        return "";
    }
}

// void raftclient::request_push_logs(
//     std::function<void(asio::error_code, std::string_view)> callback,
//     const std::string &endpoint, const term_t &term_id) {

// }

int32_t raftclient::heartbeat(const term_t &term_id, const std::string &node_id) {
    HeartbeatRequest request;
    request.set_node_id(node_id);
    request.set_termid(term_id);

    HeartbeatResponse resp;
    grpc::ClientContext ccxt;
    grpc::Status s = stub_->HandleRequestHeartbeat(&ccxt, request, &resp);
    if (s.ok()) {
        return resp.termid();
    } else {
        RAFTCPP_LOG(RLL_ERROR) << s.error_code() << ": " << s.error_message();
        return -1;
    }
}

}  // namespace raftcpp