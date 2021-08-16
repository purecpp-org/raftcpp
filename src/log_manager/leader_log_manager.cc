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
    timer_manager->RegisterTimer(RaftcppConstants::TIMER_PUSH_LOGS,
                                 std::bind(&LeaderLogManager::DoPushLogs, this));
}

void LeaderLogManager::PullLogs(const NodeID &node_id, int64_t next_log_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    RAFTCPP_CHECK(next_log_index >= 0 && next_log_index <= MAX_LOG_INDEX);

    /// Update the next log index first.
    auto it = next_log_indexes_.find(node_id);
    if (it != next_log_indexes_.end() && it->second >= next_log_index) {
        return;
    }
    next_log_indexes_[node_id] = next_log_index;

    TryAsyncCommitLogs(node_id, next_log_index,
                       /*done=*/[this](int64_t dumped_log_index) {});
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

void LeaderLogManager::Run() {
    timer_manager_->StartTimer(RaftcppConstants::TIMER_PUSH_LOGS, 1000);
}

void LeaderLogManager::Stop() {
    timer_manager_->StopTimer(RaftcppConstants::TIMER_PUSH_LOGS);
}

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

        if (next_log_indexes_.find(follower_node_id) == next_log_indexes_.end()) {
            continue;
        }
        auto next_log_index_to_be_sent = next_log_indexes_[follower_node_id];

        // get pre_log_term
        int32_t pre_log_term_num = -1;
        if (next_log_index_to_be_sent > 0) {
            auto it = all_log_entries_.find(next_log_index_to_be_sent - 1);
            if (it != all_log_entries_.end()) {
                pre_log_term_num = it->second.term_id.getTerm();
            }
        }

        // get log entry
        auto it = all_log_entries_.find(next_log_index_to_be_sent);
        if (it == all_log_entries_.end()) {
            continue;
        }

        // do request push log
        auto &follower_rpc_client = follower.second;
        follower_rpc_client->async_call(
            RaftcppConstants::REQUEST_PUSH_LOGS,
            [](boost::system::error_code ec, string_view data) {
                //// LOG
            },
            /*committed_log_index=*/committed_log_index_,
            /*pre_log_term_num=*/pre_log_term_num,
            /*log_entry=*/it->second);
    }
}

}  // namespace raftcpp
