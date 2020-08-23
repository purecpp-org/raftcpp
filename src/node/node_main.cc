#include "raft_node.h"
#include <gflags/gflags.h>

using namespace raftcpp;
using namespace raftcpp::node;

DEFINE_string(address, "127.0.0.1", "What address to listen on");
DEFINE_int32(port, 8080, "What port to listen on");
DEFINE_string(state_string, "follower", "What the initial state the node is");

int main(int argc, char *argv[]) {
      
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    
    RaftState state;

    if (FLAGS_state_string == "leader") {
        state = RaftState::LEADER;
    } else if (FLAGS_state_string == "follower") {
        state = RaftState::FOLLOWER;
    } else {
        raftcpp::node::ShowUsage();
        exit(EXIT_FAILURE);
    }

    raftcpp::node::RaftNode node{FLAGS_address, FLAGS_port, state};
    gflags::ShutDownCommandLineFlags();
    node.start();

    return 0;
}
