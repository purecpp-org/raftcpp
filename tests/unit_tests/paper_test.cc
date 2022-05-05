#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "common/endpoint.h"
#include "node/node.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;


std::shared_ptr<Server> BuildServer(const raftcpp::Endpoint& endpoint) {
    raftcpp::node::RaftNode::Service service;
    ServerBuilder builder;
    builder.AddListeningPort(endpoint.ToString(), grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::shared_ptr<Server> server(builder.BuildAndStart());
    return server;
}
