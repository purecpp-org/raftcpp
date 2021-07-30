#include "timer_manager.h"

#include "common/util.h"

namespace raftcpp {
namespace common {

TimerManager::TimerManager() {
    io_service_ = std::make_unique<asio::io_service>();
    work_ = std::make_unique<asio::io_service::work>(*io_service_);
}

TimerManager::~TimerManager() {
    for (auto &timer : timers_) {
        timer->Stop();
    }

    io_service_->stop();
    thread_->join();
}

void TimerManager::Run() {
    // Note that the `Start()` should be invoked before `io_service->run()`.
    thread_ = std::make_unique<std::thread>([this]() { io_service_->run(); });
}

void TimerManager::StartTimer(int id, uint64_t timeout_ms) {
    int size = (int)timers_.size();
    if (id >= 0 && id < size) {
        timers_[id]->Start(timeout_ms);
    }
}

void TimerManager::ResetTimer(int id, uint64_t timeout_ms) {
    int size = (int)timers_.size();
    if (id >= 0 && id < size) {
        timers_[id]->Reset(timeout_ms);
    }
}

void TimerManager::StopTimer(int id) {
    int size = (int)timers_.size();
    if (id >= 0 && id < size) {
        timers_[id]->Stop();
    }
}
int TimerManager::RegisterTimer(const std::function<void(void)> &handler) {
    if (handler == nullptr) {
        return -1;
    }

    timers_.emplace_back(std::make_shared<common::RepeatedTimer>(
        *io_service_, [handler](const asio::error_code &e) {
            if (e.value() != asio::error::operation_aborted) {
                handler();
            }
        }));

    return (int)timers_.size() - 1;
}

}  // namespace common
}  // namespace raftcpp
