CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
SET(project_name raftcpp)
PROJECT(${project_name})


SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread -std=c++17")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

#SET(ENABLE_ASIO_STANDALONE ON)
#if(ENABLE_ASIO_STANDALONE)
#	ADD_DEFINITIONS(-DASIO_STANDALONE)
#else()
#	FIND_PACKAGE(Boost 1.60 REQUIRED COMPONENTS system)
#endif()

INCLUDE(ExternalProject)
INCLUDE(DoctestExternalProject)
INCLUDE(AsioExternalProject)
INCLUDE(Rest_rpcExternalProject)
INCLUDE_DIRECTORIES(SYSTEM ${DOCTEST_INCLUDE_DIR})
INCLUDE_DIRECTORIES(SYSTEM thirdparty/logging)

INCLUDE_DIRECTORIES(includes)
INCLUDE_DIRECTORIES(
		SYSTEM ${ASIO_INCLUDE_DIR}
		SYSTEM ${REST_RPC_INCLUDE_DIR}
		SYSTEM ${MSGPACK_INCLUDE_DIR}
)


# Test targets
ADD_EXECUTABLE(memory_log_store_test src/tests/memory_log_store_test.cpp)
ADD_DEPENDENCIES(memory_log_store_test doctest_ep)
TARGET_LINK_LIBRARIES(memory_log_store_test pthread)

ADD_EXECUTABLE(nanolog_test src/tests/nanolog_test.cpp)
ADD_DEPENDENCIES(nanolog_test doctest_ep)
TARGET_LINK_LIBRARIES(nanolog_test)
