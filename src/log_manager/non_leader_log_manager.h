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

    void Run();

    void Stop();

    void Push(int64_t committed_log_index, LogEntry log_entry);

private:
    void CommitLogs(int64_t committed_log_index);

    void DoPullLogs();

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
