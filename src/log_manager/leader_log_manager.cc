#include "log_manager/leader_log_manager.h"

#include <condition_variable>
#include <memory>
#include <msgpack.hpp>
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
      timer_manager_(timer_manager) {
    timer_manager->RegisterTimer(RaftcppConstants::TIMER_PUSH_LOGS,
                                 std::bind(&LeaderLogManager::DoPushLogs, this));
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

void LeaderLogManager::Run(std::unordered_map<int64_t, LogEntry> &logs,
                           int64_t committedIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto size = static_cast<int64_t>(logs.size());
    curr_log_index_ = size - 1;
    committed_log_index_ = committedIndex;
    all_log_entries_.swap(logs);

    timer_manager_->StartTimer(RaftcppConstants::TIMER_PUSH_LOGS, 1000);
}

void LeaderLogManager::Stop() {
    timer_manager_->StopTimer(RaftcppConstants::TIMER_PUSH_LOGS);
}

void LeaderLogManager::TryAsyncCommitLogs(
    const NodeID &node_id, size_t next_log_index,
    std::function<void(int64_t)> committed_callback) {
    /// TODO(qwang): Trigger commit.

    std::vector<int64_t> index_nums;
    for (const auto &it : match_log_indexes_) {
        index_nums.emplace_back(it.second);
    }
    index_nums.emplace_back(curr_log_index_);

    std::sort(index_nums.begin(), index_nums.end());
    auto size = index_nums.size();
    auto i = (size % 2 == 0) ? size / 2 : size / 2 + 1;
    int media_index = index_nums.at(i);
    if (media_index > committed_log_index_) {
        committed_log_index_ = media_index;
    }
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

        // get log entry
        auto it = all_log_entries_.find(next_log_index_to_be_sent);
        if (it == all_log_entries_.end()) {
            continue;
        }

        // get pre_log_term
        int32_t pre_log_term_num = -1;
        if (next_log_index_to_be_sent > 0) {
            auto it = all_log_entries_.find(next_log_index_to_be_sent - 1);
            if (it != all_log_entries_.end()) {
                pre_log_term_num = it->second.term_id.getTerm();
            }
        }

        // do request push log
        auto &follower_rpc_client = follower.second;
        follower_rpc_client->async_call(
            RaftcppConstants::REQUEST_PUSH_LOGS,
            [this, next_log_index_to_be_sent, follower_node_id](
                boost::system::error_code ec, string_view data) {
                /**
                 * The return parameters are success and lastLogIdx
                 * @param success if push logs successfully
                 * @param lastLogIdx response tells the leader the last log index it has
                 */
                msgpack::type::tuple<bool, int64_t> response;
                msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
                msgpack::object obj = oh.get();
                obj.convert(response);

                bool success = response.get<0>();
                int64_t resp_last_log_idx = response.get<1>();

                std::lock_guard<std::mutex> lock(this->mutex_);

                auto iter = this->next_log_indexes_.find(follower_node_id);
                if (success) {
                    iter->second = next_log_index_to_be_sent;
                } else {
                    iter->second = resp_last_log_idx;
                }
            },
            /*committed_log_index=*/committed_log_index_,
            /*pre_log_term_num=*/pre_log_term_num,
            /*log_entry=*/it->second);
    }
}

}  // namespace raftcpp
