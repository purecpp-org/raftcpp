#ifndef _RAFTCPP_EXAMPLE_COUNTER_COUNTER_STATE_MACHINE_H
#define _RAFTCPP_EXAMPLE_COUNTER_COUNTER_STATE_MACHINE_H

#include <atomic>

#include "common/file.h"
#include "common/status.h"
#include "counter_service_def.h"
#include "statemachine/state_machine.h"

namespace examples {
namespace counter {

using Status = raftcpp::Status;
/**
 * The CounterStateMachine extends from raftcpp::StateMachine. It overrides
 * the snapshot interfaces that user can decide when and how do snapshot.
 *
 * The customs state machine must override the OnApply, OnApply interface defines
 * how to apply the request from clients.
 *
 * Now in this state machine, we do the snapshot for every 3 requests.
 */
class CounterStateMachine : public raftcpp::StateMachine {
public:
    // We should do snapshot for every 3 requests.
    bool ShouldDoSnapshot() override { return received_requests_num_.load() % 3; }

    void SaveSnapshot() override {
        raftcpp::File snapshot_file =
            raftcpp::File::Open("/tmp/raftcpp/counter/snapshot.txt");
        snapshot_file.CleanAndWrite(std::to_string(atomic_value_.load()));
    }

    void LoadSnapshot() override {
        raftcpp::File snapshot_file =
            raftcpp::File::Open("/tmp/raftcpp/counter/snapshot.txt");
        std::string value = snapshot_file.ReadAll();
        atomic_value_.store(static_cast<int64_t>(std::stoi(value)));
    }

    raftcpp::RaftcppResponse OnApply(const std::string &serialized) override {
        received_requests_num_.fetch_add(1);

        /// serialized to Request.
        auto counter_request = CounterRequest::Deserialize1(serialized);
        if (counter_request->GetType() == CounterRequestType::INCR) {
            counter_request.get();
            auto *incr_request = dynamic_cast<IncrRequest *>(counter_request.get());
            atomic_value_.fetch_add(incr_request->GetDelta());
            return IncrResponse(Status::OK);
        } else if (counter_request->GetType() == CounterRequestType::GET) {
            auto *get_request = dynamic_cast<GetRequest *>(counter_request.get());
            return GetResponse(atomic_value_.load());
        }

        return CounterResponse(Status::UNKNOWN_REQUEST);
    }

    int64_t GetValue() const { return atomic_value_.load(); }

private:
    std::atomic<int64_t> atomic_value_;

    // the number of received requests to decided when we do snapshot.
    std::atomic<uint64_t> received_requests_num_;
};

}  // namespace counter
}  // namespace examples

#endif
