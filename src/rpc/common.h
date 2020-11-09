#pragma once

namespace raftcpp {

class RaftcppRequest {
public:
    virtual ~RaftcppRequest() = default;
};

class RaftcppResponse {
public:
    virtual ~RaftcppResponse() = default;
};

}  // namespace raftcpp
