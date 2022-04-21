#pragma once

#include "src/statemachine/state_machine.h"

namespace raftcpp {

class RaftStateMachine : public raftcpp::StateMachine {
public:
    bool ShouldDoSnapshot() override { return true; }

    void SaveSnapshot() override{};

    void LoadSnapshot() override{};

    bool OnApply(const std::string &serialized) override { return true; }

    // virtual raftcpp::RaftcppResponse OnApply(const std::string &serialized) override {
    //     return MockResponse();
    // };

private:
};
}  // namespace raftcpp