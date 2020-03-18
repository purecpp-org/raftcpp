#pragma once
#include <vector>
#include <map>
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
            debug();
        }

        void run() {
            raft_server_.run();
        }

        void async_run() {
            raft_server_.async_run();
        }

    //private:
        void debug() {
            std::thread thd([this] {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    for (auto& [id, client] : raft_clients_) {
                        if (client->has_connected()) {
                            std::cout << "connected, id= " << id << "\n";
                            return;
                        }
                    }
                }
                
            });
            thd.detach();
        }

        void init() {
            for (auto& addr : conf_.all_peers) {
                if (addr.id == conf_.self_addr.id)
                    continue;

                auto client = std::make_shared<rpc_client>();
                client->enable_auto_reconnect();
                client->enable_auto_heartbeat();
                client->async_connect(addr.ip, addr.port);
                raft_clients_.emplace(addr.id, std::move(client));
            }

            raft_server_.register_handler("prevote", [this](rpc_conn conn, vote_req req) {
                return handle_vote_request(std::move(req));
            });
        }

        void vote(bool prevote) {
            vote_req request{};
            request.pre_vote = prevote;
            request.src = server_id_;
            request.term = curr_term_ + 1;
            request.last_log_index = log_store_.last_log_index();
            request.last_log_term = log_store_.last_log_term();

            for (auto& [id, client] : raft_clients_) {
                if (!client->has_connected())
                    continue;

                request.dst = id;
                client->async_call("prevote", [this](const auto& ec, string_view data) {
                    if (ec) {
                        std::cout << ec.value() << ", " << ec.message() << "\n";
                        return;
                    }

                    vote_resp resp = as<vote_resp>(data);
                    handle_vote_response(std::move(resp));
                }, request);
            }
        }

        vote_resp handle_vote_request(vote_req req) {
            if (req.pre_vote) {
                std::cout << "get pre_vote request\n";
            }
            else {
                std::cout << "get vote request\n";
            }
            return {};
        }

        void handle_vote_response(vote_resp resp) {
            std::cout << "granted result: " << (resp.granted ? "yes" : "no") << "\n";
        }

        //connections
        raft_config conf_;
        rpc_server raft_server_;
        int server_id_;
        std::map<int, std::shared_ptr<rpc_client>> raft_clients_;        

        //raft business
        State state_;
        int64_t curr_term_ = 0;
        int leader_id_;
        int voted_id_;
        memory_log_store log_store_;
        //timer
    };
}
