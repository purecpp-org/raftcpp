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
    TimerManager() {
        io_service_ = std::make_unique<asio::io_service>();
        election_timer_ = std::make_unique<common::RandomTimer>(
            *io_service_,
            /*random_range*/ common::Range{200, 800}, [](const asio::error_code &e) {
                std::cout << "Election time's out, request election." << std::endl;
                // TODO(qwang): Election time's out, request election.
            });
        // Note that the `Start()` should be invoked before `io_service->run()`.
        election_timer_->Start();
        thread_ = std::make_unique<std::thread>([this]() { io_service_->run(); });
    }

    ~TimerManager() {
        io_service_->stop();
        thread_->join();
    }

    private:
    // A separated service that runs for all timers.
    std::unique_ptr<asio::io_service> io_service_ = nullptr;

    // The thread that runs all timers.
    std::unique_ptr<std::thread> thread_ = nullptr;

    std::unique_ptr<common::RandomTimer> election_timer_ = nullptr;
};

}  // namespace node
}  // namespace raftcpp
