#pragma once
#include "common/endpoint.h"
namespace raftcpp {

class NodeID {
public:
    NodeID(const Endpoint &endpoint_id) {
        node_id_ = "";
        node_id_.resize(6);
        unsigned int ipint = ip2uint(endpoint_id.GetHost());
        uint16_t port = endpoint_id.GetPort();
        memcpy(node_id_.data(), &ipint, 4);
        memcpy(node_id_.data() + 4, &port, 2);
    }
    NodeID(const NodeID &nid) { node_id_ = nid.node_id_; }
    NodeID &operator=(const NodeID &o) {
        if (this == &o) return *this;
        node_id_ = o.node_id_;
        return *this;
    }
    std::string ToBinary() const { return node_id_; }
    std::string ToHex() const {
        std::string result;
        result.resize(12);
        for (size_t i = 0; i < node_id_.size(); i++) {
            uint8_t cTemp = node_id_[i];
            for (size_t j = 0; j < 2; j++) {
                uint8_t cCur = (cTemp & 0x0f);
                if (cCur < 10) {
                    cCur += '0';
                } else {
                    cCur += ('a' - 10);
                }
                result[2 * i + 1 - j] = cCur;
                cTemp >>= 4;
            }
        }
        return result;
    }

private:
    const std::vector<std::string> explode(const std::string &s, const char &c) {
        std::string buff{""};
        std::vector<std::string> v;

        for (auto n : s) {
            if (n != c)
                buff += n;
            else if (n == c && buff != "") {
                v.push_back(buff);
                buff = "";
            }
        }
        if (buff != "") v.push_back(buff);

        return v;
    }
    unsigned int ip2uint(const std::string &ip) {
        std::vector<std::string> v{explode(ip, '.')};
        unsigned int result = 0;
        for (auto i = 1; i <= v.size(); i++)
            if (i < 4)
                result += (stoi(v[i - 1])) << (8 * (4 - i));
            else
                result += stoi(v[i - 1]);

        return result;
    }
    std::string node_id_;
};

inline bool operator==(const NodeID &lhs, const NodeID &rhs) {
    return lhs.ToHex() == rhs.ToHex();
}
inline bool operator!=(const NodeID &lhs, const NodeID &rhs) {
    return lhs.ToHex() != rhs.ToHex();
}

class TermID {};

}  // namespace raftcpp
