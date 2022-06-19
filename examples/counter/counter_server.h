#pragma once
#include <gflags/gflags.h>

#include <thread>

#include "common/config.h"
#include "counter_state_machine.h"
#include "node/node.h"


#include "examples/proto/counter.pb.h"
#include "examples/proto/counter.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using namespace examples;
using namespace examples::counter;
namespace examples {
namespace counter {

class CounterServiceImpl: public examples::counter::CounterService::Service, public std::enable_shared_from_this<CounterServiceImpl> {
public:
    // TODO(qwang): Are node and fsm uncopyable?
    CounterServiceImpl(std::shared_ptr<raftcpp::node::RaftNode> node,
                       std::shared_ptr<CounterStateMachine> &fsm)
        : node_(std::move(node)), fsm_(std::move(fsm)) {}



    grpc::Status Incr(::grpc::ServerContext *context,
                                        const ::examples::counter::IncrRequest *request,
                                        ::examples::counter::IncrResponse *response) {

    }

    grpc::Status Get(::grpc::ServerContext *context,
                                        const ::examples::counter::GetRequest *request,
                                        ::examples::counter::GetResponse *response) {

    }

    // void Incr(rpc_conn conn, int delta) {
    //     // CHECK is leader.
    //     RAFTCPP_LOG(RLL_INFO) << "=============Incring: " << delta;
    //     // Does this should be enabled from this?
    //     std::shared_ptr<IncrRequest> request = std::make_shared<IncrRequest>(delta);
    //     if (!node_->IsLeader()) {
    //         //// RETURN redirect.
    //     }
    //     node_->PushRequest(request);
    // }

    // int64_t Get(rpc_conn conn) {
    //     // There is no need to gurantee the write-read consistency,
    //     // so we can get the value directly from this fsm instead of
    //     // apply it to all nodes.
    //     return fsm_->GetValue();
    // }

private:
    std::shared_ptr<raftcpp::node::RaftNode> node_;
    std::shared_ptr<CounterStateMachine> fsm_;
};


}

}  // namespace examples
