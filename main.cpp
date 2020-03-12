#include <include/rpc_server.h>

int main() {
    using namespace rest_rpc;
    using namespace rest_rpc::rpc_service;

    rpc_server server(9000, std::thread::hardware_concurrency());

    server.register_handler("hello", [](rpc_conn conn) {
        return "hello world!";
    });

    server.run();
}
