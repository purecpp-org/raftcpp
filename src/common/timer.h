#pragma once

#include <asio/io_service.hpp>
#include <asio/steady_timer.hpp>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>

#include "common/range.h"
#include "common/util.h"

namespace raftcpp {

namespace common {

/**
 * A class that encapsulates a boost timer with random expired time.
 *
 * Note that the io_service must be ran after we create this timer, otherwise
 * it may cause that the io_service has no event and it will exit at once.
 */
class RandomTimer final {
public:
    RandomTimer(asio::io_service &io_service, const Range &random_range,
                std::function<void(const asio::error_code &e)> timeout_handler)
        : io_service_(io_service),
          timer_(io_service_),
          random_range_(random_range),
          timeout_handler_(std::move(timeout_handler)) {
        srand(CurrentTimeMs());
    }

    void Start() { ResetForTimer(); }

private:
    // TODO(qwang): This method should renamed a meaningful one.
    void ResetForTimer();

private:
    // The io service that runs this timer.
    asio::io_service &io_service_;

    // The actual boost timer.
    asio::steady_timer timer_;

    // The range that represents for a scope of the random range.
    Range random_range_;

    // The handler that will be triggered once the time's out.
    std::function<void(const asio::error_code &e)> timeout_handler_;
};

class RepeatedTimer final {
public:
    RepeatedTimer(asio::io_service &io_service,
                  std::function<void(const asio::error_code &e)> timeout_handler)
    : io_service_(io_service),
    timer_(io_service_),
    timeout_handler_(std::move(timeout_handler)) {}

    void Start(const uint64_t timeout_ms) { Reset(timeout_ms); }

    void Reset(const uint64_t timeout_ms);

private:
    // The io service that runs this timer.
    asio::io_service &io_service_;

    // The actual boost timer.
    asio::steady_timer timer_;

    // The handler that will be triggered once the time's up.
    std::function<void(const asio::error_code &e)> timeout_handler_;
};

}  // namespace common
}  // namespace raftcpp
