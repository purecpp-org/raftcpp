#pragma once

#include <asio/io_service.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "common/randomer.h"
#include "common/timer.h"

namespace raftcpp {
namespace common {

/**
 * The manager class to manage all of the timers for node.
 */
class TimerManager final {
public:
    explicit TimerManager();

    ~TimerManager();

    // run the manager, as start run timers that be registered
    void Run();

    // register timer by timer handler func, and return timer id that be assign
    int RegisterTimer(const std::function<void(void)> &handler);

    // enclosure timer's operation by timer id
    void StartTimer(int id, uint64_t timeout_ms);
    void ResetTimer(int id, uint64_t timeout_ms);
    void StopTimer(int id);

private:
    // A separated service that runs for all timers.
    std::unique_ptr<asio::io_service> io_service_ = nullptr;

    std::unique_ptr<asio::io_service::work> work_ = nullptr;

    // The thread that runs all timers.
    std::unique_ptr<std::thread> thread_ = nullptr;

    std::vector<std::shared_ptr<common::RepeatedTimer>> timers_;
};

}  // namespace common
}  // namespace raftcpp
