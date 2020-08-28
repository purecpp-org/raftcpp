#include "node.h"

namespace raftcpp::node {

RaftNode::RaftNode(const std::string &address, const int &port)
    : endpoint_(address, port),
      io_service_(),
      // TODO(qwang): This should be random value.
      election_timer_(io_service_, boost::asio::chrono::milliseconds(200)) {
    election_timer_.async_wait([]() {
        // TODO(qwang): Time's up, request a new election.
    });

    // Initial all connections and regiester fialures.
}

void RaftNode::start(){};

}  // namespace raftcpp::node
