#pragma once

#include <grpc/impl/codegen/connectivity_state.h>
#include <grpcpp/channel.h>
#include <grpcpp/grpcpp.h>
#include <sys/_types/_int32_t.h>

#include <asio/error_code.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common/constants.h"
#include "common/endpoint.h"
#include "common/id.h"
#include "raft.grpc.pb.h"

namespace raftcpp {

class raftclient {
public:
    explicit raftclient(std::shared_ptr<grpc::Channel> channel)
        : stub_(raftrpc::NewStub(channel)) {}

    std::string request_pre_vote(const std::string &endpoint, const term_t &term_id);

    std::string request_vote(const std::string &endpoint, const term_t &term_id);

    // void request_push_logs(std::function<void(asio::error_code, std::string_view)>
    // callback, const std::string& endpoint, const term_t& term_id);

    int32_t heartbeat(const term_t &term_id, const std::string &node_id);

private:
    std::unique_ptr<raftrpc::Stub> stub_;
};

}  // namespace raftcpp