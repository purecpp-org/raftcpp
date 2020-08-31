#include "node/timer_manager.h"

namespace raftcpp {
namespace node {

TimerManager::TimerManager(const std::function<void()> &election_timer_timeout_handler) {
    io_service_ = std::make_unique<asio::io_service>();
    election_timer_ = std::make_unique<common::RandomTimer>(
        *io_service_,
        /*random_range*/ common::Range{200, 800},
        [election_timer_timeout_handler](const asio::error_code &e) {
            std::cout << "Election time's out, request election." << std::endl;
            election_timer_timeout_handler();
        });
}

TimerManager::~TimerManager() {
    io_service_->stop();
    thread_->join();
}

void TimerManager::Start() {
    // Note that the `Start()` should be invoked before `io_service->run()`.
    election_timer_->Start();
    thread_ = std::make_unique<std::thread>([this]() { io_service_->run(); });
}

}  // namespace node
}  // namespace raftcpp
