#pragma once

#include <mutex>
#include <queue>
#include <boost/lockfree/queue.hpp>

#include "log_manager/log_manager.h"

namespace raftcpp {

template <typename LogEntryType>
class LogManagerMutexImpl : public LogManagerInterface<LogEntryType> {
    public:
    LogManagerMutexImpl() = default;

    ~LogManagerMutexImpl() = default;

    virtual LogEntryType Pop() override;

    virtual bool Pop(LogEntryType &log_entry) override;

    virtual void Push(const LogEntryType &log_entry) override;

    private:
    std::mutex queue_mutex_;

    boost::lockfree::queue<LogEntryType, boost::lockfree::fixed_sized<false>> queue_{128};
};

template <typename LogEntryType>
LogEntryType LogManagerMutexImpl<LogEntryType>::Pop() {
    LogEntryType log_entry_type;
    queue_.pop(log_entry_type);
    return log_entry_type;
}

template <typename LogEntryType>
bool LogManagerMutexImpl<LogEntryType>::Pop(LogEntryType &log_entry) {
    return queue_.pop(log_entry);
}

template <typename LogEntryType>
void LogManagerMutexImpl<LogEntryType>::Push(const LogEntryType &log_entry) {
	queue_.push(log_entry);
}

}  // namespace raftcpp
