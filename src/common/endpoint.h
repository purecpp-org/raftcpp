#pragma once

#include <string>

namespace raftcpp {

class Endpoint {
    public:
    std::string GetHost() const { return host_; }

    uint16_t GetPort() const { return port_; }

    private:
    std::string host_;
    uint16_t port_;
};

}  // namespace raftcpp
