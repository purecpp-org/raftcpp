#pragma once

#include <mutex>
#include <queue>

#include "log_manager/log_manager.h"

namespace raftcpp {

template <typename LogEntryType>
class LogManagerMutexImpl : public LogManagerInterface<LogEntryType> {
    public:
    LogManagerMutexImpl() = default;

    ~LogManagerMutexImpl() = default;

    LogEntryType Pop() override {
        std::lock_guard<std::mutex> guard{queue_mutex_};
        LogEntryType ret;
        ret = queue_.front();
        return ret;
    }

    void Push(const LogEntryType &log_entry) override {
        std::lock_guard<std::mutex> guard{queue_mutex_};
        queue_.push(log_entry);
    }

    private:
    std::mutex queue_mutex_;

    std::queue<LogEntryType> queue_;
};

}  // namespace raftcpp
