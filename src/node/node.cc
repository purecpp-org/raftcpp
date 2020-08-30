#include "node.h"

// TODO(qwang): Refine this as a component logging.
#include "nanolog.hpp"

namespace raftcpp::node {

RaftNode::RaftNode(const std::string &address, const int &port)
    : endpoint_(address, port) {
    // Initial logging
    nanolog::initialize(nanolog::GuaranteedLogger(), "/tmp/raftcpp", "node.log", 10);
    nanolog::set_log_level(nanolog::LogLevel::DEBUG);

    // Initial all connections and regiester fialures.
}

RaftNode::~RaftNode() {}

}  // namespace raftcpp::node
