#include "common/timer.h"

#include <atomic>

namespace raftcpp {
namespace common {

void RepeatedTimer::Start(const uint64_t timeout_ms) { Reset(timeout_ms); }

void RepeatedTimer::Stop() {
    std::lock_guard<std::shared_mutex> guard{shared_mutex_};
    is_running_ = false;
}

void RepeatedTimer::Reset(const uint64_t timeout_ms) {
    {
        std::lock_guard<std::shared_mutex> guard{shared_mutex_};
        is_running_ = true;
    }
    DoSetExpired(timeout_ms);
}

void RepeatedTimer::DoSetExpired(const uint64_t timeout_ms) {
    std::shared_lock<std::shared_mutex> guard{shared_mutex_};
    if (!is_running_) {
        return;
    }

    timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
    timer_.async_wait([this, timeout_ms](const asio::error_code &e) {
        {
            if (e.value() == asio::error::operation_aborted) {
                // The timer was canceled.
                return;
            }

            std::shared_lock<std::shared_mutex> guard{shared_mutex_};
            if (!is_running_) {
                // The timer is stopping, should we should trigger any handler.
                return;
            }
        }
        timeout_handler_(e);
        this->DoSetExpired(timeout_ms);
    });
}

}  // namespace common
}  // namespace raftcpp
