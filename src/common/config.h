#pragma once

#include <regex>
#include <string>
#include <vector>

#include "common/endpoint.h"

namespace raftcpp {
namespace common {

class Config final {
public:
    Config(Config &&c)
        : this_endpoint_(std::move(c.this_endpoint_)),
          other_endpoints_(std::move(c.other_endpoints_)) {}

    static Config From(const std::string &config_str);

    std::vector<Endpoint> GetOtherEndpoints() const { return other_endpoints_; }

    Endpoint GetThisEndpoint() const { return this_endpoint_; }

    Config(const Config &c) = default;

    size_t GetNodesNum() const { return 1 + other_endpoints_.size(); }

    bool GreaterThanHalfNodesNum(size_t num) const { return num > GetNodesNum() / 2; }

    std::string ToString() const {
        std::string s = this_endpoint_.ToString();
        for (const auto &e : other_endpoints_) {
            s.append("," + e.ToString());
        }
        return std::move(s);
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
        if (this_endpoint_ != c.this_endpoint_) {
            return false;
        }

        std::vector<Endpoint> v1 = other_endpoints_;
        std::vector<Endpoint> v2 = c.other_endpoints_;
        std::sort(v1.begin(), v1.end(), Endpoint());
        std::sort(v2.begin(), v2.end(), Endpoint());

        return v1 == v2;
    };

    bool operator!=(const Config &c) const { return !(*this == c); }

private:
    Config() = default;

    // Push to other endpoints.
    void PushBack(const Endpoint &endpoint) { other_endpoints_.push_back(endpoint); }

    std::vector<Endpoint> other_endpoints_;

    Endpoint this_endpoint_;
};

}  // namespace common
}  // namespace raftcpp
