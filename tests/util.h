#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <deque>

#include "rest_rpc/rpc_server.h"
#include "node/node.h"
#include "rwlock.h"
#include "rpc/services.h"

const int DEFAULT_MAX_DELAY = 3000; // ms

using RpcServ = rest_rpc::rpc_service::rpc_server

class NetworkConfig {
public:
    NetworkConfig(bool is_unreliable, int max_delay) 
        : is_unreliable_(is_unreliable), max_delay_(max_delay) {}

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

private:
    // ndoes that their rpc should be discarded
    std::set<int> block_nodes_;

    // rpc may be delayed or discarded when the network is unreliable
    bool is_unreliable_;

    // maximum delay time, ms
    int max_delay_ = 3000;

    ReaderWriterLock rwlock;
};

class ProxyNode : public rpc::NodeService {
public:
    void HandleRequestPreVote(rpc::RpcConn conn, const std::string &endpoint_str,
                            int32_t term_id) override;

    void HandleRequestVote(rpc::RpcConn conn, const std::string &endpoint_str,
                            int32_t term_id) override;

    void HandleRequestHeartbeat(rpc::RpcConn conn, int32_t term_id,
                                std::string node_id_binary) override;

    void HandleRequestPullLogs(rpc::RpcConn conn, std::string node_id_binary,
                                int64_t next_log_index) override;

    void HandleRequestPushLogs(rpc::RpcConn conn, int64_t committed_log_index,
                                LogEntry log_entry) override;

private:
    void ConnectToOtherNodes();
    void InitRpcHandlers();

    std::shared_ptr<NetworkConfig> net_cfg_;
    RpcServ rpc_server_;
};


// TODO Node failure should be considered which is different from blocking nodes
/**
 * @brief Cluster manages several raft nodes and mocks a real-world network environment
 */
class Cluster {
public:
    Cluster(int node_num, bool is_unreliable = true, int max_delay = DEFAULT_MAX_DELAY)
        : node_num_(node_num), net_cfg_(std::make_shared<NetworkConfig>(is_unreliable, max_delay)) {
        std::vector<std::string> proxy_node_addr;
        std::vector<std::string> node_addr;
        std::deque<std::pair<std::string, std::string>> addr = InitAddress();

        // allocate addresses and create servers
        for (int i = 0; i < node_num_; i++) {
            auto ip_port = addr.front();
            proxy_node_addr.push_back(ip_port.first + ":" + ip_port.second);
            port_to_node_.insert(ip_port.second, i);
            addr.pop_front();
            proxy_servers_.push_back(
                std::make_shared<RpcServ>(std::stoi(ip_port.second), std::thread::hardware_concurrency()));

            ip_port = addr.front();
            node_addr.push_back(ip_port.first + ":" + ip_port.second);
            addr.pop_front();
            servers_.push_back(
                std::make_shared<RpcServ>(std::stoi(ip_port.second), std::thread::hardware_concurrency()));
        }

        std::vector<std::string> proxy_node_cfg;
        std::vector<std::string> node_cfg;

        // init node config
        for (int i = 0; i < node_num_; i++) {
            std::vector<std::string> addrs;
            addrs.push_back(node_addr[i]);
            for (auto addr : proxy_node_addr) {
                addrs.push_back(addr);
            }
            node_cfg.push_back(GenerateConfig(addrs));
        }

        // init proxy node config
        for (int i = 0; i < node_num_; i++) {
            std::vector<std::string> addrs;
            addrs.push_back(proxy_nodes_[i]);
            addrs.push_back(node_addr[i]);
            proxy_node_cfg.push_back(GenerateConfig(addrs));
        }

        
    }

    Cluster(const Cluster &cluster) = delete;
    Cluster &operator=(const Cluster &cluster) = delete;
    
    std::vector<int> GetLeader() {
        std::vector<int> leader;
        for (int i = 0; i < node_num_; i++) {
            if (nodes_[i]->GetCurrState() == raftcpp::RaftState::LEADER) {
                leader.push_back(i);
            }
        }
    }

    raftcpp::RaftState GetNodeState(int idx) {
        return nodes_[idx]->GetCurrState();
    }

    void Stop(int idx) {
        
    }

    void Start(int idx) {

    }

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

    int GetNodeNum() {
        return node_num_;
    }

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
        std::deque<std::string> addr;

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
    
    int node_num_;

    std::vector<std::shared_ptr<raftcpp::node::RaftNode>> nodes_;
    std::vector<std::shared_ptr<RpcServ>> servers_;

    // help the proxy node find the rpc caller node
    std::map<std::string, int> port_to_node_;

    std::vector<std::shared_ptr<ProxyNode>> proxy_nodes_;
    std::vector<std::shared_ptr<RpcServ>> proxy_servers_;

    std::shared_ptr<NetworkConfig> net_cfg_;

    // server's port is assigned according to this BASE_PORT
    const int BASE_PORT = 10000;
};
