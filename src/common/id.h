#pragma once

#include "common/endpoint.h"
#include <msgpack.hpp>

namespace raftcpp {

class BaseID {
public:
    BaseID() { data_ = ""; }

    bool operator==(const BaseID &rhs) const { return (this->ToHex() == rhs.ToHex()); }

    bool operator!=(const BaseID &rhs) const { return (this->ToHex() != rhs.ToHex()); }

    std::string ToBinary() const { return data_; }

    std::string ToHex() const {
        std::string result;
        result.resize(data_.length() * 2);
        for (size_t i = 0; i < data_.size(); i++) {
            uint8_t cTemp = data_[i];
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

protected:
    std::string data_;
};

class NodeID : public BaseID {
public:
    NodeID() {}

    explicit NodeID(const Endpoint &endpoint_id) {
        data_ = "";
        data_.resize(sizeof(uint32_t) + sizeof(uint16_t));
        uint32_t inet = ip2uint(endpoint_id.GetHost());
        uint16_t port = endpoint_id.GetPort();
        memcpy(data_.data(), &inet, sizeof(uint32_t));
        memcpy(data_.data() + sizeof(uint32_t), &port, sizeof(uint16_t));
    }

    NodeID(const NodeID &nid) : BaseID(nid) { data_ = nid.data_; }

    NodeID &operator=(const NodeID &o) {
        if (this == &o) return *this;
        data_ = o.data_;
        return *this;
    }

    static NodeID FromBinary(const std::string &binary) {
        NodeID ret;
        ret.data_ = binary;
        return ret;
    }

private:
    static std::vector<std::string> explode(const std::string &s, const char &c) {
        std::string buff;
        std::vector<std::string> v;

        for (auto n : s) {
            if (n != c)
                buff += n;
            else if (n == c && !buff.empty()) {
                v.push_back(buff);
                buff = "";
            }
        }
        if (buff != "") v.push_back(buff);

        return v;
    }
    static uint32_t ip2uint(const std::string &ip) {
        std::vector<std::string> v{explode(ip, '.')};
        uint32_t result = 0;
        for (auto i = 1; i <= v.size(); i++)
            if (i < 4)
                result += (stoi(v[i - 1])) << (8 * (4 - i));
            else
                result += stoi(v[i - 1]);

        return result;
    }
};

class TermID : public BaseID {
public:
    TermID() { term_ = 0; }

    explicit TermID(int32_t term) : term_(term) {
        data_ = "";
        data_.resize(sizeof(int32_t));
        memcpy(data_.data(), &term_, sizeof(int32_t));
    }

    TermID(const TermID &tid) : BaseID(tid) {
        term_ = tid.term_;
        data_ = tid.data_;
    }

    TermID &operator=(const TermID &o) {
        if (this == &o) return *this;
        term_ = o.term_;
        data_ = o.data_;
        return *this;
    }

    int32_t getTerm() const { return term_; }

    void setTerm(int32_t term) {
        term_ = term;
        data_ = "";
        data_.resize(sizeof(int32_t));
        memcpy(data_.data(), &term_, sizeof(int32_t));
    }

    MSGPACK_DEFINE(term_);

private:
    int32_t term_;
};

}  // namespace raftcpp

namespace std {
template <>
struct hash<raftcpp::NodeID> {
    std::size_t operator()(const raftcpp::NodeID &n) const noexcept {
        return std::hash<std::string>()(n.ToBinary());
    }
};

template <>
struct hash<raftcpp::TermID> {
    std::size_t operator()(const raftcpp::TermID &t) const noexcept {
        return std::hash<std::int32_t>()(t.getTerm());
    }
};
}  // namespace std
