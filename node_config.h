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
        size_t election_timeout_milli = 2000; //upper 4000
        size_t heartbeat_interval = 1000;
        size_t req_timeout_milli = 3000;
    };
}