#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>

#include "blocking_queue_interface.h"
#include "blocking_queue_mutex_impl.h"
#include "external/asio/_virtual_includes/asio/asio/io_service.hpp"
#include "proto/raft.grpc.pb.h"
#include "proto/raft.pb.h"
#include "src/common/constants.h"
#include "src/common/id.h"
#include "src/common/timer.h"
#include "src/common/timer_manager.h"

namespace raftcpp {

using AllRpcClientType = std::unordered_map<NodeID, std::shared_ptr<raftrpc::Stub>>;

/// TODO(qwang): Should clean all inmemory data once this is Ran().
class LeaderLogManager final {
public:
    explicit LeaderLogManager(NodeID this_node_id,
                              std::function<AllRpcClientType()> get_all_rpc_clients_func,
                              const std::shared_ptr<common::TimerManager> &timer_manager);

    ~LeaderLogManager() { timer_manager_->StopTimer(RaftcppConstants::TIMER_PUSH_LOGS); }

    void Push(const TermID &term_id, const std::shared_ptr<PushLogsRequest> &request);

    void Run(std::unordered_map<int64_t, LogEntry> &logs, int64_t committedIndex);

    void Stop();

    int64_t CurrLogIndex() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return curr_log_index_;
    }

    // attention to all_log_entries_ may be large, so as far as possible no copy
    std::unordered_map<int64_t, LogEntry> &Logs() {
        std::lock_guard<std::mutex> lock(mutex_);
        return all_log_entries_;
    }

    int64_t CommittedLogIndex() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return committed_log_index_;
    }

private:
    /// Try to commit the logs asynchronously. If a log was replied
    /// by more than one half of followers, it will be async-commit,
    /// and apply the user state machine. otherwise we don't dump it.
    void TryAsyncCommitLogs(const NodeID &node_id, size_t next_log_index,
                            std::function<void(int64_t)> committed_callback);

    void DoPushLogs();

private:
    mutable std::mutex mutex_;

    /// The largest log index that not committed in this leader.
    int64_t curr_log_index_ = -1;

    int64_t committed_log_index_ = -1;

    std::unordered_map<int64_t, LogEntry> all_log_entries_;

    /// The map that contains the non-leader nodes to the next_log_index.
    std::unordered_map<NodeID, int64_t> next_log_indexes_;

    /// The map that contains the non-leader nodes to follower already has max log index.
    std::unordered_map<NodeID, int64_t> match_log_indexes_;

    /// The function to get all rpc clients to followers(Including this node self).
    std::function<AllRpcClientType()> get_all_rpc_clients_func_;

    /// ID of this node.
    NodeID this_node_id_;

    asio::io_service io_service_;

    /// TODO(qwang): This shouldn't be hardcode.
    constexpr static size_t NODE_NUM = 3;

    constexpr static size_t MAX_LOG_INDEX = 1000000000;

    std::shared_ptr<common::TimerManager> timer_manager_;
};

}  // namespace raftcpp
