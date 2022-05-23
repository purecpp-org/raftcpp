#include "util.h"

#include <grpcpp/grpcpp.h>

#include <string>
#include <vector>

#include "src/node/node.h"

std::string init_config(std::string address, int basePort, int nodeNum, int thisNode) {
    std::vector<std::string> addr;
    addr.push_back(address + ":" + std::to_string(basePort + thisNode));

    for (int i = 0; i < nodeNum; i++) {
        if (i == thisNode) {
            continue;
        }
        addr.push_back(address + ":" + std::to_string(basePort + i));
    }

    std::string config;
    for (int i = 0; i < nodeNum; i++) {
        config += addr[i];
        if (i < nodeNum - 1) {
            config += ",";
        }
    }

    return config;
}

std::unique_ptr<grpc::Server> BuildServer(
    const raftcpp::Endpoint &endpoint, std::shared_ptr<raftcpp::node::RaftNode> service) {
    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(endpoint.ToString(), grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case, it corresponds to an *synchronous* service.
    builder.RegisterService(service.get());
    auto server_ = builder.BuildAndStart();
    return server_;
}