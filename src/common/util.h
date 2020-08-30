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

}  // namespace common
}  // namespace raftcpp
