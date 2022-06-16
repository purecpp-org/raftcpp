
#include "common/logging.h"

#include "examples/proto/counter.pb.h"
#include "examples/proto/counter.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using examples::counter::CounterService;
using examples::counter::GetRequest;
using examples::counter::GetResponse;

int main(int argc, char *argv[]) {

    auto target_str = "localhost:50051";
    auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    auto stub = CounterService::NewStub(channel);

    {
        GetRequest req;
        GetResponse resp;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub->Get(&context, req, &resp);
        // Act upon its status.
        if (status.ok()) {
            std::cout << resp.value() << std::endl;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                        << std::endl;
            std::cout <<  "RPC failed" << std::endl;
        }
    }

    return 0;
}
