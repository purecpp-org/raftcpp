#pragma once

#include <string>

namespace raftcpp {
namespace common {

class Range final {
public:
    Range(uint64_t begin, uint64_t end)
       : begin_(begin), end_(end), cache_delta_(end_ - begin_) {}

    Range(const Range &other)
        : Range(other.begin_, other.end_) {}

    uint64_t GetBegin() const {return begin_;}

    uint64_t GetEnd() const {return end_;}

    uint64_t GetDelta() const {return cache_delta_;}

private:
    uint64_t begin_;

    uint64_t end_;

    uint64_t cache_delta_;
};

} // namespace common
} // namespace raftcpp
