#pragma once

#include <iostream>
#include <string>
#include "rpc_client.hpp"
#include "rpc_server.h"

namespace raftcpp {
    namespace node {

        enum State {
            Leader,
            Follower,
            Candidate
        };

        bool heartbeat(rpc_conn conn) {
            std::cout << "receive heartbeat" << std::endl;
            return true;
        }

        void ShowUsage() {
            std::cerr << "Usage: <address> <port> <role> [leader follower]" << std::endl;
        }

        class RaftNode {
        public:
            explicit RaftNode(const std::string &address, const int &port, const State &state)
                    : address_{address},
                      port_{port},
                      state_{state} {
                switch (state) {
                    case Leader:
                        server = std::make_unique<rest_rpc::rpc_service::rpc_server>(port_,
                                                                                     std::thread::hardware_concurrency());
                        server->register_handler("heartbeat", heartbeat);
                        break;
                    case Follower:
                        client = std::make_unique<rest_rpc::rpc_client>(address_, port_);
                        {
                            bool r = client->connect();
                            if (!r) {
                                std::cout << "connect timeout" << std::endl;
                            }
                        }
                        break;
                    default:
                        throw std::runtime_error("error state parameter");
                }
            }

            void start() {
                if (state_ == Leader) {
                    server->run();
                } else {
                    try {
                        bool result = client->call<bool>("heartbeat");
                        std::cout << "heartbeat result: " << std::boolalpha << result << std::endl;
                    }
                    catch (const std::exception &e) {
                        std::cout << e.what() << std::endl;
                    }
                    while (1);
                }
            };
        private:
            const std::string address_;
            const int port_;
            const State state_;
            std::unique_ptr<rest_rpc::rpc_service::rpc_server> server;
            std::unique_ptr<rest_rpc::rpc_client> client;
        };

    }
}
