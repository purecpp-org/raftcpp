#include "node.h"

// TODO(qwang): Refine this as a component logging.
#include "nanolog.hpp"

namespace raftcpp::node {

RaftNode::RaftNode(const std::string &address, const int &port)
    : endpoint_(address, port),
      timers_io_service_(),
      // TODO(qwang): This should be random value.
      election_timer_(timers_io_service_, boost::asio::chrono::milliseconds(2000)) {
    election_timer_.async_wait(
        [this](const boost::system::error_code &e) { HandleElectionTimer(); });

    timers_thread_ =
        std::make_unique<std::thread>([this]() { timers_io_service_.run(); });

    // Initial logging
    nanolog::initialize(nanolog::GuaranteedLogger(), "/tmp/raftcpp", "node.log", 10);
    nanolog::set_log_level(nanolog::LogLevel::DEBUG);

    // Initial all connections and regiester fialures.
}

RaftNode::~RaftNode() { timers_thread_->join(); }

void RaftNode::HandleElectionTimer() {
    std::cout << "hellllooo" << std::endl;
    LOG_DEBUG << "Election timer timeout.";
    LOG_INFO << "Election timer timeout.";

    election_timer_.expires_from_now(boost::asio::chrono::milliseconds(2000));
    election_timer_.async_wait(
        [this](const boost::system::error_code &e) { this->HandleElectionTimer(); });
}

}  // namespace raftcpp::node
