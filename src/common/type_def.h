#pragma once

namespace raftcpp {

/**
 * The RaftState that represents the current state of a node.
 */
enum class RaftState {
    LEADER = 0,
    PRECANDIDATE = 1,
    CANDIDATE = 2,
    FOLLOWER = 3,
};

}  // namespace raftcpp
