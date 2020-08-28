#pragma once

#include <iostream>
#include <string>

#include "rpc_client.hpp"
#include "rpc_server.h"

#include "rpc/common.h"
#include "common/type_def.h"


namespace raftcpp {
namespace node {


inline bool heartbeat(rpc_conn conn) {
    std::cout << "receive heartbeat" << std::endl;
    return true;
}

inline void ShowUsage() {
    std::cerr << "Usage: <address> <port> <role> [leader follower]" << std::endl;
}

class RaftNode {
public:
    RaftNode(const std::string &address, const int &port, const RaftState &state);

    void start();

    void Apply(raftcpp::RaftcppRequest request) {

    }

private:
    const std::string address_;
    const int port_;
    const RaftState state_;
    std::unique_ptr<rest_rpc::rpc_service::rpc_server> server;
    std::unique_ptr<rest_rpc::rpc_client> client;
};

}
}
