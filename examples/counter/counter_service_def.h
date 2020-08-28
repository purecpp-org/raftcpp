#pragma once

#include "rpc/common.h"
#include "common/status.h"

#include "rpc_server.h"

using namespace rest_rpc;
using namespace rpc_service;

namespace examples {
namespace counter {

enum class CounterRequestType {
    GET,
    INCR,
};

class CounterRequest : public raftcpp::RaftcppRequest {
public:
    ~CounterRequest() override {}

    CounterRequestType GetType() const {
        return CounterRequestType::GET;
    }
};

class IncrRequest : public CounterRequest {
public:
    IncrRequest(uint64_t delta) {

    }

    uint64_t GetDelta() const {
        return -1;
    }
};

class GetRequest : public CounterRequest {

};

class CounterResponse : public raftcpp::RaftcppResponse {
public:
    explicit CounterResponse(raftcpp::Status type) {

    }

    ~CounterResponse() override {}
};

class GetResponse : public CounterResponse {
public:
    explicit GetResponse(uint64_t value) : CounterResponse(raftcpp::Status::OK) {

    }
};

class IncrResponse : public CounterResponse {
public:
    explicit IncrResponse(raftcpp::Status status) : CounterResponse(raftcpp::Status::OK) {}
};

struct CounterService {
  int Incr(rpc_conn, int delta) {
    
  }
};

}
}