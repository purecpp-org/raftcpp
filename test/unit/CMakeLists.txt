cmake_minimum_required(VERSION 3.0)
set(project_name raftcpp_test)
project(${project_name})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread -std=c++17")

SET(ENABLE_ASIO_STANDALONE ON)
if(ENABLE_ASIO_STANDALONE)
	add_definitions(-DASIO_STANDALONE)
else()
	find_package(Boost 1.60 REQUIRED COMPONENTS system)
endif()

set(raftcpp_test
        memory_log_store_test.cpp
   )

add_executable(${project_name} ${raftcpp_test})
include_directories(
		../../thirdparty/asio/asio/include
        ../../thirdparty/rest_rpc
        ../../thirdparty/rest_rpc/third/msgpack/include
)

target_link_libraries(${project_name} -lstdc++fs)