#pragma once

#include <string>

namespace raftcpp {

class Endpoint {
public:
    explicit Endpoint(const std::string address) {
        // TODO(qwang):
    }

    Endpoint(const std::string &host, const uint16_t port) : host_(host), port_(port) {}

    std::string GetHost() const { return host_; }

    uint16_t GetPort() const { return port_; }

private:
    std::string host_;
    uint16_t port_;
};

}  // namespace raftcpp
