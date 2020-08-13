#include "raft_node.hpp"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        raftcpp::node::ShowUsage();
        exit(EXIT_FAILURE);
    }

    std::string address = argv[1];
    int port = std::atoi(argv[2]);
    std::string state_string(argv[3]);
    raftcpp::node::State state;

    if (state_string == "leader") {
        state = raftcpp::node::Leader;
    } else if (state_string == "follower") {
        state = raftcpp::node::Follower;
    } else {
        raftcpp::node::ShowUsage();
        exit(EXIT_FAILURE);
    }

    raftcpp::node::RaftNode node{address, port, state};
    node.start();
}
