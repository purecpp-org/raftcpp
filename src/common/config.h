#pragma once

#include <map>
#include <regex>
#include <string>

#include "endpoint.h"

namespace raftcpp {
namespace common {

class Config final {
public:
    Config(Config &&c)
        : other_endpoints_(std::move(c.other_endpoints_)),
          this_endpoint_(std::move(c.this_endpoint_)) {}

    static Config From(const std::string &config_str);

    const std::map<int64_t, Endpoint> &GetOtherEndpoints() const {
        return other_endpoints_;
    }

    Endpoint GetThisEndpoint() const { return this_endpoint_.second; }

    int64_t GetThisId() const { return this_endpoint_.first; }

    Config(const Config &c) = default;

    size_t GetNodesNum() const { return 1 + other_endpoints_.size(); }

    bool GreaterThanHalfNodesNum(size_t num) const { return num > GetNodesNum() / 2; }

    std::string ToString() const {
        std::string s = this_endpoint_.second.ToString();
        for (const auto &e : other_endpoints_) {
            s.append("," + e.second.ToString());
        }
        return s;
    }

    Config &operator=(const Config &c) {
        if (&c == this) {
            return *this;
        }
        this_endpoint_ = c.this_endpoint_;
        other_endpoints_ = c.other_endpoints_;
        return *this;
    }

    bool operator==(const Config &c) const {
        if (&c == this) {
            return true;
        }
        return this_endpoint_ == c.this_endpoint_ &&
               other_endpoints_ == c.other_endpoints_;
    };

    bool operator!=(const Config &c) const { return !(*this == c); }

    Config() = default;

private:
    // Push to other endpoints.
    void PushBack(int64_t id, Endpoint endpoint) {
        other_endpoints_.emplace(id, endpoint);
    }

    std::map<int64_t, Endpoint> other_endpoints_;

    std::pair<int64_t, Endpoint> this_endpoint_;
};

}  // namespace common
}  // namespace raftcpp
