#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <vector>

#include "common/config.h"
#include "common/randomer.h"
#include "log_manager/log_entry.h"
#include "mock_state_machine.h"
#include "node/node.h"
#include "rest_rpc/rpc_server.h"
#include "rpc/services.h"

const int DEFAULT_MAX_DELAY_MS = 3000;  // ms

using RpcServ = rest_rpc::rpc_service::rpc_server;
using RpcConn = rest_rpc::rpc_service::rpc_conn;
using RaftNode = raftcpp::node::RaftNode;
using RaftcppConstants = raftcpp::RaftcppConstants;

class NetworkConfig {
public:
    NetworkConfig(bool is_unreliable, int max_delay,
                  std::map<std::string, int> port_to_node)
        : is_unreliable_(is_unreliable),
          max_delay_(max_delay),
          port_to_node_(port_to_node) {}

    void ReadLock() { rwlock.lock_shared(); }

    void ReadUnlock() { rwlock.unlock_shared(); }

    void WriteLock() { rwlock.lock(); }

    void WriteUnlock() { rwlock.unlock(); }

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

    void UnblockNodes(std::vector<int> nodes) {
        for (auto node : nodes) {
            auto iter = block_nodes_.find(node);
            if (iter != block_nodes_.end()) {
                block_nodes_.erase(iter);
            }
        }
    }

    void UnblockNode(int idx) {
        auto iter = block_nodes_.find(idx);
        if (iter != block_nodes_.end()) {
            block_nodes_.erase(iter);
        }
    }

    int GetNode(std::string port) {
        auto iter = port_to_node_.find(port);
        if (iter == port_to_node_.end()) {
            return -1;
        }
        return iter->second;
    }

    int GetProbLoss() { return prob_loss_; }

    int GetProbDelay() { return prob_delay_; }

    std::set<int> GetBlockedNodes() { return block_nodes_; }

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

    std::shared_mutex rwlock;
};

class ProxyNode : public raftcpp::rpc::NodeService,
                  public std::enable_shared_from_this<ProxyNode> {
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
                              int32_t term_id) override;

    void HandleRequestVote(RpcConn conn, const std::string &endpoint_str,
                           int32_t term_id) override;

    void HandleRequestHeartbeat(RpcConn conn, int32_t term_id,
                                std::string node_id_binary) override;

    /**
     * There is no information to identify the source node.
     * We may see the leader as source node forever?
     */
    void HandleRequestPushLogs(RpcConn conn, int64_t committed_log_index,
                               int32_t pre_log_term,
                               raftcpp::LogEntry log_entry) override {}

private:
    uint16_t GetPortFromBinary(const std::string &binary_addr);

    uint16_t GetPortFromEndPoint(const std::string &endpoint);

    /**
     * Discard or delay the rpc here.
     * @return true: discard the rpc
     */
    bool MockNet();

    /**
     * check if source or destination of the rpc is blocked
     * check if destination is the source itself
     * @return true: discard the rpc
     */
    bool IfDiscard(uint16_t source_port);

    std::string GetRemoteNodePort(const RpcConn &conn);

    void ConnectToOtherNodes();

    void InitRpcHandlers();

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
 *
 * Examples can be referred in cluster_test.cc
 *
 * How to check leader?
 *     CheckOneLeader() will return true if there is only one leader in the cluster.
 *     GetLeader() returns all leaders.
 *     NOTICE: The above functions consider leaders in all terms, this should be fixed.
 *
 * How to manage the network?
 *     1. Pass true for Cluster constructor: "Cluster c(3, true);". This makes the network
 *        unreliable.
 *     2. Call SetNetUnreliable() when cluster is running makes the network unreliable.
 *     3. Call SetNetReliable() when cluster is running makes the network reliable.
 *     4. Max delay time can be set in the Cluster constructor: "CLuster c(3, true,
 *        3000)".  3000ms
 *     Tips: Unreliable network means some rpc will be discarded or delayed.
 *
 * How to manage nodes?
 *     1. BlockNode(int idx) cuts the network of the idx node, all rpc that it sends or
 *        receives will be discarded.
 *     2. UnblockNode(int idx) recovers the network of the idx node, all rpc that it sends
 *        or receives will be ok.
 *     3. TODO Shutdown(int idx) shuts one node down, it's different from blocking one
 *        node.
 */
