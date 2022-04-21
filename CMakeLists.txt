CMAKE_MINIMUM_REQUIRED(VERSION 3.19)
SET(project_name raftcpp)
set(CMAKE_CXX_STANDARD 20)

PROJECT(${project_name})

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

ADD_DEFINITIONS(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)

SET(ENABLE_ASIO_STANDALONE ON)
if(ENABLE_ASIO_STANDALONE)
	ADD_DEFINITIONS(-DASIO_STANDALONE)
else()
	FIND_PACKAGE(Boost 1.60 REQUIRED COMPONENTS system)
endif()
INCLUDE(CompilerOptions)
INCLUDE(ExternalProject)
INCLUDE(GtestExternalProject)
INCLUDE(AsioExternalProject)
INCLUDE(Rest_rpcExternalProject)
INCLUDE(GrpcExternal)
INCLUDE(GflagsExternalProject)
INCLUDE(CustomeFunctions)
INCLUDE(SpdlogExternalProject)
INCLUDE_DIRECTORIES(SYSTEM thirdparty/logging)
INCLUDE_DIRECTORIES(src/)
INCLUDE_DIRECTORIES(includes)
INCLUDE_DIRECTORIES(
		SYSTEM ${GTEST_INCLUDE_DIR}
		SYSTEM ${ASIO_INCLUDE_DIR}
		SYSTEM ${REST_RPC_INCLUDE_DIR}
		SYSTEM ${PROTOBUF_INCLUDE_DIR}
		SYSTEM ${GRPC_INCLUDE_DIR}
		SYSTEM ${MSGPACK_INCLUDE_DIR}
		SYSTEM ${GFLAGS_INCLUDE_DIR}
		SYSTEM ${SPDLOG_INCLUDE_DIR}
		SYSTEM ${PROTO_SRC_DIR}
)

# common library
ADD_LIBRARY(common_lib
		src/common/config.cc
		src/common/timer.cc
		src/common/timer_manager.cc
		src/common/file.cc
		src/common/logging.cc)
ADD_DEPENDENCIES(common_lib asio_ep rest_rpc_ep grpc_ep protobuf_ep spdlog_ep)

# node library
ADD_LIBRARY(
		node_lib
		src/node/node.cc
		src/log_manager/leader_log_manager.cc
		src/log_manager/non_leader_log_manager.cc)
ADD_DEPENDENCIES(node_lib asio_ep rest_rpc_ep grpc_ep protobuf_ep common_lib)


# grpc library
ADD_LIBRARY(
		rpc_lib
		src/rpc/proto.cc
		src/rpc/client.cc
		${PROTO_HDRS}
		${GRPC_HDRS}
)
ADD_DEPENDENCIES(rpc_lib common_lib asio_ep grpc_ep protobuf_ep)
TARGET_LINK_LIBRARIES(node_lib rpc_lib common_lib)


# Test targets
if(UNIX)
	set(link_libs pthread)
else()
	set(link_libs )
endif()

if(WIN32)
	ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
endif()

# RAFTCPP_DEFINE_TEST(example_counter_client_main SRC_FILES examples/counter/counter_client_main.cc LINKS common_lib gflags)
# RAFTCPP_DEFINE_TEST(example_counter_server_main SRC_FILES examples/counter/counter_server_main.cc LINKS node_lib gflags)
RAFTCPP_DEFINE_TEST(test_timer SRC_FILES tests/unit_tests/util.cc tests/unit_tests/timer_test.cc LINKS common_lib)
RAFTCPP_DEFINE_TEST(test_config SRC_FILES tests/unit_tests/util.cc tests/unit_tests/config_test.cc LINKS common_lib)
RAFTCPP_DEFINE_TEST(test_logging SRC_FILES tests/unit_tests/util.cc tests/unit_tests/logging_test.cc LINKS common_lib)
RAFTCPP_DEFINE_TEST(test_file SRC_FILES tests/unit_tests/util.cc tests/unit_tests/file_test.cc LINKS common_lib)
RAFTCPP_DEFINE_TEST(test_log_manager SRC_FILES tests/unit_tests/util.cc tests/unit_tests/blocking_queue_test.cc LINKS common_lib)
RAFTCPP_DEFINE_TEST(test_nodeid SRC_FILES tests/unit_tests/util.cc tests/unit_tests/nodeid_test.cc LINKS common_lib)
# RAFTCPP_DEFINE_TEST(test_node_elect SRC_FILES tests/unit_tests/util.cc tests/unit_tests/node_elect_test.cc LINKS common_lib node_lib)
RAFTCPP_DEFINE_TEST(logging_check_test SRC_FILES tests/unit_tests/util.cc tests/unit_tests/logging_check_test.cc LINKS common_lib node_lib)
#RAFTCPP_DEFINE_TEST(cluster_test SRC_FILES tests/unit_tests/util.cc tests/unit_tests/cluster_test.cc LINKS common_lib node_lib)
#RAFTCPP_DEFINE_TEST(log_manager_test SRC_FILES tests/unit_tests/util.cc tests/unit_tests/log_manager_test.cc LINKS common_lib node_lib)
