#pragma once

#include <regex>
#include <string>
#include <vector>

#include "common/endpoint.h"

namespace raftcpp {

class Config final {
    public:
    static Config From(const std::string &config_str) {
        Config config;
        config.endpoints.clear();
        const static std::regex reg(R"((\d{1,3}(\.\d{1,3}){3}:\d+))");
        std::sregex_iterator pos(config_str.begin(), config_str.end(), reg);
        decltype(pos) end;
        for (; pos != end; ++pos) {
            config.PushBack(Endpoint(pos->str()));
        }
        return config;
    }

    std::vector<Endpoint> GetAllEndpoints() const { return endpoints; }

    private:
    Config() = default;

    Config(const Config &c) = default;

    void PushBack(const Endpoint &endpoint) { endpoints.push_back(endpoint); }

    std::vector<Endpoint> endpoints;
};

}  // namespace raftcpp
