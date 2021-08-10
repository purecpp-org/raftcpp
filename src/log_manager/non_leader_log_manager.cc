#include "log_manager/non_leader_log_manager.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "common/constants.h"
#include "common/logging.h"
#include "common/timer.h"
#include "log_manager/blocking_queue_interface.h"
#include "log_manager/blocking_queue_mutex_impl.h"
#include "log_manager/log_entry.h"
#include "statemachine/state_machine.h"

namespace raftcpp {

NonLeaderLogManager::NonLeaderLogManager(
    std::shared_ptr<StateMachine> fsm, std::function<bool()> is_leader_func,
    std::function<std::shared_ptr<rest_rpc::rpc_client>()> get_leader_rpc_client_func,
    const std::shared_ptr<common::TimerManager> &timer_manager)
    : is_leader_func_(std::move(is_leader_func)),
      is_running_(false),
      get_leader_rpc_client_func_(std::move(get_leader_rpc_client_func)),
      fsm_(std::move(fsm)),
      timer_manager_(timer_manager) {
    timer_manager_->RegisterTimer(RaftcppConstants::TIMER_PULL_LOGS,
                                  std::bind(&NonLeaderLogManager::DoPullLogs, this));
}

void NonLeaderLogManager::Run() {
    timer_manager_->StartTimer(RaftcppConstants::TIMER_PULL_LOGS, 1000);
}

void NonLeaderLogManager::Stop() {
    timer_manager_->StopTimer(RaftcppConstants::TIMER_PULL_LOGS);
}

void NonLeaderLogManager::Push(int64_t committed_log_index, LogEntry log_entry) {
    RAFTCPP_CHECK(log_entry.log_index >= 0);
    /// Ignore if duplicated log_index.
    if (all_log_entries_.count(log_entry.log_index) > 0) {
        RAFTCPP_LOG(RLL_DEBUG) << "Duplicated log index = " << log_entry.log_index;
        return;
    }
    if (log_entry.log_index > 0) {
        RAFTCPP_CHECK(all_log_entries_.count(log_entry.log_index - 1) == 1);
    }
    RAFTCPP_CHECK(all_log_entries_.count(log_entry.log_index - 1) == 1);
    all_log_entries_[log_entry.log_index] = log_entry;
    if (log_entry.log_index >= next_index_) {
        next_index_ = log_entry.log_index + 1;
    }
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
        fsm_->OnApply(all_log_entries_[index].data);
    }
}

void NonLeaderLogManager::DoPullLogs() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto leader_rpc_client = get_leader_rpc_client_func_();
    if (leader_rpc_client == nullptr) {
        RAFTCPP_LOG(RLL_INFO) << "Failed to get leader rpc client.Is "
                                 "this node the leader? "
                              << is_leader_func_();
        is_running_.store(false);
        return;
    }
    leader_rpc_client->async_call<1>(
        RaftcppConstants::REQUEST_PULL_LOGS,
        [this](const boost::system::error_code &ec, string_view data) {},
        /*this_node_id_str=*/this_node_id_.ToHex(),
        /*next_index=*/next_index_);
}

}  // namespace raftcpp
