#pragma once
#include <string>
#include <vector>

namespace raftcpp {
    struct address {
        std::string ip;
        int port;
        int id;
    };

    struct raft_config {
        address self_addr;
        std::vector<address> all_peers;
    };
}