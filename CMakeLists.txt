CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
SET(project_name raftcpp)
set(CMAKE_CXX_STANDARD 17)

PROJECT(${project_name})

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

SET(ENABLE_ASIO_STANDALONE ON)
if(ENABLE_ASIO_STANDALONE)
	ADD_DEFINITIONS(-DASIO_STANDALONE)
else()
	FIND_PACKAGE(Boost 1.60 REQUIRED COMPONENTS system)
endif()
INCLUDE(CompilerOptions)
INCLUDE(ExternalProject)
INCLUDE(DoctestExternalProject)
INCLUDE(AsioExternalProject)
INCLUDE(Rest_rpcExternalProject)
INCLUDE_DIRECTORIES(SYSTEM ${DOCTEST_INCLUDE_DIR})
INCLUDE_DIRECTORIES(SYSTEM thirdparty/logging)
INCLUDE_DIRECTORIES(src/)
INCLUDE_DIRECTORIES(includes)
INCLUDE_DIRECTORIES(
		SYSTEM ${ASIO_INCLUDE_DIR}
		SYSTEM ${REST_RPC_INCLUDE_DIR}
		SYSTEM ${MSGPACK_INCLUDE_DIR}
)


# common library
ADD_LIBRARY(common_lib src/common/flags.cc)

# node library
ADD_LIBRARY(node_lib src/node/raft_node.cc)

# Test targets
ADD_EXECUTABLE(memory_log_store_test tests/memory_log_store_test.cpp)
ADD_DEPENDENCIES(memory_log_store_test rest_rpc_ep doctest_ep)
if(UNIX)
TARGET_LINK_LIBRARIES(memory_log_store_test pthread)
else()
	TARGET_LINK_LIBRARIES(memory_log_store_test )
endif()

ADD_EXECUTABLE(nanolog_test tests/nanolog_test.cpp)
ADD_DEPENDENCIES(nanolog_test doctest_ep)
TARGET_LINK_LIBRARIES(nanolog_test)


ADD_EXECUTABLE(raft_node src/node/node_main.cc src/node/raft_node.cc)
ADD_DEPENDENCIES(raft_node asio_ep rest_rpc_ep)
#TARGET_LINK_LIBRARIES(raft_node)

ADD_EXECUTABLE(raft_node_test tests/raft_node_test.cpp)
ADD_DEPENDENCIES(raft_node_test common_lib node_lib doctest_ep raft_node)
TARGET_LINK_LIBRARIES(raft_node_test node_lib common_lib)

ADD_EXECUTABLE(flag_test tests/flag_test.cc)
ADD_DEPENDENCIES(flag_test common_lib)
TARGET_LINK_LIBRARIES(flag_test common_lib)
