#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "common/constants.h"
#include "common/timer.h"
#include "log_manager/blocking_queue_interface.h"
#include "log_manager/blocking_queue_mutex_impl.h"
#include "log_manager/log_entry.h"
#include "rest_rpc.hpp"
#include "rpc/common.h"

namespace raftcpp {

using AllRpcClientType =
    std::unordered_map<NodeID, std::shared_ptr<rest_rpc::rpc_client>>;

/// TODO(qwang): Should clean all inmemory data once this is Ran().
class LeaderLogManager final {
public:
    explicit LeaderLogManager(NodeID this_node_id,
                              std::function<AllRpcClientType()> get_all_rpc_clients_func);

    ~LeaderLogManager() { repeated_timer_->Stop(); }

    std::vector<LogEntry> PullLogs(const NodeID &node_id, int64_t next_log_index);

    void Push(const TermID &term_id,
              const std::shared_ptr<raftcpp::RaftcppRequest> &request);

    void Run();

    void Stop();

private:
    /// Try to commit the logs asynchronously. If a log was replied
    /// by more than one half of followers, it will be async-commit,
    /// and apply the user state machine. otherwise we don't dump it.
    void TryAsyncCommitLogs(const NodeID &node_id, size_t next_log_index,
                            std::function<void(int64_t)> committed_callback);

    void DoPushLogs();

private:
    std::mutex mutex_;

    /// The largest log index that not committed in this leader.
    int64_t curr_log_index_ = -1;

    int64_t committed_log_index_ = -1;

    std::unordered_map<int64_t, LogEntry> all_log_entries_;

    /// The map that contains the non-leader nodes to the next_log_index.
    std::unordered_map<NodeID, int64_t> next_log_indexes_;

    /// The function to get all rpc clients to followers(Including this node self).
    std::function<AllRpcClientType()> get_all_rpc_clients_func_;

    /// ID of this node.
    NodeID this_node_id_;

    boost::asio::io_service io_service_;

    std::unique_ptr<common::RepeatedTimer> repeated_timer_;

    std::atomic_bool is_running_ = false;

    /// TODO(qwang): This shouldn't be hardcode.
    constexpr static size_t NODE_NUM = 3;

    constexpr static size_t MAX_LOG_INDEX = 1000000000;
};

}  // namespace raftcpp
