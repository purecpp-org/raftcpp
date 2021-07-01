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
                              std::function<AllRpcClientType()> get_all_rpc_clients_func)
        : io_service_(),
          this_node_id_(std::move(this_node_id)),
          get_all_rpc_clients_func_(get_all_rpc_clients_func),
          all_log_entries_(),
          is_running_(true),
          repeated_timer_(std::make_unique<common::RepeatedTimer>(
              io_service_, [this](const asio::error_code &e) { DoPushLogs(); })) {}

    ~LeaderLogManager() { repeated_timer_->Stop(); }

    std::vector<LogEntry> PullLogs(const NodeID &node_id, int64_t next_log_index) {
        std::lock_guard<std::mutex> lock(mutex_);
        RAFTCPP_CHECK(next_log_index >= 0);

        /// Update the next log index first.
        next_log_indexes_[node_id] = next_log_index;
        std::vector<LogEntry> ret;
        if (next_log_index == 0) {
            // First time to fetch logs.
            if (all_log_entries_.find(next_log_index) != all_log_entries_.end()) {
                ret = {all_log_entries_[0]};
            } else {
                RAFTCPP_LOG(RLL_DEBUG) << "There is no logs in this leader.";
                ret = {};
            }
        } else {
            RAFTCPP_CHECK(next_log_index >= 0 && next_log_index <= MAX_LOG_INDEX);
            ret = {all_log_entries_[next_log_index]};
        }
        TryAsyncCommitLogs(node_id, next_log_index,
                           /*done=*/[this](int64_t dumped_log_index) {});
        return ret;
    }

    void Push(const TermID &term_id,
              const std::shared_ptr<raftcpp::RaftcppRequest> &request) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++curr_log_index_;
        LogEntry entry;
        entry.term_id = term_id;
        entry.log_index = curr_log_index_;
        entry.data = request->Serialize();
        all_log_entries_[curr_log_index_] = entry;
    }

    void Run() { repeated_timer_->Start(1000); }

    void Stop() { repeated_timer_->Stop(); }

private:
    /// Try to commit the logs asynchronously. If a log was replied
    /// by more than one half of followers, it will be async-commit,
    /// and apply the user state machine. otherwise we don't dump it.
    void TryAsyncCommitLogs(const NodeID &node_id, size_t next_log_index,
                            std::function<void(int64_t)> committed_callback) {
        /// TODO(qwang): Trigger commit.
    }

    void DoPushLogs() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto all_rpc_clients = get_all_rpc_clients_func_();
        for (const auto &follower : all_rpc_clients) {
            const auto &follower_node_id = follower.first;
            // Filter this node self.
            if (follower_node_id == this_node_id_) {
                continue;
            }
            auto next_log_index_to_be_sent = next_log_indexes_[follower_node_id];
            auto it = all_log_entries_.find(next_log_index_to_be_sent);
            if (it == all_log_entries_.end()) {
                continue;
            }
            auto &follower_rpc_client = follower.second;
            follower_rpc_client->async_call(
                RaftcppConstants::REQUEST_PUSH_LOGS,
                [](boost::system::error_code ec, string_view data) {
                    //// LOG
                },
                it->second);
        }
    }

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
