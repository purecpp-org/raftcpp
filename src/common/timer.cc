#include "common/timer.h"

namespace raftcpp {
namespace common {

void RepeatedTimer::Reset(const uint64_t timeout_ms) {
    std::lock_guard<std::shared_mutex> guard{shared_mutex_};
    is_running_ = true;
    timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
    timer_.async_wait([this, timeout_ms](const asio::error_code &e) {
        {
            std::shared_lock<std::shared_mutex> guard{shared_mutex_};
            if (!is_running_) {
                // The timer is stopping, should we should trigger any handler.
                return;
            }
        }
        timeout_handler_(e);
        this->Reset(timeout_ms);
    });
}

void RepeatedTimer::Stop() {
    std::lock_guard<std::shared_mutex> guard{shared_mutex_};
    is_running_ = false;
}

}  // namespace common
}  // namespace raftcpp
