#include "common/timer.h"

namespace raftcpp {
namespace common {

void RandomTimer::ResetForTimer() {
    // TODO(qwang): Use random library in modern standard library.
    const uint64_t timeout_in_ms =
        (rand() % random_range_.GetDelta()) + random_range_.GetBegin();
    std::cout << "timeout_in_ms = " << timeout_in_ms << std::endl;
    timer_.expires_from_now(std::chrono::milliseconds(timeout_in_ms));
    timer_.async_wait([this](const asio::error_code &e) {
        timeout_handler_(e);
        this->ResetForTimer();
    });
}

void RepeatedTimer::Reset(const uint64_t timeout_ms) {
    timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
    timer_.async_wait([this, timeout_ms](const asio::error_code &e) {
        timeout_handler_(e);
        this->Reset(timeout_ms);
    });
}

void RepeatedTimer::PostStop() {
    // Aay thread safe issue?
    io_service_.post([this]() {
        timer_.cancel();
    });
}

void RepeatedTimer::Stop() { timer_.cancel(); }

}  // namespace common
}  // namespace raftcpp
