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
INCLUDE(GflagsExternalProject)
INCLUDE(CustomeFunctions)
INCLUDE_DIRECTORIES(SYSTEM ${DOCTEST_INCLUDE_DIR})
INCLUDE_DIRECTORIES(SYSTEM thirdparty/logging)
INCLUDE_DIRECTORIES(src/)
INCLUDE_DIRECTORIES(includes)
INCLUDE_DIRECTORIES(
		SYSTEM ${ASIO_INCLUDE_DIR}
		SYSTEM ${REST_RPC_INCLUDE_DIR}
		SYSTEM ${MSGPACK_INCLUDE_DIR}
		SYSTEM ${MSGPACK_INCLUDE_DIR}
		SYSTEM ${GFLAGS_INCLUDE_DIR}
)


# node library
ADD_LIBRARY(node_lib src/node/node.cc)
ADD_DEPENDENCIES(node_lib asio_ep rest_rpc_ep)


#RAFTCPP_DEFINE_TEST( test_memory_log_store  FILE ests/memory_log_store_test.cpp DEP rest_rpc_ep doctest_ep LINK pthread)

# Test targets
if(UNIX)
	set(link_libs pthread)
else()
	set(link_libs )
endif()
RAFTCPP_DEFINE_TEST(test_memory_log_store SRC_FILES tests/memory_log_store_test.cpp LINKS ${link_libs})
#ADD_EXECUTABLE(test_memory_log_store tests/memory_log_store_test.cpp)
#ADD_DEPENDENCIES(test_memory_log_store rest_rpc_ep doctest_ep)
#if(UNIX)
#TARGET_LINK_LIBRARIES(test_memory_log_store pthread)
#else()
#	TARGET_LINK_LIBRARIES(test_memory_log_store )
#endif()
RAFTCPP_DEFINE_TEST(node_main SRC_FILES src/node/node_main.cc src/node/node.cc LINKS node_lib gflags)
#ADD_EXECUTABLE(node_main src/node/node_main.cc src/node/node.cc)
#ADD_DEPENDENCIES(node_main node_lib gflags_ep)
#TARGET_LINK_LIBRARIES(node_main node_lib gflags)

RAFTCPP_DEFINE_TEST(test_nanolog SRC_FILES tests/nanolog_test.cpp )

#ADD_EXECUTABLE(test_nanolog tests/nanolog_test.cpp)
#ADD_DEPENDENCIES(test_nanolog doctest_ep)
#TARGET_LINK_LIBRARIES(test_nanolog)

RAFTCPP_DEFINE_TEST(example_counter_server_main SRC_FILES examples/counter/counter_server_main.cpp LINKS node_lib gflags)
# counter examples
#ADD_EXECUTABLE(example_counter_server_main examples/counter/counter_server_main.cpp)
#ADD_DEPENDENCIES(example_counter_server_main node_lib gflags_ep)
#TARGET_LINK_LIBRARIES(example_counter_server_main node_lib gflags)
