#pragma once

#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <chrono>

#include "common/config.h"
#include "common/randomer.h"
#include "log_manager/log_entry.h"
#include "node/node.h"
#include "rest_rpc/rpc_server.h"
#include "rpc/services.h"
#include "rwlock.h"
#include "mock_state_machine.h"

const int DEFAULT_MAX_DELAY = 3000;  // ms

using RpcServ = rest_rpc::rpc_service::rpc_server;
using RpcConn = rest_rpc::rpc_service::rpc_conn;
using RaftNode = raftcpp::node::RaftNode;
using RaftcppConstants = raftcpp::RaftcppConstants;

class NetworkConfig {
public:
    NetworkConfig(bool is_unreliable, int max_delay, std::map<std::string, int> port_to_node)
        : is_unreliable_(is_unreliable), max_delay_(max_delay), port_to_node_(port_to_node) {}

    void ReadLock() { rwlock.r_lock(); }

    void ReadUnlock() { rwlock.r_unlock(); }

    void WriteLock() { rwlock.w_lock(); }

    void WriteUnlock() { rwlock.w_unlock(); }

    bool IsUnreliable() { return is_unreliable_; }

    void SetNetReliable() { is_unreliable_ = false; }

    void SetNetUnreliable() { is_unreliable_ = true; }

    int GetMaxDelay() { return max_delay_; }

    bool IsBlocked(int idx) {
        auto iter = block_nodes_.find(idx);
        if (iter == block_nodes_.end()) {
            return false;
        }
        return true;
    }

    void BlockNodes(std::vector<int> nodes) {
        for (auto node : nodes) {
            block_nodes_.insert(node);
        }
    }

    void BlockNode(int idx) { block_nodes_.insert(idx); }

    int GetNode(std::string port) {
        auto iter = port_to_node_.find(port);
        if (iter == port_to_node_.end()) {
            return -1;
        }
        return iter->second;
    }

    int GetProbLoss() { return prob_loss_; }

    int GetProbDelay() { return prob_delay_; }

private:
    // ndoes that their rpc should be discarded
    std::set<int> block_nodes_;

    // rpc may be delayed or discarded when the network is unreliable
    bool is_unreliable_;

    // maximum delay time, ms
    int max_delay_ = 3000;

    /**
     * probability of rpc loss and delay
     * 
     * First, we get a number by random generator with range from 0 to 100,
     * then decide how to treat this rpc by the following picture.
     * 
     * ===============================================================
     * |   loss   |   delay   |                Normal                |
     * ===============================================================
     *            ^           ^ 
     * 0          10          20                                   100
     */
    int prob_loss_ = 10;
    int prob_delay_ = 20;

    std::map<std::string, int> port_to_node_;

    ReaderWriterLock rwlock;
};

class ProxyNode : public raftcpp::rpc::NodeService {
public:
    ProxyNode(std::shared_ptr<RpcServ> rpc_server, raftcpp::common::Config config,
              int peer_node, std::shared_ptr<NetworkConfig> net_cfg)
        : rpc_server_(rpc_server),
          config_(config),
          node_id_(config_.GetThisEndpoint()),
          peer_node_(peer_node),
          net_cfg_(net_cfg) {
        InitRpcHandlers();
        ConnectToOtherNodes();
    }

    void HandleRequestPreVote(RpcConn conn, const std::string &endpoint_str,
                              int32_t term_id) override {
        if (IfDiscard(conn) || MockNet()) {
            return;
        }

        auto request_pre_vote_callback = [this, conn](const boost::system::error_code &ec,
                                                        string_view data) {
            auto conn_sp = conn.lock();
            if (conn_sp) {
                const auto req_id = conn.lock()->request_id();
                std::string ret_data(data);
                conn_sp->response(req_id, ret_data);
            }
        };

        rpc_client_->async_call<0>(RaftcppConstants::REQUEST_PRE_VOTE_RPC_NAME,
                                    std::move(request_pre_vote_callback),
                                    endpoint_str,
                                    term_id);
    }

    void HandleRequestVote(RpcConn conn, const std::string &endpoint_str,
                           int32_t term_id) override {
        if (IfDiscard(conn) || MockNet()) {
            return;
        }

        auto request_vote_callback = [this, conn](const boost::system::error_code &ec,
                                                        string_view data) {
            auto conn_sp = conn.lock();
            if (conn_sp) {
                const auto req_id = conn.lock()->request_id();
                std::string ret_data(data);
                conn_sp->response(req_id, ret_data);
            }
        };

        rpc_client_->async_call<0>(RaftcppConstants::REQUEST_VOTE_RPC_NAME,
                                    std::move(request_vote_callback),
                                    endpoint_str,
                                    term_id);
    }

    void HandleRequestHeartbeat(RpcConn conn, int32_t term_id,
                                std::string node_id_binary) override {

        if (IfDiscard(conn) || MockNet()) {
            return;
        }

        auto request_heartbeat_callback = [this, conn](const boost::system::error_code &ec,
                                                        string_view data) {
            auto conn_sp = conn.lock();
            if (conn_sp) {
                const auto req_id = conn.lock()->request_id();
                std::string ret_data(data);
                conn_sp->response(req_id, ret_data);
            }
        };

        rpc_client_->async_call<0>(RaftcppConstants::REQUEST_HEARTBEAT,
                                    std::move(request_heartbeat_callback),
                                    term_id,
                                    node_id_binary);
    }

