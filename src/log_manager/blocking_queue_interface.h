#pragma once
#include <vector>

namespace raftcpp {

/**
 * The interface that represents a blocking queue.
 */
template <typename LogEntryType>
class BlockingQueueInterface {
public:
    virtual ~BlockingQueueInterface() {}

    /**
     * Pop the front element from blocking queue. Note that
     * this will be blocked if there is no log in the queue.
     */
    virtual LogEntryType Pop() = 0;

    /**
     * Pop the front element from blocking queue, if the queue is
     * empty, it will return false.
     */
    virtual bool Pop(LogEntryType &log_entry) = 0;

    /**
     * Push the log to this log manager.
     */
    virtual void Push(const LogEntryType &log_entry) = 0;

    /**
     * Get the most front items from the queue.
     */
    virtual std::vector<LogEntryType> MostFront(int mostFrontNumber) = 0;
};

}  // namespace raftcpp
