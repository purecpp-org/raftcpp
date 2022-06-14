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
