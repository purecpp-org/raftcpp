#include <gflags/gflags.h>

#include <thread>

#include "common/config.h"
#include "counter_state_machine.h"
#include "node/node.h"
#include "rpc_server.h"

using namespace rest_rpc;
using namespace rpc_service;

using namespace examples;
using namespace examples::counter;

DEFINE_string(conf, "", "The configurations of this raft group.");
// DEFINE_string(this_addr, "", "This address of this instance listening on.");

class CounterServiceImpl {
public:
    // TODO(qwang): Are node and fsm uncopyable?
    CounterServiceImpl(std::shared_ptr<raftcpp::node::RaftNode> node,
                       std::shared_ptr<CounterStateMachine> &fsm)
        : node_(std::move(node)), fsm_(std::move(fsm)) {}

    void Incr(rpc_conn conn, int delta) {
        IncrRequest request = IncrRequest(delta);
        node_->Apply(request);
    }

    int64_t Get(rpc_conn conn) {
        // There is no need to gurantee the write-read consistency,
        // so we can get the value directly from this fsm instead of
        // apply it to all nodes.
        return fsm_->GetValue();
    }

private:
    std::shared_ptr<raftcpp::node::RaftNode> node_;
    std::shared_ptr<CounterStateMachine> fsm_;
};

int main(int argc, char *argv[]) {
    std::string conf_str;
    {
        gflags::ParseCommandLineFlags(&argc, &argv, false);
        conf_str = FLAGS_conf;
        gflags::ShutDownCommandLineFlags();
    }
    const auto config = raftcpp::common::Config::From(conf_str);

    // Initial a rpc server and listening on its port.
    rpc_server server(config.GetThisEndpoint().GetPort(),
                      std::thread::hardware_concurrency());

    auto node = std::make_shared<raftcpp::node::RaftNode>(server, config);
    auto fsm = std::make_shared<CounterStateMachine>();

    CounterServiceImpl service(node, fsm);
    server.register_handler("incr", &CounterServiceImpl::Incr, &service);
    server.register_handler("get", &CounterServiceImpl::Get, &service);
    server.run();

    return 0;
}
