#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <memory>

#include "common/constants.h"
#include "log_manager/blocking_queue_interface.h"
#include "log_manager/blocking_queue_mutex_impl.h"

namespace raftcpp {

class LeaderLogManager final {

public:
    LeaderLogManager() : all_log_entries_(), is_running_(true) {
        push_thread_ = std::make_unique<std::thread>([this] () {
            while(is_running_) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    for (auto follower : follower_rpc_clients_) {
                        const auto &follower_node_id = follower.first;
                        auto log_index_to_be_sent = committed_log_indexes_[follower_node_id] + 1;
                        auto it = all_log_entries_.find(log_index_to_be_sent);
                        if (it == all_log_entries_.end()) {
                            continue;
                        }
                        auto &follower_rpc_client = follower.second;
                        follower_rpc_client->async_call(RaftcppConstants::REQUEST_PUSH_LOGS,
                            [](boost::system::error_code ec, string_view data) {
                                //// LOG
                            }, it->second);
                    }
                }
                // TODO(qwang): This should be refined.
                std::this_thread::sleep_for(std::chrono::milliseconds{2 * 1000});
            }
        });
    }

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
            RAFTCPP_CHECK(committed_log_index >= 0 && committed_log_index <= MAX_LOG_INDEX);
            RAFTCPP_CHECK(all_log_entries_.find(committed_log_index + 1) != all_log_entries_.end());
            ret = {all_log_entries_[committed_log_index + 1]};
        }
        TriggerAsyncDumpLogs(committed_log_index, /*done=*/[this](int64_t dumped_log_index) {
            /// Since we dumped the logs, so that we can cleanup it from queue_in_leader_.
        });
        return ret;
    }

    void Push(const TermID &term_id, const std::shared_ptr<raftcpp::RaftcppRequest> &request) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++curr_log_index_;
        LogEntry entry;
        entry.term_id = term_id;
        // Fake log index. Will be refined after we support log index.
        entry.log_index = curr_log_index_;
        entry.data = request->Serialize();
        all_log_entries_[curr_log_index_] = entry;
    }

private:
    void TriggerAsyncDumpLogs(size_t committed_log_index, std::function<void(int64_t)> dumped_callback) {
        // TODO: Dump logs.
    }

private:
    std::mutex mutex_;

    /// The largest log index that not committed.
    int64_t curr_log_index_ = -1;

    std::unordered_map<int64_t, LogEntry> all_log_entries_;

    // The map that contains the non-leader nodes to the committed_log_index.
    std::unordered_map<NodeID, int64_t> committed_log_indexes_;

//    std::unique_ptr<BlockingQueueInterface<LogEntry>> queue_in_non_leader_ {nullptr};

    /// The thread used to push the log entries to non-leader nodes.
    /// Note that this is only used in leader.
    std::unique_ptr<std::thread> push_thread_;

    /// The rpc client to followers.
    std::unordered_map<NodeID, std::shared_ptr<rest_rpc::rpc_client>> follower_rpc_clients_;

    std::atomic_bool is_running_ = false;

    constexpr static size_t MAX_LOG_INDEX = 1000000000;
};

} // namespace raftcpp
