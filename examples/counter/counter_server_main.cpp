#include <thread>

#include "node/node.h"
#include "common/type_def.h"

#include "counter_state_machine.h"
#include "rpc_server.h"

using namespace rest_rpc;
using namespace rpc_service;

using namespace examples;
using namespace examples::counter;

class CounterServiceImpl {
public:
  // TODO(qwang): Are node and fsm uncopyable?
  CounterServiceImpl(std::shared_ptr<raftcpp::node::RaftNode> node, std::shared_ptr<CounterStateMachine> &fsm)
    : node_(std::move(node)), fsm_(std::move(fsm)) {}

  void incr(int delta) {
    IncrRequest request = IncrRequest(delta);
    node_->Apply(request);
  }

  int64_t get() {
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

  std::shared_ptr<raftcpp::node::RaftNode> node = std::make_shared<raftcpp::node::RaftNode>(
          "127.0.0.1", 10001, raftcpp::RaftState::LEADER);
  std::shared_ptr<CounterStateMachine> fsm = std::make_shared<CounterStateMachine>();
  CounterServiceImpl service(node, fsm);
  server.register_handler("incr", &CounterServiceImpl::incr, &service);
  server.register_handler("get", &CounterServiceImpl::get, &service);
  server.run();

  return 0;
}