    // TODO
    void HandleRequestPullLogs(RpcConn conn, std::string node_id_binary,
                               int64_t next_log_index) override {
        if (IfDiscard(conn) || MockNet()) {
            return;
        }
    }

    // TODO
    void HandleRequestPushLogs(RpcConn conn, int64_t committed_log_index,
                               raftcpp::LogEntry log_entry) override {
        if (IfDiscard(conn) || MockNet()) {
            return;
        }
    }

private:
    /**
     * Discard or delay the rpc here.
     * @return true: discard the rpc
     */
    bool MockNet() {
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
    RET:
        return if_discard;
    }

    /**
     * check if source or destination of the rpc is blocked
     * check if destination is the source itself
     * @return true: discard the rpc
     */
    bool IfDiscard(const RpcConn &conn) {
        bool if_discard = false;
        net_cfg_->ReadLock();

        // check source of the rpc
        std::string remote_port = GetRemoteNodePort(conn);
        std::cout << "received: " << remote_port << std::endl;
        int remote_node = net_cfg_->GetNode(remote_port); // TODO bug here
        bool is_blocked = net_cfg_->IsBlocked(remote_node);

        // check destination of the rpc
        is_blocked = net_cfg_->IsBlocked(peer_node_);

        net_cfg_->ReadUnlock();

        std::cout << "remote: " << remote_node << " peer: " << peer_node_ << std::endl;
        if (remote_node == peer_node_ || is_blocked) {
            if_discard = true;
        }

        return if_discard;
    }

    std::string GetRemoteNodePort(const RpcConn &conn) {
        auto con = conn.lock();
        auto remote = con->socket().remote_endpoint();
        asio::ip::address addr = remote.address();
        return std::to_string(remote.port());
    }

    void ConnectToOtherNodes() {
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

    void InitRpcHandlers() {
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_PRE_VOTE_RPC_NAME, &ProxyNode::HandleRequestPreVote,
            this);
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_VOTE_RPC_NAME, &ProxyNode::HandleRequestVote, this);
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_HEARTBEAT, &ProxyNode::HandleRequestHeartbeat, this);
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_PULL_LOGS, &ProxyNode::HandleRequestPullLogs, this);
        rpc_server_->register_handler<rest_rpc::Async>(
            RaftcppConstants::REQUEST_PUSH_LOGS, &ProxyNode::HandleRequestPushLogs, this);
    }

    std::shared_ptr<NetworkConfig> net_cfg_;
    std::shared_ptr<RpcServ> rpc_server_;

    raftcpp::common::Config config_;
    raftcpp::NodeID node_id_;

    const int peer_node_;

    // protect the Randomer
    std::mutex mu;

    raftcpp::Randomer rand_;

    std::shared_ptr<rest_rpc::rpc_client> rpc_client_;
};

// TODO Node failure should be considered which is different from blocking nodes
/**
 * @brief Cluster manages several raft nodes and mocks a real-world network environment
 */
class Cluster {
public:
    Cluster(int node_num, bool is_unreliable = false, int max_delay = DEFAULT_MAX_DELAY)
        : node_num_(node_num) {
        std::vector<std::string> proxy_node_addr;
        std::vector<std::string> node_addr;
        std::deque<std::pair<std::string, std::string>> addr = InitAddress();

        // allocate addresses and create servers
        for (int i = 0; i < node_num_; i++) {
            auto ip_port = addr.front();
            proxy_node_addr.push_back(ip_port.first + ":" + ip_port.second);
            std::cout << "insert: " << ip_port.second << " i: " << i << std::endl;
            port_to_node_.insert(std::make_pair(ip_port.second, i));
            addr.pop_front();
            proxy_servers_.push_back(std::make_shared<RpcServ>(
                std::stoi(ip_port.second), std::thread::hardware_concurrency()));

            ip_port = addr.front();
            node_addr.push_back(ip_port.first + ":" + ip_port.second);
            addr.pop_front();
            node_servers_.push_back(std::make_shared<RpcServ>(
                std::stoi(ip_port.second), std::thread::hardware_concurrency()));
        }

        std::vector<std::string> proxy_node_cfg;
        std::vector<std::string> node_cfg;

        // init node config
        for (int i = 0; i < node_num_; i++) {
            std::vector<std::string> addrs;

            // node should connect to all proxy servers
            addrs.push_back(node_addr[i]);
            for (auto addr : proxy_node_addr) {
                addrs.push_back(addr);
            }
            node_cfg.push_back(GenerateConfig(addrs));
        }

        // init proxy node config
        for (int i = 0; i < node_num_; i++) {
            std::vector<std::string> addrs;

            // only connection to peer node should be established by the proxy node
            addrs.push_back(proxy_node_addr[i]);
            addrs.push_back(node_addr[i]);
            proxy_node_cfg.push_back(GenerateConfig(addrs));
        }
        net_cfg_ = std::make_shared<NetworkConfig>(is_unreliable, max_delay, port_to_node_);

        // create proxy nodes
        for (int i = 0; i < node_num_; i++) {
            const auto config = raftcpp::common::Config::From(proxy_node_cfg[i]);
            proxy_threads_.push_back(std::thread(Cluster::ProxyRun, this, config, i));
        }

        // create nodes
        for (int i = 0; i < node_num_; i++) {
            const auto config = raftcpp::common::Config::From(node_cfg[i]);
            node_threads_.push_back(std::thread(Cluster::NodeRun, this, config, i));
        }
    }

