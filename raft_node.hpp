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
        }

        void run() {
            raft_server_.run();
        }

        void async_run() {
            raft_server_.async_run();
        }

    //private:
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
            raft_server_.register_handler("vote", [this](rpc_conn conn, vote_req req) {
                return handle_vote_request(std::move(req));
            });
        }

        State state() const {
            return state_;
        }

        int64_t current_term() const {
            return curr_term_;
        }

        void request_vote(bool prevote) {
            vote_req request{};
            request.pre_vote = prevote;
            request.src = server_id_;

            if (prevote) {
                request.term = curr_term_ + 1;
                prevote_ack_num_ = 0;
            }
            else {
                curr_term_++;
                request.term = curr_term_;
                vote_ack_num_ = 0;
            }
            
            request.last_log_index = log_store_.last_log_index();
            request.last_log_term = log_store_.last_log_term();

            std::string service_name = prevote ? "prevote" : "vote";
            for (auto& [id, client] : raft_clients_) {
                if (!client->has_connected())
                    continue;

                request.dst = id;
                client->async_call(service_name, [this, prevote](const auto& ec, string_view data) {
                    if (ec) {
                        std::cout << ec.value() << ", " << ec.message() << "\n";
                        return;
                    }

                    vote_resp resp = as<vote_resp>(data);
                    prevote ? handle_prevote_response(std::move(resp)) : handle_vote_response(std::move(resp));
                }, request);
            }
        }

        void handle_prevote_response(vote_resp resp) {
            if (state_ != State::FOLLOWER) {
                return;
            }

            if (resp.term > curr_term_) {
                stepdown(resp.term);
                return;
            }

            if (resp.granted) {
                prevote_ack_num_++;
                //get quorum
                if (prevote_ack_num_ > conf_.all_peers.size() / 2) {
                    //elect_self();
                }
            }
        }

        void handle_vote_response(vote_resp resp) {
            if (state_ != State::CANDIDATE) {
                return;
            }

            if (resp.term > curr_term_) {
                stepdown(resp.term);
                return;
            }

            if (resp.granted) {
                vote_ack_num_++;
                //get quorum
                if (vote_ack_num_ > conf_.all_peers.size() / 2) {
                    //become_leader();
                }
            }
        }

        void stepdown(int64_t term) {
            if (state_ == State::CANDIDATE) {
                //TODO stop vote timer
            }

            leader_id_ = -1;
            state_ = State::FOLLOWER;

            if (term > curr_term_) {
                curr_term_ = term;
                voted_id_ = -1;
            }

            //TODO:election timer start
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

        //connections
        raft_config conf_;
        rpc_server raft_server_;
        int server_id_ = -1;
        std::map<int, std::shared_ptr<rpc_client>> raft_clients_;        

        //raft business
        State state_ = State::FOLLOWER;
        std::atomic<int64_t> curr_term_ = 0;
        std::atomic<int> leader_id_ = -1;
        std::atomic<int> voted_id_ = -1;
        memory_log_store log_store_;

        std::atomic<int> prevote_ack_num_ = 0;
        std::atomic<int> vote_ack_num_ = 0;
        //timer
    };
}
