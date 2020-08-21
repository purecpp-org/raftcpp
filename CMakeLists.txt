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
ADD_DEPENDENCIES(node_lib asio_ep rest_rpc_ep)

# Test targets
ADD_EXECUTABLE(test_memory_log_store tests/memory_log_store_test.cpp)
ADD_DEPENDENCIES(test_memory_log_store rest_rpc_ep doctest_ep)
if(UNIX)
TARGET_LINK_LIBRARIES(test_memory_log_store pthread)
else()
	TARGET_LINK_LIBRARIES(test_memory_log_store )
endif()

ADD_EXECUTABLE(node_main src/node/node_main.cc src/node/raft_node.cc)
ADD_DEPENDENCIES(node_main node_lib)
TARGET_LINK_LIBRARIES(node_main node_lib)

ADD_EXECUTABLE(test_nanolog tests/nanolog_test.cpp)
ADD_DEPENDENCIES(test_nanolog doctest_ep)
TARGET_LINK_LIBRARIES(test_nanolog)

ADD_EXECUTABLE(test_raft_node tests/raft_node_test.cpp)
ADD_DEPENDENCIES(test_raft_node common_lib node_lib doctest_ep)
TARGET_LINK_LIBRARIES(test_raft_node node_lib common_lib)

ADD_EXECUTABLE(test_flag tests/flag_test.cc)
ADD_DEPENDENCIES(test_flag common_lib doctest_ep)
TARGET_LINK_LIBRARIES(test_flag common_lib)
