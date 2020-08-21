#ifndef _RAFTCPP_EXAMPLE_COUNTER_COUNTER_STATE_MACHINE_H
#define _RAFTCPP_EXAMPLE_COUNTER_COUNTER_STATE_MACHINE_H

#include <atomic>

namespace examples {
namespace counter {

/**
 * The CounterStateMachine extends from raftcpp::StateMachine. It overrides
 * the snapshot interfaces that user can decide when and how do snapshot.
 *
 * The customs state machine must override the OnApply, OnApply interface defines
 * how to apply the request from clients.
 *
 * Now in this state machine, we do the snapshot for every 3 requests.
 */
class CounterStateMachine: public raftcpp::StateMachine {
public:

  // We should do snapshot for every 3 requests.
  bool ShouldDoSnapshot() override {
    return received_requests_num_.get() % 3;
  }

  void SaveSnapshot() override {
    File snapshot_file = File::Open("/tmp/raftcpp/counter/snapshot.txt");
    snapshot_file.CleanAndWrite(std::to_string(atomic_value_.get()));
  }

  void LoadSnapshot() override {
    File snapshot_file = File::Open("/tmp/raftcpp/counter/snapshot.txt");
    std::string value = snapshot_file.ReadAll();
    atomic_value_.store(static_cast<int64_t>(std::atoi(value)));
  }

  RaftcppResponse OnApply(RaftcppRequest request) override {
    received_requests_num_.fetch_add(1);

    auto counter_request = dynamic_cast<CounterRequest>(request);
    if (counter_request.GetType() == CounterRequest::INCR) {
      auto &incr_request = dynamic_cast<IncrRequest &>(counter_request);
      atomic_value_.fetch_add(incr_request.GetDelta());
      return IncrResponse(Status::OK);
    } else if (counter_request.GetType() == CounterRequest::GET) {
      auto &get_request = dynamic_cast<GetRequest &>(counter_request);
      return GetResponse(atomic_value_.get());
    }

    return CounterResponse(Status::UNKNOWN_REQUEST);
  }

private:
  std::atomic<int64_t> atomic_value_;

  // the number of received requests to decided when we do snapshot.
  std::atomic<uint64_t> received_requests_num_;
};

}
}

#endif
