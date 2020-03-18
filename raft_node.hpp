#pragma once
#include <vector>
#include <include/rpc_server.h>
#include <include/rpc_client.hpp>
#include "entity.h"
#include "node_config.h"
#include "memory_log_store.hpp"

namespace raftcpp {
    using namespace rest_rpc;
    using namespace rpc_service;

    class raft_node final {
    public:
        raft_node(raft_config conf) : 
            raft_server_(conf.self_addr.port, std::thread::hardware_concurrency()),
            conf_(std::move(conf)), server_id_(conf.self_addr.id)
        {
            init();
        }

    private:
        void init() {
            for (auto& addr : conf_.all_peers) {
                if (addr.id == conf_.self_addr.id)
                    continue;

                auto client = std::make_shared<rpc_client>();
                client->enable_auto_reconnect();
                client->enable_auto_heartbeat();
                client->async_connect(addr.ip, addr.port);
                raft_clients_.push_back(std::move(client));
            }
        }

        void prevote() {

        }

        void handle_prevote() {

        }

        //connections
        raft_config conf_;
        rpc_server raft_server_;
        int server_id_;
        std::vector<std::shared_ptr<rpc_client>> raft_clients_;        

        //raft business
        State state_;
        int64_t curr_term_ = 0;
        int leader_id_;
        int voted_id_;
        memory_log_store log_store_;
        //timer
    };
}
