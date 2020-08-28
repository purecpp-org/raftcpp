#pragma once

#include <string>
#include <vector>

#include "common/endpoint.h"

namespace raftcpp {

class Config final {
    public:
    static Config From(const std::string &config_str) { return Config(); }

    Endpoint GetSelfEndpoint() const {}

    std::vector<Endpoint> GetAllEndpoints() const {}

    private:
};

}  // namespace raftcpp
