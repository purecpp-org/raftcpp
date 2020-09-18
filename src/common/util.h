#pragma once

#include <chrono>
#include <random>

namespace raftcpp {
namespace common {

/**
 * Return the current timestamp in milliseconds in a steady clock.
 */
inline int64_t CurrentTimeMs() {
    std::chrono::milliseconds ms_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    return ms_since_epoch.count();
}

inline int64_t CurrentTimeUs() {
    std::chrono::microseconds us_since_epoch =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    return us_since_epoch.count();
}

inline uint64_t RandomNumber(const uint64_t begin, const uint64_t end) {
    std::default_random_engine e(CurrentTimeUs());
    std::uniform_int_distribution<uint64_t> u(begin, end);
    return u(e);
}

}  // namespace common
}  // namespace raftcpp
