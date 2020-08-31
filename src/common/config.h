#pragma once

#include <regex>
#include <string>
#include <vector>

#include "common/endpoint.h"

namespace raftcpp {
namespace common {

class Config final {
public:
    static Config From(const std::string &config_str);

    std::vector<Endpoint> GetOtherEndpoints() const { return other_endpoints_; }

    Endpoint GetThisEndpoint() const { return this_endpoint_; }

    Config(const Config &c) = default;

private:
    Config() = default;

    // Push to other endpoints.
    void PushBack(const Endpoint &endpoint) { other_endpoints_.push_back(endpoint); }

    std::vector<Endpoint> other_endpoints_;

    Endpoint this_endpoint_;
};

}  // namespace common
}  // namespace raftcpp
