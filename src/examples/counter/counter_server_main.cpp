#include <thread>

#include "counter_state_machine.h"
#include "rpc_server.h"

using namespace rest_rpc;
using namespace rpc_service;

using namespace examples;
using namespace examples::counter;

class CounterServiceImpl {
public:
  // TODO(qwang): Are node and fsm uncopyable?
  CounterServiceImpl(RaftcppNode node, CounterStateMachine fsm)
    : node_(node), fsm_(fsm) {}

  void incr(int delta) {
    IncrRequest request = IncrRequest(delta);
    node.apply(request);
  }

  int64_t get() {
    // There is no need to gurantee the write-read consistency,
    // so we can get the value directly from this fsm instead of
    // apply it to all nodes.
    return fsm_.GetValue();
  }

private:
  RaftcppNode node_;
  CounterStateMachine fsm_;
};



int main(int argc, char *argv[]) {
  rpc_server server(10001, std::thread::hardware_concurrency());

  RaftcppNode node;
  CounterStateMachine fsm;
  CounterServiceImpl service(node, fsm);
  server.register_handler("incr", &CounterServiceImpl::incr, &service);
  server.register_handler("get", &CounterServiceImpl::get, &service);
  server.run();

  return 0;
}
