#include "common/timer.h"

#include <atomic>

namespace raftcpp {
namespace common {

void RepeatedTimer::Start(const uint64_t timeout_ms) { Reset(timeout_ms); }

void RepeatedTimer::Stop() {
    is_running_.store(false);
}

void RepeatedTimer::Reset(const uint64_t timeout_ms) {
    is_running_.store(true);
    DoSetExpired(timeout_ms);
}

void RepeatedTimer::DoSetExpired(const uint64_t timeout_ms) {
    if (!is_running_.load()) {
        return;
    }

    timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
    timer_.async_wait([this, timeout_ms](const asio::error_code &e) {
        if (e.value() == asio::error::operation_aborted || !is_running_.load()) {
            // The timer was canceled.
            return;
        }
        timeout_handler_(e);
        this->DoSetExpired(timeout_ms);
    });
}

}  // namespace common
}  // namespace raftcpp
