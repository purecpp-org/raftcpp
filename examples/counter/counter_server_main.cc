#include <gflags/gflags.h>

#include <thread>

#include "common/config.h"
#include "counter_state_machine.h"
#include "node/node.h"
#include "counter_server.h"

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


DEFINE_string(conf, "", "The configurations of this raft group.");
// DEFINE_string(this_addr, "", "This address of this instance listening on.");

int main(int argc, char *argv[]) {
    std::string conf_str;
    {
        gflags::ParseCommandLineFlags(&argc, &argv, false);
        conf_str = FLAGS_conf;
        gflags::ShutDownCommandLineFlags();
    }
    //    RAFTCPP_CHECK(!conf_str.empty()) << "Failed to start counter server with empty
    //    config string.";
    if (conf_str.empty()) {
        RAFTCPP_LOG(RLL_INFO)
            << "Failed to start counter server with empty config string.";
        return -1;
    }
    const auto config = raftcpp::common::Config::From(conf_str);
    auto fsm = std::make_shared<CounterStateMachine>();
    auto node = std::make_shared<raftcpp::node::RaftNode>(fsm, config);
    node->Init();

    ////////grpc server
    std::string server_address("0.0.0.0:50051");
    CounterServiceImpl service(node, fsm);

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();

    return 0;
}
