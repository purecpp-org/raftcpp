#pragma once

#include <sstream>
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

    bool operator==(const Endpoint &e) const {
        return host_ == e.host_ && port_ == e.port_;
    }

    bool operator!=(const Endpoint &e) const { return !(*this == e); }

    bool operator()(const Endpoint &lhs, const Endpoint &rhs) const {
        char dot;
        std::vector<int> v1(4, 0);
        std::vector<int> v2(4, 0);
        std::istringstream s1(lhs.host_);
        std::istringstream s2(rhs.host_);
        s1 >> v1[0] >> dot >> v1[1] >> dot >> v1[2] >> dot >> v1[3];
        s2 >> v2[0] >> dot >> v2[1] >> dot >> v2[2] >> dot >> v2[3];

        uint64_t result1 = (((v1[0] * 255 + v1[1]) * 255 + v1[2]) * 255 + v1[3]);
        uint64_t result2 = (((v2[0] * 255 + v2[1]) * 255 + v2[2]) * 255 + v2[3]);
        if (result1 < result2) {
            return true;
        } else if (result1 == result2) {
            return lhs.port_ < rhs.port_;
        } else {
            return false;
        }
    }

    friend std::ostream &operator<<(std::ostream &out, const Endpoint &e) {
        out << e.host_ + ":" + std::to_string(e.port_);
        return out;
    }

    friend std::istream &operator>>(std::istream &in, Endpoint &e) {
        in >> e.host_ >> e.port_;
        return in;
    }

private:
    std::string host_;
    uint16_t port_;
};

}  // namespace raftcpp

namespace std {
template <>
struct hash<raftcpp::Endpoint> {
    std::size_t operator()(const raftcpp::Endpoint &e) const noexcept {
        return std::hash<std::string>()(e.GetHost()) ^
               (std::hash<uint16_t>()(e.GetPort()) << 1u);
    }
};
}  // namespace std
