#pragma once

#include <chrono>

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
    srand(CurrentTimeMs());
    return (rand() % (end - begin)) + begin;
}

}  // namespace common
}  // namespace raftcpp
