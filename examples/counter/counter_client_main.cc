
#include "rest_rpc/rpc_client.hpp"
#include "common/logging.h"

int main(int argc, char *argv[]) {
    auto rpc_client = std::make_shared<rest_rpc::rpc_client>("127.0.0.1", 10001);
    bool connected = rpc_client->connect();
    if (!connected) {
        RAFTCPP_LOG(RLL_DEBUG)
            << "Failed to connect to the node: ";
    }
    rpc_client->enable_auto_heartbeat();
    RAFTCPP_LOG(RLL_DEBUG) << "Counter client connected to the server.";

    auto r = rpc_client->call<int>("incr", 98);
    RAFTCPP_LOG(RLL_INFO) << "==========r = " << r;
    return 0;
}
