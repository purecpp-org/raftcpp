#include <grpcpp/grpcpp.h>

#include <string>
#include <vector>

#include "src/node/node.h"

std::string init_config(std::string address, int basePort, int nodeNum, int thisNode);
std::unique_ptr<grpc::Server> BuildServer(
    const raftcpp::Endpoint &endpoint, std::shared_ptr<raftcpp::node::RaftNode> service);