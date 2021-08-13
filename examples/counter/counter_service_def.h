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

    static std::shared_ptr<CounterRequest> Deserialize1(const std::string &s);

};

class IncrRequest : public CounterRequest {
public:
    explicit IncrRequest(uint64_t delta):delta_(delta) {}

    IncrRequest() = default;

    uint64_t GetDelta() const { return delta_; }

    std::string Serialize() override {
        msgpack_codec codec;
        auto s_buf = codec.pack(this->delta_);
        /// TODO: NOTE: 0 -> Incr request
        return "0" + std::string(s_buf.data(), s_buf.size());
    }

    virtual void Deserialize(const std::string &s) override {
        msgpack_codec codec;
        this->delta_ = codec.unpack<uint64_t>(s.data(), s.size());
    }

private:
    uint64_t delta_ = 0;
};

class GetRequest : public CounterRequest {
public:
    explicit GetRequest() {}

    std::string Serialize() override {
        // TODO(qwang):
        return "";
    }

    virtual void Deserialize(const std::string &s) override {
        // TODO
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

std::shared_ptr<CounterRequest> CounterRequest::Deserialize1(const std::string &s) {
    if (std::string("0") == s.substr(0, 1)) {
        auto ret = std::make_shared<IncrRequest>();
        ret->Deserialize(s.substr(1, s.length() - 1));
        return ret;
    } else if (std::string("1") == s.substr(0, 1)) {
        auto ret = std::make_shared<GetRequest>();
        ret->Deserialize(s.substr(1, s.length() - 1));
        return ret;
    }
    return nullptr;
}

struct CounterService {
    void Incr(rpc_conn, int delta) {}
};

}  // namespace counter
}  // namespace examples