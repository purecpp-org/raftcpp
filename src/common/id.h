#pragma once
#include "common/endpoint.h"
namespace raftcpp {

class NodeID {
public:
    NodeID(const Endpoint &endpoint_id) : endpoint_id_(endpoint_id) {
        node_id_ = "";
        char node_id[6];
        memset(node_id, 0, sizeof(node_id));
        int ipint = ip2uint(endpoint_id_.GetHost());
        short port = endpoint_id_.GetPort();
        int len = 0;
        memcpy(node_id, &ipint, 4);
        len += 4;
        memcpy(node_id + len, &port, 2);
        node_id_.assign(node_id, 6);
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
    Endpoint endpoint_id_;
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
