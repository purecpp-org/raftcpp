#include <thread>

#include "common/type_def.h"
#include "counter_state_machine.h"
#include "node/node.h"
#include "rpc_server.h"

#include <gflags/gflags.h>

using namespace rest_rpc;
using namespace rpc_service;

using namespace examples;
using namespace examples::counter;

//DEFINE_string(raftcpp_conf, "", "The configurations of this raft group.");
//DEFINE_string(this_addr, "", "This address of this instance listening on.");

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
    rpc_server server(10001, std::thread::hardware_concurrency());

    std::shared_ptr<raftcpp::node::RaftNode> node =
        std::make_shared<raftcpp::node::RaftNode>("127.0.0.1", 10002,
                                                  raftcpp::RaftState::LEADER);
    std::shared_ptr<CounterStateMachine> fsm = std::make_shared<CounterStateMachine>();
    CounterServiceImpl service(node, fsm);
    server.register_handler("incr", &CounterServiceImpl::Incr, &service);
    server.register_handler("get", &CounterServiceImpl::Get, &service);
    server.run();

    return 0;
}
