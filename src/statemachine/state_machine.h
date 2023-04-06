#pragma once

#include <string>

namespace raftcpp {

class StateMachine {
public:
    virtual ~StateMachine() = default;

    virtual bool ShouldDoSnapshot() { return true; }

    virtual void SaveSnapshot() = 0;

    virtual void LoadSnapshot() = 0;

    //    virtual RaftcppResponse OnApply(RaftcppRequest &request) = 0;

    virtual bool OnApply(const std::string &serialized) = 0;

private:
};

}  // namespace raftcpp
