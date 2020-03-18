#include "raft_node.hpp"

int main() {
    using namespace raftcpp;
    raft_config conf{};
    conf.all_peers = { 
        {"127.0.0.1", 8001, 0},
        {"127.0.0.1", 8002, 1}, 
        {"127.0.0.1", 8003, 2},
    };

    std::cout << "all the peers: \n";
    for (auto& addr : conf.all_peers) {
        std::cout << "id: " << addr.id << ", ";
        std::cout << "address: " << addr.ip << " "<<addr.port<<"\r\n";
    }
    std::cout << "please choose an id: ";

    std::string str;
    std::cin >> str;
    int select = atoi(str.data());
    if (select > conf.all_peers.size() || select < 0) {
        std::cout << "please choose a correct id: ";
        return -1;
    }

    std::cout << "has chose id: " << std::to_string(select) << "\n";
    std::cout << "wait input command: \n";

    conf.self_addr = conf.all_peers[select];
    
    raft_node node(conf);
    node.async_run();
    
    std::string debug_str;
    while (true) {
        std::cin >> debug_str;
        if (debug_str == "prevote") {
            node.vote(true);
        }
        else if(debug_str == "vote"){
            node.vote(false);
        }
        else {
            std::cout << "bad command line\n";
        }
    }
}
