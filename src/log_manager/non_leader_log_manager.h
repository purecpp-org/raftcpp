#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "common/constants.h"
#include "log_manager/blocking_queue_interface.h"
#include "log_manager/blocking_queue_mutex_impl.h"
#include "log_manager/log_entry.h"

namespace raftcpp {

class NonLeaderLogManager final {
public:
    NonLeaderLogManager(std::function<bool()> is_leader_func,
                        std::function<std::shared_ptr<rest_rpc::rpc_client>()> get_leader_rpc_client_func)
        : is_leader_func_(std::move(is_leader_func)),
          is_running_(true),
          queue_in_non_leader_(std::make_unique<BlockingQueueMutexImpl<LogEntry>>()),
          get_leader_rpc_client_func_(std::move(get_leader_rpc_client_func)) {
        committing_thread_ = std::make_unique<std::thread>([this]() {
            while (is_running_ && !is_leader_func_()) {
                auto log = queue_in_non_leader_->Pop();
                CommitLogToStateMachine(log);
                std::lock_guard<std::mutex> lock(mutex_);
                committed_index_ = log.log_index;
            }
        });

        pull_thread_ = std::make_unique<std::thread>([this]() {
            /// TODO: Use RepeatedTimer instead.
            while (is_running_) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    auto leader_rpc_client = get_leader_rpc_client_func_();
                    if (leader_rpc_client == nullptr) {
                        RAFTCPP_LOG(RLL_INFO) << "Failed to get leader rpc client.Is this node the leader? "
                                              << is_leader_func_();
                        is_running_.store(false);
                        continue;
                    }
                    leader_rpc_client->async_call<1>(
                        RaftcppConstants::REQUEST_PULL_LOGS,
                        [this](const boost::system::error_code &ec, string_view data) {
                            HandleReceivedLogsFromLeader({});
                        },
                        /*this_node_id_str=*/this_node_id_.ToHex(),
                        /*committed_index=*/committed_index_);
                }
                // TODO(qwang): This should be refined.
                std::this_thread::sleep_for(std::chrono::milliseconds{2 * 1000});
            }
        });
    }

    ~NonLeaderLogManager() {
        committing_thread_->detach();
        pull_thread_->detach();
    };

private:
    void CommitLogToStateMachine(const LogEntry &log) {
        /// TODO: A callback to commit to fsm.
        /// state_machine的接口，没有用户的真正子类state_machine
        /// ->post(onApply);
        ///
    }

    void HandleReceivedLogsFromLeader(LogEntry log_entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        /// 对比term id/log index来决定:
        /// 1) 现在leader的状态, 要不要更新节点的状态，或者丢弃log，更新本地log
        /// (直接参照raft论文即可) 2) 如果没问题，则commit logs to state machine
    }

private:
    std::mutex mutex_;

    /// The index where this node committed.
    int64_t committed_index_ = -1;

    /// Next index to be read from leader.
    int64_t next_index_ = 0;

    std::unique_ptr<BlockingQueueInterface<LogEntry>> queue_in_non_leader_{nullptr};

    /// The thread used to send pull log entries requests to leader.
    /// Note that this is only used in non-leader node.
    std::unique_ptr<std::thread> pull_thread_;

    std::unique_ptr<std::thread> committing_thread_;

    /// The function to get leader rpc client.
    std::function<std::shared_ptr<rest_rpc::rpc_client>()> get_leader_rpc_client_func_;

    std::function<bool()> is_leader_func_ = nullptr;

    std::atomic_bool is_running_ = false;

    NodeID this_node_id_;
};

}  // namespace raftcpp
