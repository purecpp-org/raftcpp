#include "non_leader_log_manager.h"

#include <condition_variable>
#include <memory>

#include "src/common/logging.h"
#include "src/statemachine/state_machine.h"

namespace raftcpp {

NonLeaderLogManager::NonLeaderLogManager(
    int64_t this_node_id, std::shared_ptr<StateMachine> fsm,
    std::function<bool()> is_leader_func,
    std::function<std::shared_ptr<raftrpc::Stub>()> get_leader_rpc_client_func,
    const std::shared_ptr<common::TimerManager> &timer_manager)
    : this_node_id_(this_node_id),
      is_leader_func_(std::move(is_leader_func)),
      is_running_(false),
      get_leader_rpc_client_func_(std::move(get_leader_rpc_client_func)),
      fsm_(std::move(fsm)),
      timer_manager_(timer_manager) {}

void NonLeaderLogManager::Run(std::unordered_map<int64_t, LogEntry> &logs,
                              int64_t committedIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    committed_log_index_ = committedIndex;
    next_index_ = logs.size();
    all_log_entries_.swap(logs);
}

void NonLeaderLogManager::Stop() { is_running_.store(false); }

bool NonLeaderLogManager::IsRunning() const { return is_running_.load(); }

void NonLeaderLogManager::Push(int64_t committed_log_index, int64_t pre_log_term,
                               LogEntry log_entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    // RAFTCPP_CHECK(log_entry.log_index >= 0);
    RAFTCPP_CHECK(log_entry.index() >= 0);

    /// Ignore if duplicated log_index.
    if (all_log_entries_.count(log_entry.index()) > 0) {
        RAFTCPP_LOG(RLL_DEBUG) << "Duplicated log index = " << log_entry.index();
    }

    auto pre_log_index = log_entry.index() - 1;
    if (log_entry.index() > 0) {
        auto it = all_log_entries_.find(pre_log_index);
        if (it == all_log_entries_.end() || it->second.term() != pre_log_term) {
            next_index_ = pre_log_index;
            push_log_result_ = false;

            RAFTCPP_LOG(RLL_DEBUG) << "lack of log index = " << pre_log_index;
            return;
        }
    }

    auto req_term = log_entry.term();
    auto it = all_log_entries_.find(log_entry.index());
    if (it != all_log_entries_.end() && it->second.term() != req_term) {
        auto index = log_entry.index();
        while ((it = all_log_entries_.find(index)) != all_log_entries_.end()) {
            all_log_entries_.erase(it);
            index++;
        }

        next_index_ = log_entry.index();
        RAFTCPP_LOG(RLL_DEBUG) << "conflict at log index = " << next_index_;
    }

    all_log_entries_[log_entry.index()] = log_entry;
    if (log_entry.index() >= next_index_) {
        next_index_ = log_entry.index() + 1;
    }

    push_log_result_ = true;
    CommitLogs(committed_log_index);
}

void NonLeaderLogManager::CommitLogs(int64_t committed_log_index) {
    if (committed_log_index <= committed_log_index_) {
        return;
    }
    const auto last_committed_log_index = committed_log_index;
    committed_log_index_ = committed_log_index;
    for (auto index = last_committed_log_index + 1; index <= committed_log_index_;
         ++index) {
        RAFTCPP_CHECK(all_log_entries_.count(index) == 1);
        fsm_->OnApply(all_log_entries_[index].data());
    }
}

}  // namespace raftcpp
