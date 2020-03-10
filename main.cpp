#include <include/rpc_server.h>
using namespace rest_rpc;
using namespace rest_rpc::rpc_service;

int main() {
	rpc_server server(9000, std::thread::hardware_concurrency());

	server.register_handler("hello", [](rpc_conn conn) {
		return "hello world!";
	});

	server.run();
}