#include "timer_manager.h"

#include "src/common/logging.h"

namespace raftcpp {
namespace common {

TimerManager::TimerManager() {
    io_service_ = std::make_unique<asio::io_service>();
    work_ = std::make_unique<asio::io_service::work>(*io_service_);
}

TimerManager::~TimerManager() {
    for (auto &iter : timers_) {
        iter.second->Stop();
    }

    io_service_->stop();
    thread_->join();
}

void TimerManager::Run() {
    // Note that the `Start()` should be invoked before `io_service->run()`.
    thread_ = std::make_unique<std::thread>([this]() { io_service_->run(); });
}

void TimerManager::StartTimer(const std::string &timer_key, uint64_t timeout_ms) {
    RAFTCPP_CHECK(timers_.end() != timers_.find(timer_key));
    timers_[timer_key]->Start(timeout_ms);
}

void TimerManager::ResetTimer(const std::string &timer_key, uint64_t timeout_ms) {
    RAFTCPP_CHECK(timers_.end() != timers_.find(timer_key));
    timers_[timer_key]->Reset(timeout_ms);
}

void TimerManager::StopTimer(const std::string &timer_key) {
    RAFTCPP_CHECK(timers_.end() != timers_.find(timer_key));
    timers_[timer_key]->Stop();
}

bool TimerManager::IsTimerRunning(const std::string &timer_key) const {
    RAFTCPP_CHECK(timers_.end() != timers_.find(timer_key));
    return timers_.at(timer_key)->IsRunning();
}

void TimerManager::RegisterTimer(const std::string &timer_key,
                                 const std::function<void(void)> &handler) {
    RAFTCPP_CHECK(handler != nullptr);
    RAFTCPP_CHECK(timers_.end() == timers_.find(timer_key));
    timers_.insert(std::make_pair(
        timer_key, std::make_shared<common::RepeatedTimer>(
                       *io_service_, [handler](const asio::error_code &e) {
                           if (e.value() != asio::error::operation_aborted) {
                               handler();
                           }
                       })));
}

}  // namespace common
}  // namespace raftcpp
