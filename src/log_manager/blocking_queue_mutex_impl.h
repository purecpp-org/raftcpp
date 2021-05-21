#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "log_manager/blocking_queue_interface.h"

namespace raftcpp {

template <typename LogEntryType>
class BlockingQueueMutexImpl : public BlockingQueueInterface<LogEntryType> {
public:
    BlockingQueueMutexImpl() = default;

    ~BlockingQueueMutexImpl() = default;

    virtual LogEntryType Pop() override;

    virtual bool Pop(LogEntryType &log_entry) override;

    virtual void Push(const LogEntryType &log_entry) override;

private:
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<LogEntryType> queue_;
};

template <typename LogEntryType>
LogEntryType BlockingQueueMutexImpl<LogEntryType>::Pop() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_cv_.wait(lock, [this] { return !queue_.empty(); });
    LogEntryType log_entry_type = queue_.front();
    queue_.pop();
    return log_entry_type;
}

template <typename LogEntryType>
bool BlockingQueueMutexImpl<LogEntryType>::Pop(LogEntryType &log_entry) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (!queue_.empty()) {
        log_entry = queue_.front();
        queue_.pop();
        return true;
    }
    return false;
}

template <typename LogEntryType>
void BlockingQueueMutexImpl<LogEntryType>::Push(const LogEntryType &log_entry) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_.push(log_entry);
    queue_cv_.notify_all();
}

}  // namespace raftcpp
