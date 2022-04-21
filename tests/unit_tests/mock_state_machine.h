#pragma once

#include "statemachine/state_machine.h"

class MockResponse {
public:
    MockResponse() {}

    ~MockResponse()  {}
};

class MockStateMachine : public raftcpp::StateMachine {
public:
    bool ShouldDoSnapshot() override { return true; }

    void SaveSnapshot() override{};

    void LoadSnapshot() override{};

    // virtual raftcpp::RaftcppResponse OnApply(const std::string &serialized) override {
    //     return MockResponse();
    // };

private:
};
