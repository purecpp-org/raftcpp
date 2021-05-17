#pragma once

#include "common/status.h"
#include "rest_rpc/rpc_server.h"
#include "rpc/common.h"
#include "rest_rpc/codec.h"

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

    CounterRequestType GetType() const { return CounterRequestType::GET; }

    virtual std::string Serialize() override = 0;

    virtual void Deserialize(const std::string &s) override = 0;
};

class IncrRequest : public CounterRequest {
public:
    explicit IncrRequest(uint64_t delta):delta_(delta) {}

    uint64_t GetDelta() const { return delta_; }

    std::string Serialize() override {
        msgpack_codec codec;
        auto s_buf = codec.pack(this->delta_);
        return std::string(s_buf.data(), s_buf.size());
    }

    virtual void Deserialize(const std::string &s) override {
        msgpack_codec codec;
        this->delta_ = codec.unpack<uint64_t>(s.data(), s.size());
    }

private:
    uint64_t delta_ = 0;
};

class GetRequest : public CounterRequest {
    std::string Serialize() override {
        // TODO(qwang):
        return "";
    }
};

class CounterResponse : public raftcpp::RaftcppResponse {
public:
    explicit CounterResponse(raftcpp::Status type) {}

    ~CounterResponse() override {}
};

class GetResponse : public CounterResponse {
public:
    explicit GetResponse(uint64_t value) : CounterResponse(raftcpp::Status::OK) {}
};

class IncrResponse : public CounterResponse {
public:
    explicit IncrResponse(raftcpp::Status status)
        : CounterResponse(raftcpp::Status::OK) {}
};

struct CounterService {
    void Incr(rpc_conn, int delta) {}
};

}  // namespace counter
}  // namespace examples