    ~Cluster() {
        // destroy nodes
        for (int i = 0; i < node_num_; i++) {
            nodes_[i].reset();
            node_servers_[i].reset();
            if (node_threads_[i].joinable()) {
                node_threads_[i].detach();
                node_threads_[i].std::thread::~thread();
            }
        }

        // destroy proxy nodes
        for (int i = 0; i < node_num_; i++) {
            proxy_nodes_[i].reset();
            proxy_servers_[i].reset();
            if (proxy_threads_[i].joinable()) {
                proxy_threads_[i].detach();
                proxy_threads_[i].std::thread::~thread();
            }
        }
    }

    Cluster(const Cluster &cluster) = delete;
    Cluster &operator=(const Cluster &cluster) = delete;

    std::vector<int> GetLeader() {
        std::vector<int> leader;
        for (int i = 0; i < node_num_; i++) {
            std::cout << "node: " << i << " cur state: " << static_cast<int>(nodes_[i]->GetCurrState()) << std::endl;
            if (nodes_[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
                leader.push_back(i);
            }
        }
        return leader;
    }

    bool CheckOneLeader() {
        std::vector<int> leaders = GetLeader();
        std::cout << "l size: " << leaders.size() << std::endl;
        if (leaders.size() == 1) {
            return true;
        }
        return false;
    }

    raftcpp::RaftState GetNodeState(int idx) { return nodes_[idx]->GetCurrState(); }

    // TODO it's different from blocking the node
    void ShutDown(int idx) {}

    // TODO
    void Start(int idx) {}

    void BlockNode(int idx) {
        net_cfg_->WriteLock();
        net_cfg_->BlockNode(idx);
        net_cfg_->WriteUnlock();
    }

    void BlockNodes(std::vector<int> nodes) {
        net_cfg_->WriteLock();
        net_cfg_->BlockNodes(nodes);
        net_cfg_->WriteUnlock();
    }

    int GetNodeNum() { return node_num_; }

    void SetNetUnreliable() {
        net_cfg_->WriteLock();
        net_cfg_->SetNetUnreliable();
        net_cfg_->WriteUnlock();
    }

    void SetNetReliable() {
        net_cfg_->WriteLock();
        net_cfg_->SetNetReliable();
        net_cfg_->WriteUnlock();
    }

private:
    std::deque<std::pair<std::string, std::string>> InitAddress() {
        std::string ip("127.0.0.1");
        std::deque<std::pair<std::string, std::string>> addr;

        for (int i = 0; i < node_num_ * 2; i++) {
            addr.push_back(std::make_pair(ip, std::to_string(BASE_PORT + i)));
        }

        return addr;
    }

    std::string GenerateConfig(const std::vector<std::string> &addr) {
        std::string config;
        int size = addr.size();
        for (int i = 0; i < size; i++) {
            config += addr[i];
            if (i < size - 1) {
                config += ",";
            }
        }
        return config;
    }

    // proxy node's index is equal to the peer_node
    static void ProxyRun(Cluster *self, raftcpp::common::Config config, int peer_node) {
        self->mu.lock();
        self->proxy_nodes_.push_back(std::make_shared<ProxyNode>(self->proxy_servers_[peer_node], config, peer_node, self->net_cfg_));
        self->mu.unlock();
        self->proxy_servers_[peer_node]->run();
    }

    static void NodeRun(Cluster *self, raftcpp::common::Config config, int idx) {
        self->mu.lock();
        self->nodes_.push_back(std::make_shared<raftcpp::node::RaftNode>(std::make_shared<MockStateMachine>(),
                                                                            *(self->node_servers_[idx]),
                                                                            config,
                                                                            raftcpp::RaftcppLogLevel::RLL_DEBUG));
        self->mu.unlock();
        self->node_servers_[idx]->run();
    }

    int node_num_;

    // protect vectors when initialization
    std::mutex mu;

    std::vector<std::shared_ptr<raftcpp::node::RaftNode>> nodes_;
    std::vector<std::shared_ptr<RpcServ>> node_servers_;
    std::vector<std::thread> node_threads_;

    // help the proxy node find the rpc caller node
    std::map<std::string, int> port_to_node_;

    std::vector<std::shared_ptr<ProxyNode>> proxy_nodes_;
    std::vector<std::shared_ptr<RpcServ>> proxy_servers_;
    std::vector<std::thread> proxy_threads_;

    std::shared_ptr<NetworkConfig> net_cfg_;


    // server's port is assigned according to this BASE_PORT
    const int BASE_PORT = 10000;
};
