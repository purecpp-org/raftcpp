#include "log_manager/leader_log_manager.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "common/constants.h"
#include "common/logging.h"
#include "common/timer.h"
#include "log_manager/log_entry.h"
#include "rest_rpc.hpp"
#include "rpc/common.h"

namespace raftcpp {

LeaderLogManager::LeaderLogManager(
    NodeID this_node_id, std::function<AllRpcClientType()> get_all_rpc_clients_func,
    const std::shared_ptr<common::TimerManager> &timer_manager)
    : io_service_(),
      this_node_id_(std::move(this_node_id)),
      get_all_rpc_clients_func_(get_all_rpc_clients_func),
      all_log_entries_(),
      is_running_(true),
      timer_manager_(timer_manager) {
    push_logs_timer_id_ =
        timer_manager->RegisterTimer(std::bind(&LeaderLogManager::DoPushLogs, this));
}

std::vector<LogEntry> LeaderLogManager::PullLogs(const NodeID &node_id,
                                                 int64_t next_log_index) {
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

void LeaderLogManager::Push(const TermID &term_id,
                            const std::shared_ptr<raftcpp::RaftcppRequest> &request) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++curr_log_index_;
    LogEntry entry;
    entry.term_id = term_id;
    entry.log_index = curr_log_index_;
    entry.data = request->Serialize();
    all_log_entries_[curr_log_index_] = entry;
}

void LeaderLogManager::Run() { timer_manager_->StartTimer(push_logs_timer_id_, 1000); }

void LeaderLogManager::Stop() { timer_manager_->StopTimer(push_logs_timer_id_); }

void LeaderLogManager::TryAsyncCommitLogs(
    const NodeID &node_id, size_t next_log_index,
    std::function<void(int64_t)> committed_callback) {
    /// TODO(qwang): Trigger commit.
}

void LeaderLogManager::DoPushLogs() {
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
            /*committed_log_index=*/committed_log_index_,
            /*log_entry=*/it->second);
    }
}

}  // namespace raftcpp
