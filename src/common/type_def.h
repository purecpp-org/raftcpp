#pragma once

namespace raftcpp {

/**
 * The RaftState that represents the current state of a node.
 */
enum class RaftState {
    LEADER = 0,
    CANDIDATE = 1,
    FOLLOWER = 2,
};

}  // namespace raftcpp
