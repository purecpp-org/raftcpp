#pragma once

#include <asio/io_service.hpp>
#include <iostream>
#include <memory>
#include <thread>

#include "common/timer.h"

namespace raftcpp {
namespace node {

/**
 * The manager class to manage all of the timers for node.
 */
class TimerManager final {
public:
    explicit TimerManager(const std::function<void()> &election_timer_timeout_handler,
                          const std::function<void()> &heartbeat_timer_timeout_handler,
                          const std::function<void()> &vote_timer_timeout_handler);

    ~TimerManager();

    void Start();

    common::RepeatedTimer &GetElectionTimerRef() { return *election_timer_; }

    common::RepeatedTimer &GetHeartbeatTimerRef() { return *heartbeat_timer_; }

    common::RepeatedTimer &GetVoteTimerRef() { return *vote_timer_; }

private:
    // A separated service that runs for all timers.
    std::unique_ptr<asio::io_service> io_service_ = nullptr;

    // The thread that runs all timers.
    std::unique_ptr<std::thread> thread_ = nullptr;

    std::unique_ptr<common::RepeatedTimer> election_timer_ = nullptr;

    std::unique_ptr<common::RepeatedTimer> vote_timer_ = nullptr;

    std::unique_ptr<common::RepeatedTimer> heartbeat_timer_ = nullptr;
};

}  // namespace node
}  // namespace raftcpp
