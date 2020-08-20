#include "raft_node.h"

using namespace raftcpp;
using namespace raftcpp::node;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        raftcpp::node::ShowUsage();
        exit(EXIT_FAILURE);
    }

    std::string address = argv[1];
    int port = std::atoi(argv[2]);
    std::string state_string(argv[3]);
    RaftState state;

    if (state_string == "leader") {
        state = RaftState::LEADER;
    } else if (state_string == "follower") {
        state = RaftState::FOLLOWER;
    } else {
        raftcpp::node::ShowUsage();
        exit(EXIT_FAILURE);
    }

    raftcpp::node::RaftNode node{address, port, state};
    node.start();

    return 0;
}
