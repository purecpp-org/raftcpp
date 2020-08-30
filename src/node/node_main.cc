#include <gflags/gflags.h>

#include <chrono>
#include <thread>

#include "node.h"

using namespace raftcpp;
using namespace raftcpp::node;

DEFINE_string(address, "127.0.0.1", "What address to listen on");
DEFINE_int32(port, 8080, "What port to listen on");
DEFINE_string(state_string, "follower", "What the initial state the node is");

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    raftcpp::node::RaftNode node{FLAGS_address, FLAGS_port};
    gflags::ShutDownCommandLineFlags();

    std::this_thread::sleep_for(std::chrono::milliseconds(10 * 1000));

    return 0;
}
