#pragma once

#include <string>

namespace raftcpp {

class Endpoint {
public:
    Endpoint() = default;

    explicit Endpoint(const std::string &address) {
        const auto index = address.find(':');
        host_ = address.substr(0, index);
        // Note that stoi may throw an exception once the format is incorrect.
        port_ = std::stoi(address.substr(index + 1, address.size()));
    }

    Endpoint(std::string host, const uint16_t port)
        : host_(std::move(host)), port_(port) {}

    std::string ToString() const { return host_ + ":" + std::to_string(port_); }

    std::string GetHost() const { return host_; }

    uint16_t GetPort() const { return port_; }

private:
    std::string host_;
    uint16_t port_;
};

}  // namespace raftcpp
