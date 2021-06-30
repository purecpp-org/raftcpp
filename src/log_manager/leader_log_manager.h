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

class LeaderLogManager final {
public:
    explicit LeaderLogManager(NodeID this_node_id,
                              std::function<AllRpcClientType()> get_all_rpc_clients_func)
        : io_service_(),
          this_node_id_(std::move(this_node_id)),
          get_all_rpc_clients_func_(get_all_rpc_clients_func),
          all_log_entries_(),
          is_running_(false),
          repeated_timer_(std::make_unique<common::RepeatedTimer>(
              io_service_, [this](const asio::error_code &e) { DoPushLogs(); })) {}

    ~LeaderLogManager() { repeated_timer_->Stop(); }

    std::vector<LogEntry> PullLogs(const NodeID &node_id, int64_t committed_log_index) {
        std::lock_guard<std::mutex> lock(mutex_);

        /// Update last committed log index first.
        committed_log_indexes_[node_id] = committed_log_index;
        std::vector<LogEntry> ret;
        if (committed_log_index == -1) {
            // First time to fetch logs.
            //            ret = queue_in_leader_->MostFront(/*most_front_number=*/10);
            RAFTCPP_CHECK(all_log_entries_.find(0) != all_log_entries_.end());
            ret = {all_log_entries_[0]};
        } else {
            RAFTCPP_CHECK(committed_log_index >= 0 &&
                          committed_log_index <= MAX_LOG_INDEX);
            RAFTCPP_CHECK(all_log_entries_.find(committed_log_index + 1) !=
                          all_log_entries_.end());
            ret = {all_log_entries_[committed_log_index + 1]};
        }
        TriggerAsyncDumpLogs(committed_log_index,
                             /*done=*/[this](int64_t dumped_log_index) {
                                 /// Since we dumped the logs, so that we can cleanup it
                                 /// from queue_in_leader_.
                             });
        return ret;
    }

    void Push(const TermID &term_id,
              const std::shared_ptr<raftcpp::RaftcppRequest> &request) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++curr_log_index_;
        LogEntry entry;
        entry.term_id = term_id;
        // Fake log index. Will be refined after we support log index.
        entry.log_index = curr_log_index_;
        entry.data = request->Serialize();
        all_log_entries_[curr_log_index_] = entry;
    }

    void Run() { repeated_timer_->Start(1000); }

    void Stop() { repeated_timer_->Stop(); }

private:
    void TriggerAsyncDumpLogs(size_t committed_log_index,
                              std::function<void(int64_t)> dumped_callback) {
        // TODO: Dump logs.
    }

    void DoPushLogs() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto all_rpc_clients = get_all_rpc_clients_func_();
        for (const auto &follower : all_rpc_clients) {
            const auto &follower_node_id = follower.first;
            // Filter myself node.
            if (follower_node_id == this_node_id_) {
                continue;
            }
            auto log_index_to_be_sent = committed_log_indexes_[follower_node_id] + 1;
            auto it = all_log_entries_.find(log_index_to_be_sent);
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

    /// The largest log index that not committed.
    int64_t curr_log_index_ = -1;

    std::unordered_map<int64_t, LogEntry> all_log_entries_;

    // The map that contains the non-leader nodes to the committed_log_index.
    std::unordered_map<NodeID, int64_t> committed_log_indexes_;

    /// The function to get all rpc clients to followers(Including this node self).
    std::function<AllRpcClientType()> get_all_rpc_clients_func_;

    /// ID of this node.
    NodeID this_node_id_;

    boost::asio::io_service io_service_;

    std::unique_ptr<common::RepeatedTimer> repeated_timer_;

    std::atomic_bool is_running_ = false;

    constexpr static size_t MAX_LOG_INDEX = 1000000000;
};

}  // namespace raftcpp
