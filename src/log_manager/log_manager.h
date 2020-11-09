#pragma once

namespace raftcpp {

/**
 * The interface that represents a log manager.
 */
template <typename LogEntryType>
class LogManagerInterface {
public:
    virtual ~LogManagerInterface() {}

    /**
     * Pop the front element from log manager. Note that
     * this will be blocked if there is no log in the queue.
     */
    virtual LogEntryType Pop() = 0;

    /**
     * Pop the front element from log manager, if the queue is
     * empty, it will return false.
     */
    virtual bool Pop(LogEntryType &log_entry) = 0;

    /**
     * Push the log to this log manager.
     */
    virtual void Push(const LogEntryType &log_entry) = 0;
};

}  // namespace raftcpp
