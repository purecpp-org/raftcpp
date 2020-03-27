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
            conf_(std::move(conf)), server_id_(conf.self_addr.id),
            work_(ios_), election_timer_(ios_), vote_timer_(ios_){
            init();
        }

        ~raft_node() {
            election_timer_.cancel();
            vote_timer_.cancel();
            ios_.stop();
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
                return handle_prevote_request(std::move(req));
            });
            raft_server_.register_handler("vote", [this](rpc_conn conn, vote_req req) {
                return handle_vote_request(std::move(req));
            });

            std::thread thd([this] { ios_.run(); });
            thd.detach();
        }

        void reset_election_timer(size_t timeout) {
            reset_timer(true, timeout);
        }

        void reset_vote_timer(size_t timeout) {
            reset_timer(false, timeout);
        }

        void reset_timer(bool prevote, size_t timeout) {
            auto& timer = prevote ? election_timer_ : vote_timer_;
            timer.expires_from_now(std::chrono::milliseconds(timeout));
            timer.async_wait([this](boost::system::error_code ec) {
                if (ec) {
                    return;
                }

                request_vote(false);
            });
        }

        State state() const {
            return state_;
        }

        int64_t current_term() const {
            return curr_term_;
        }

#ifdef _DEBUG
        //void set_prevote_ack_num(int num) {
        //    prevote_ack_num_ = num;
        //}

        void set_vote_ack_num(int num) {
            vote_ack_num_ = num;
        }

        void set_current_term(int64_t term) {
            curr_term_ = term;
        }

        void append(log_entry entry) {
            log_store_.append_entry(entry);
        }
#endif

        void request_vote(bool prevote) {
            vote_req request{};
            request.src = server_id_;

            if (prevote) {
                request.term = curr_term_ + 1;
                prevote_ack_num_ = 1;
            }
            else {
                curr_term_++;
                request.term = curr_term_;
                vote_ack_num_ = 1;
            }

            request.last_log_index = log_store_.last_log_index();
            request.last_log_term = log_store_.last_log_term();

            std::string service_name = prevote ? "prevote" : "vote";
            for (auto&[id, client] : raft_clients_) {
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
                    state_ = State::CANDIDATE;
                    request_vote(false);
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
                    state_ = State::LEADER;
                    //become_leader();
                }
            }
        }

        void stepdown(int64_t term) {
            if (state_ == State::CANDIDATE) {
                //TODO stop vote timer
                vote_timer_.cancel();
            }

            leader_id_ = -1;
            state_ = State::FOLLOWER;

            if (term > curr_term_) {
                curr_term_ = term;
                voted_id_ = -1;
            }

            //TODO:election timer restart
            reset_election_timer(get_random_milli());
        }

        size_t get_random_milli() {
            return 0;//TODO
        }

        vote_resp handle_prevote_request(vote_req req) {
            bool granted = false;
            do {
                if (req.term < curr_term_) {
                    break;
                }

                auto last_log_term = log_store_.last_log_term();
                if (req.last_log_term < last_log_term) {
                    break;
                }

                if (req.last_log_term > last_log_term) {
                    granted = true;
                    break;
                }

                if (req.last_log_index >= log_store_.last_log_index()) {
                    granted = true;
                }
            } while (false);

            return { curr_term_, granted };
        }

        vote_resp handle_vote_request(vote_req req) {
            bool granted = false;
            do {
                if (req.term < curr_term_) {
                    break;
                }

                if (req.term > curr_term_) {
                    stepdown(req.term);
                }

                if (req.last_log_term < curr_term_) {
                    break;
                }

                if (req.last_log_term > curr_term_) {
                    granted = true;
                    break;
                }

                if (req.last_log_index >= log_store_.last_log_index()) {
                    granted = true;
                }
            } while (false);
            
            return { curr_term_, granted };
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

        std::atomic<int> prevote_ack_num_ = 1;
        std::atomic<int> vote_ack_num_ = 1;
        //timer
        asio::io_service ios_;
        asio::io_service::work work_;
        
        asio::steady_timer election_timer_;
        asio::steady_timer vote_timer_;        
    };
}