class Cluster {
public:
    Cluster(int node_num, bool is_unreliable = false,
            int max_delay = DEFAULT_MAX_DELAY_MS)
        : node_num_(node_num) {
        std::vector<std::string> proxy_node_addr;
        std::vector<std::string> node_addr;
        std::deque<std::pair<std::string, std::string>> addr = InitAddress();

        // allocate addresses and create servers
        for (int i = 0; i < node_num_; i++) {
            // allocate for proxy nodes
            auto ip_port = addr.front();
            proxy_node_addr.push_back(ip_port.first + ":" + ip_port.second);
            addr.pop_front();
            proxy_servers_.push_back(std::make_shared<RpcServ>(
                std::stoi(ip_port.second), std::thread::hardware_concurrency()));

            // allocate for nodes
            ip_port = addr.front();
            node_addr.push_back(ip_port.first + ":" + ip_port.second);
            addr.pop_front();
            node_servers_.push_back(std::make_shared<RpcServ>(
                std::stoi(ip_port.second), std::thread::hardware_concurrency()));
            port_to_node_.insert(std::make_pair(ip_port.second, i));
        }

        std::vector<std::string> proxy_node_cfg;
        std::vector<std::string> node_cfg;

        // init node config
        for (int i = 0; i < node_num_; i++) {
            std::vector<std::string> addrs;

            addrs.push_back(node_addr[i]);
            for (int j = 0; j < node_num_; j++) {
                if (j == i) {
                    continue;
                }
                addrs.push_back(proxy_node_addr[j]);
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
        net_cfg_ =
            std::make_shared<NetworkConfig>(is_unreliable, max_delay, port_to_node_);

        std::condition_variable cond;

        // create proxy nodes
        for (int i = 0; i < node_num_; i++) {
            const auto config = raftcpp::common::Config::From(proxy_node_cfg[i]);
            proxy_threads_.push_back(
                std::thread(Cluster::ProxyRun, this, config, i, std::ref(cond)));
            std::unique_lock<std::mutex> up(mu);
            cond.wait(up);
        }

        // create nodes
        for (int i = 0; i < node_num_; i++) {
            const auto config = raftcpp::common::Config::From(node_cfg[i]);
            node_threads_.push_back(
                std::thread(Cluster::NodeRun, this, config, i, std::ref(cond)));
            std::unique_lock<std::mutex> up(mu);
            cond.wait(up);
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

    // FIXME returned leaders should be in the latest term
    std::vector<int> GetLeader() {
        std::vector<int> leader;
        net_cfg_->ReadLock();
        for (int i = 0; i < node_num_; i++) {
            // ensure it's not a blocked node since a blocked
            // node will keep its leader role
            if ((nodes_[i]->GetCurrState() == raftcpp::RaftState::LEADER) &&
                !(net_cfg_->IsBlocked(i))) {
                leader.push_back(i);
            }
        }
        net_cfg_->ReadUnlock();
        return leader;
    }

    bool CheckOneLeader() {
        std::vector<int> leaders = GetLeader();
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

    void UnblockNode(int idx) {
        net_cfg_->WriteLock();
        net_cfg_->UnblockNode(idx);
        net_cfg_->WriteUnlock();
    }

    void UnblockNodes(std::vector<int> nodes) {
        net_cfg_->WriteLock();
        net_cfg_->UnblockNodes(nodes);
        net_cfg_->WriteUnlock();
    }

    std::set<int> GetBlockedNodes() {
        net_cfg_->ReadLock();
        std::set<int> blocked_nodes = net_cfg_->GetBlockedNodes();
        net_cfg_->ReadUnlock();
        return blocked_nodes;
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
    static void ProxyRun(Cluster *self, raftcpp::common::Config config, int peer_node,
                         std::condition_variable &cond) {
        self->proxy_nodes_.push_back(std::make_shared<ProxyNode>(
            self->proxy_servers_[peer_node], config, peer_node, self->net_cfg_));
        cond.notify_all();
        self->proxy_servers_[peer_node]->run();
    }

    static void NodeRun(Cluster *self, raftcpp::common::Config config, int idx,
                        std::condition_variable &cond) {
        auto node = std::make_shared<raftcpp::node::RaftNode>(
            std::make_shared<MockStateMachine>(), *(self->node_servers_[idx]), config,
            raftcpp::RaftcppLogLevel::RLL_DEBUG);
        node->Init();
        self->nodes_.push_back(std::move(node));
        cond.notify_all();
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
