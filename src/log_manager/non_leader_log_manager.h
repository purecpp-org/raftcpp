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
#include "statemachine/state_machine.h"

namespace raftcpp {

class NonLeaderLogManager final {
public:
    NonLeaderLogManager(
        std::shared_ptr<StateMachine> fsm, std::function<bool()> is_leader_func,
        std::function<std::shared_ptr<rest_rpc::rpc_client>()> get_leader_rpc_client_func)
        : io_service_(),
          pull_logs_timer_(std::make_unique<common::RepeatedTimer>(
              io_service_, [this](const asio::error_code &e) { DoPullLogs(); })),
          is_leader_func_(std::move(is_leader_func)),
          is_running_(false),
          get_leader_rpc_client_func_(std::move(get_leader_rpc_client_func)),
          fsm_(std::move(fsm)) {}

    ~NonLeaderLogManager(){};

    void Run() { pull_logs_timer_->Start(1000); }

    void Stop() { pull_logs_timer_->Stop(); }

    void Push(int64_t committed_log_index, LogEntry log_entry) {
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

private:
    void CommitLogs(int64_t committed_log_index) {
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

    void HandleReceivedLogsFromLeader(LogEntry log_entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        /// 对比term id/log index来决定:
        /// 1) 现在leader的状态, 要不要更新节点的状态，或者丢弃log，更新本地log
        /// (直接参照raft论文即可) 2) 如果没问题，则commit logs to state machine
    }

    void DoPullLogs() {
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

private:
    std::mutex mutex_;

    /// The index which the leader committed.
    int64_t committed_log_index_ = -1;

    /// Next index to be read from leader.
    int64_t next_index_ = 0;

    std::unordered_map<int64_t, LogEntry> all_log_entries_;

    boost::asio::io_service io_service_;

    /// The timer used to send pull log entries requests to leader.
    /// Note that this is only used in non-leader node.
    std::unique_ptr<common::RepeatedTimer> pull_logs_timer_;

    std::unique_ptr<std::thread> committing_thread_;

    /// The function to get leader rpc client.
    std::function<std::shared_ptr<rest_rpc::rpc_client>()> get_leader_rpc_client_func_;

    std::function<bool()> is_leader_func_ = nullptr;

    std::atomic_bool is_running_ = false;

    NodeID this_node_id_;

    std::shared_ptr<StateMachine> fsm_;
};

}  // namespace raftcpp
