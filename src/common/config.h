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

    std::vector<Endpoint> GetAllEndpoints() const { return endpoints; }

private:
    Config() = default;

    Config(const Config &c) = default;

    void PushBack(const Endpoint &endpoint) { endpoints.push_back(endpoint); }

    std::vector<Endpoint> endpoints;
};

}  // namespace common
}  // namespace raftcpp
