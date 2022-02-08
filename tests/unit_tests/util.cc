#include "util.h"

    void ProxyNode::HandleRequestPreVote(RpcConn conn, const std::string &endpoint_str,
                              int32_t term_id) {
        if (IfDiscard(GetPortFromEndPoint(endpoint_str)) || MockNet()) {
            return;
        }

        auto self = shared_from_this();

        auto request_pre_vote_callback = [self, conn](const boost::system::error_code &ec,
                                                      string_view data) {
            auto conn_sp = conn.lock();
            if (conn_sp) {
                const auto req_id = conn.lock()->request_id();
                std::string ret_data(data);
                conn_sp->response(req_id, ret_data);
            }
        };

        rpc_client_->async_call<0>(RaftcppConstants::REQUEST_PRE_VOTE_RPC_NAME,
                                   std::move(request_pre_vote_callback), endpoint_str,
                                   term_id);
    }

    void ProxyNode::HandleRequestVote(RpcConn conn, const std::string &endpoint_str,
                           int32_t term_id) {
        if (IfDiscard(GetPortFromEndPoint(endpoint_str)) || MockNet()) {
            return;
        }

        auto self = shared_from_this();

        auto request_vote_callback = [self, conn](const boost::system::error_code &ec,
                                                  string_view data) {
            auto conn_sp = conn.lock();
            if (conn_sp) {
                const auto req_id = conn.lock()->request_id();
                std::string ret_data(data);
                conn_sp->response(req_id, ret_data);
            }
        };

        rpc_client_->async_call<0>(RaftcppConstants::REQUEST_VOTE_RPC_NAME,
                                   std::move(request_vote_callback), endpoint_str,
                                   term_id);
    }

    void ProxyNode::HandleRequestHeartbeat(RpcConn conn, int32_t term_id,
                                std::string node_id_binary) {
        if (IfDiscard(GetPortFromBinary(node_id_binary)) || MockNet()) {
            return;
        }

        auto self = shared_from_this();

        auto request_heartbeat_callback =
            [self, conn](const boost::system::error_code &ec, string_view data) {
                auto conn_sp = conn.lock();
                if (conn_sp) {
                    const auto req_id = conn.lock()->request_id();
                    std::string ret_data(data);
                    conn_sp->response(req_id, ret_data);
                }
            };

        rpc_client_->async_call<0>(RaftcppConstants::REQUEST_HEARTBEAT,
                                   std::move(request_heartbeat_callback), term_id,
                                   node_id_binary);
    }

    uint16_t ProxyNode::GetPortFromBinary(const std::string &binary_addr) {
        uint16_t port;

        /*
         * size in the second and third parameters should be
         * the same with the explicit constructor in NodeID.
         */
        memcpy(&port, binary_addr.data() + sizeof(uint32_t), sizeof(uint16_t));

        return port;
    }

    uint16_t ProxyNode::GetPortFromEndPoint(const std::string &endpoint) {
        int idx = 0;

        // jump to the beginning of the port
        while (endpoint[idx] != ':') idx++;
        idx++;

        uint16_t port = 0;
        while (idx < endpoint.size()) {
            port *= 10;
            port += endpoint[idx] - 48;
            idx++;
        }

        return port;
    }

    bool ProxyNode::MockNet() {
        net_cfg_->ReadLock();
        bool if_discard = false;
        bool is_unreliable = net_cfg_->IsUnreliable();

        if (is_unreliable) {
            // mock the unreliable network
            int prob_loss = net_cfg_->GetProbLoss();
            int prob_delay = net_cfg_->GetProbDelay();
            int rand_num = rand_.TakeOne(0, 100);

            if ((0 <= rand_num) && (rand_num <= prob_loss)) {
                // discard the rpc
                if_discard = true;
            } else if ((prob_loss < rand_num) && (rand_num <= prob_delay)) {
                int sleep_time = rand_.TakeOne(0, net_cfg_->GetMaxDelay());
                net_cfg_->ReadUnlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
                goto RET;
            }
        }

        net_cfg_->ReadUnlock();
        /// TODO(qwang): Remove this goto
    RET:
        return if_discard;
    }

    bool ProxyNode::IfDiscard(uint16_t source_port) {
        bool if_discard = false;
        net_cfg_->ReadLock();

        // check source of the rpc
        int remote_node = net_cfg_->GetNode(std::to_string(source_port));
        bool source_is_blocked = net_cfg_->IsBlocked(remote_node);
        if (source_is_blocked) {
        }

        // check destination of the rpc
        bool dst_is_blocked = net_cfg_->IsBlocked(peer_node_);
        if (dst_is_blocked) {
        }

        net_cfg_->ReadUnlock();

        if (remote_node == peer_node_ || source_is_blocked || dst_is_blocked) {
            if_discard = true;
        }

        return if_discard;
    }

    std::string ProxyNode::GetRemoteNodePort(const RpcConn &conn) {
        auto con = conn.lock();
        auto remote = con->socket().remote_endpoint();
        asio::ip::address addr = remote.address();
        return std::to_string(remote.port());
    }

    void ProxyNode::ConnectToOtherNodes() {
        // only one connection should be established
        for (const auto &endpoint : config_.GetOtherEndpoints()) {
            rpc_client_ = std::make_shared<rest_rpc::rpc_client>(endpoint.GetHost(),
                                                                 endpoint.GetPort());
            bool connected = rpc_client_->connect();
            if (!connected) {
                fprintf(stderr, "proxy node connects to peer fail\n");
                exit(-1);
            }
            rpc_client_->enable_auto_reconnect();
        }
    }

    void ProxyNode::InitRpcHandlers() {
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_PRE_VOTE_RPC_NAME, &ProxyNode::HandleRequestPreVote,
            this);
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_VOTE_RPC_NAME, &ProxyNode::HandleRequestVote, this);
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_HEARTBEAT, &ProxyNode::HandleRequestHeartbeat,
            this);
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_PUSH_LOGS, &ProxyNode::HandleRequestPushLogs, this);
    }