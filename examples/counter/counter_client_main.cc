
#include "common/logging.h"
//#include "rpc/client.h"

int main(int argc, char *argv[]) {
    auto rpc_client = std::make_shared<raftcpp::raftclient>("127.0.0.1:10001");
    rpc_client->enable_auto_heartbeat();
    RAFTCPP_LOG(RLL_DEBUG) << "Counter client connected to the server.";

    auto r = rpc_client->call<int>("incr", 98);
    RAFTCPP_LOG(RLL_INFO) << "==========r = " << r;
    return 0;
}
