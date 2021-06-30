#pragma once

#include <asio/io_service.hpp>
#include <asio/steady_timer.hpp>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <shared_mutex>

#include "common/range.h"
#include "common/util.h"

namespace raftcpp {

namespace common {

/**
 * A class that encapsulates a boost timer as a repeated timer with a fixed
 * expired time. This class is thread safe to use.
 *
 * Note that the io_service must be ran after we create this timer, otherwise
 * it may cause that the io_service has no event and it will exit at once.
 */
class RepeatedTimer final {
public:
    RepeatedTimer(asio::io_service &io_service,
                  std::function<void(const asio::error_code &e)> timeout_handler)
        : io_service_(io_service),
          timer_(io_service_),
          timeout_handler_(std::move(timeout_handler)) {}

    void Start(uint64_t timeout_ms);

    void Stop();

    void Reset(uint64_t timeout_ms);

private:
    void DoSetExpired(uint64_t timeout_ms);

private:
    // The io service that runs this timer.
    asio::io_service &io_service_;

    // The actual boost timer.
    asio::steady_timer timer_;

    std::atomic<bool> is_running_{false};

    // The handler that will be triggered once the time's up.
    std::function<void(const asio::error_code &e)> timeout_handler_;
};

class ContinuousTimer final {
public:
    explicit ContinuousTimer(asio::io_service &ios, size_t timeout_ms,
                             std::function<void(const asio::error_code &e)> handler)
        : timer_(ios), timeout_ms_(timeout_ms), timeout_handler_(std::move(handler)) {}

    void Start() { RunTimer(); }

    void Cancel() {
        if (timeout_ms_ == 0) {
            return;
        }

        asio::error_code ec;
        timer_.cancel(ec);
        timeout_ms_ = 0;
    }

private:
    void RunTimer() {
        if (timeout_ms_ == 0) {
            return;
        }

        timer_.expires_from_now(std::chrono::milliseconds(timeout_ms_));
        timer_.async_wait([this](const asio::error_code &e) {
            timeout_handler_(e);
            RunTimer();
        });
    }

private:
    // The actual boost timer.
    asio::steady_timer timer_;

    // The timeout time: ms.
    size_t timeout_ms_;

    // The handler that will be triggered once the time's up.
    std::function<void(const asio::error_code &e)> timeout_handler_;
};

}  // namespace common
}  // namespace raftcpp
