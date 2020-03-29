#pragma once
#include <vector>
#include <map>
#include <random>
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
            work_(ios_), election_timer_(ios_), vote_timer_(ios_), heratbeat_timer_(ios_){
            init();
        }

        ~raft_node() {
            for (auto& [id, client] : raft_clients_) {
                client->stop();
            }

            election_timer_.cancel();
            vote_timer_.cancel();
            ios_.stop();
            timer_thd_.join();
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
                print("recieved prevote request from", req.src);
                return handle_prevote_request(std::move(req));
            });
            raft_server_.register_handler("vote", [this](rpc_conn conn, vote_req req) {
                print("recieved vote request from", req.src);
                return handle_vote_request(std::move(req));
            });
            raft_server_.register_handler("heartbeat", [this](rpc_conn conn, append_entries_req req) {
                return handle_heartbeat_request(std::move(req));
            });
            raft_server_.register_handler("append_entries", [this](rpc_conn conn, append_entries_req req) {
                return handle_append_entries_request(std::move(req));
            });

            timer_thd_ = std::thread([this] { ios_.run(); });

            reset_election_timer(get_random_milli());
        }

        void reset_election_timer(size_t timeout) {
#ifdef DOCTEST_TEST_CASE
            if (conf_.disable_election_timer)
                return;
#endif

            reset_timer(true, timeout);
        }

        void reset_vote_timer(size_t timeout) {
            reset_timer(false, timeout);
        }

        void start_heartbeat_timer() {
            heratbeat_timer_.expires_from_now(std::chrono::milliseconds(conf_.heartbeat_interval));
            heratbeat_timer_.async_wait([this](boost::system::error_code ec) {
                if (ec) {
                    return;
                }

                send_heartbeat();
                start_heartbeat_timer();
            });
        }

        State state() const {
            return state_;
        }

        int64_t current_term() const {
            return curr_term_;
        }

#ifdef _DEBUG
        std::mutex test_mtx_;
        template<typename... Args>
        void print(Args... args) {            
            auto n = std::chrono::system_clock::now();

            auto m = n.time_since_epoch();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(m).count();
            auto const msecs = diff % 1000;

            std::time_t t = std::chrono::system_clock::to_time_t(n);
            std::unique_lock lock(test_mtx_);
            std::cout << "[id " << conf_.self_addr.id << " "<< 
                std::put_time(std::localtime(&t), "%Y-%m-%d %H.%M.%S") << "." << msecs <<"] ";
            ((std::cout << args << ' '), ...);
            std::cout << "\n";
        }

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

        void request_prevote() {
            request_vote(true);
        }

        void request_vote() {
            request_vote(false);
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
                    print("get quorum prevote, become candidate");
                    state_ = State::CANDIDATE;
                    request_vote();
                    return;
                }
            }

            reset_election_timer(get_random_milli());
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
                    print("get quorum vote, become leader");
                    become_leader();
                    return;
                }                
            }
            
            reset_vote_timer(get_random_milli());
        }

        void become_leader() {
            vote_timer_.cancel();
            state_ = State::LEADER;
            leader_id_ = server_id_;
            send_heartbeat();
            start_heartbeat_timer();
        }

        void send_heartbeat() {
            append_entries_req request{};
            request.src = server_id_;
            for (auto&[id, client] : raft_clients_) {
                if (!client->has_connected())
                    continue;

                print("send heartbeat to", id);

                request.dst = id;
                client->async_call("heartbeat", [this](const auto& ec, string_view data) {
                    if (ec) {
                        std::cout << ec.value() << ", " << ec.message() << "\n";
                        return;
                    }
                    
                    append_entries_resp resp = as<append_entries_resp>(data);
                    handle_heartbeat_response(resp);
                }, request);
            }
        }

        void handle_heartbeat_response(const append_entries_resp& resp) {

        }

        void handle_append_entries_response(const append_entries_resp& resp) {

        }

        void stepdown(int64_t term) {
            print("stepdown");
            if (state_ == State::CANDIDATE) {
                vote_timer_.cancel();
            }
            else if (state_ == State::LEADER) {
                heratbeat_timer_.cancel();
            }

            leader_id_ = -1;
            state_ = State::FOLLOWER;

            if (term > curr_term_) {
                curr_term_ = term;
                voted_id_ = -1;
            }

            reset_election_timer(get_random_milli());
        }

        vote_resp handle_prevote_request(const vote_req& req) {            
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

        vote_resp handle_vote_request(const vote_req& req) {
            bool granted = false;
            do {
                if (req.term < curr_term_) {
                    break;
                }

                if (req.term > curr_term_) {
                    stepdown(req.term);
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
            
            if (granted&&voted_id_ == -1) {
                voted_id_ = req.src;
            }

            return { curr_term_, granted };
        }

        append_entries_resp handle_heartbeat_request(const append_entries_req& req) {
            print("recieve heartbeat from", req.src);
            reset_election_timer(get_random_milli());

            if (req.term < curr_term_) {
                return { curr_term_, false };
            }

            return { req.term, true, log_store_.last_log_index() };
        }

        append_entries_resp handle_append_entries_request(const append_entries_req& req) {
            if (req.term < curr_term_) {
                return { curr_term_, false };
            }

            //TODO
            if (req.src != server_id_) {
                stepdown(req.term + 1);
                return { req.term + 1, false };
            }

            return { req.term, true, log_store_.last_log_index() };
        }

        void request_vote(bool prevote) {
            vote_req request{};
            request.src = conf_.self_addr.id;

            if (prevote) {
                request.term = curr_term_ + 1;
                prevote_ack_num_ = 1;
            }
            else {
                voted_id_ = server_id_;
                curr_term_++;
                request.term = curr_term_;
                vote_ack_num_ = 1;

                //start vote timer
                reset_vote_timer(get_random_milli());
            }

            request.last_log_index = log_store_.last_log_index();
            request.last_log_term = log_store_.last_log_term();

            std::string service_name = prevote ? "prevote" : "vote";
            
            for (auto&[id, client] : raft_clients_) {
                if (!client->has_connected())
                    continue;

                request.dst = id;

                print("send", service_name, "to", request.dst);
                client->async_call(service_name, [this, prevote](const auto& ec, string_view data) {
                    if (ec) {
                        std::cout << ec.value() << ", " << ec.message() << "\n";
                        return;
                    }

                    vote_resp resp = as<vote_resp>(data);
                    prevote ? handle_prevote_response(resp) : handle_vote_response(resp);
                }, request);
            }
        }

        void reset_timer(bool prevote, size_t timeout) {
            auto& timer = prevote ? election_timer_ : vote_timer_;
            timer.expires_from_now(std::chrono::milliseconds(timeout));
            timer.async_wait([this, prevote](boost::system::error_code ec) {
                if (ec) {
                    return;
                }

                prevote ? request_prevote() : request_vote();
            });
        }

        size_t get_random_milli() {
            return random(conf_.election_timeout_milli, conf_.election_timeout_milli * 2);
        }

        size_t random(size_t min, size_t max) {
            static std::default_random_engine random_engine{};
            using Dist = std::uniform_int_distribution<size_t>;
            static Dist number{};
            return number(random_engine, Dist::param_type{ min, max });
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
        std::thread timer_thd_;
        asio::io_service ios_;
        asio::io_service::work work_;
        
        asio::steady_timer election_timer_;
        asio::steady_timer vote_timer_;
        asio::steady_timer heratbeat_timer_;
    };
}
