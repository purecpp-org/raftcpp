#include "node/timer_manager.h"

#include "common/logging.h"
#include "common/util.h"

namespace raftcpp {
namespace node {

TimerManager::TimerManager(const std::function<void()> &election_timer_timeout_handler,
                           const std::function<void()> &heartbeat_timer_timeout_handler,
                           const std::function<void()> &vote_timer_timeout_handler) {
    io_service_ = std::make_unique<asio::io_service>();

    election_timer_ = std::make_unique<common::RepeatedTimer>(
        *io_service_, [election_timer_timeout_handler](const asio::error_code &e) {
            if (e.value() != asio::error::operation_aborted) {
                RAFTCPP_LOG(DEBUG)
                    << "Election time's up, request election with error=" << e.message();
                election_timer_timeout_handler();
            }
        });

    heartbeat_timer_ = std::make_unique<common::RepeatedTimer>(
        *io_service_, [heartbeat_timer_timeout_handler](const asio::error_code &e) {
            if (e.value() != asio::error::operation_aborted) {
                heartbeat_timer_timeout_handler();
            }
        });

    vote_timer_ = std::make_unique<common::RepeatedTimer>(
        *io_service_, [vote_timer_timeout_handler](const asio::error_code &e) {
            if (e.value() != asio::error::operation_aborted) {
                vote_timer_timeout_handler();
            }
        });
}

TimerManager::~TimerManager() {
    io_service_->stop();
    thread_->join();
}

void TimerManager::Start() {
    // Note that the `Start()` should be invoked before `io_service->run()`.
    election_timer_->Start(common::RandomNumber(1000, 2000));
    thread_ = std::make_unique<std::thread>([this]() { io_service_->run(); });
}

}  // namespace node
}  // namespace raftcpp
