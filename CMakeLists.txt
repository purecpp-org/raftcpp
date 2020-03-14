cmake_minimum_required(VERSION 3.0)
set(project_name raftcpp)
project(${project_name})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread -std=c++17")

if(ENABLE_ASIO_STANDALONE)
	add_definitions(-DASIO_STANDALONE)
else()
	find_package(Boost 1.60 REQUIRED COMPONENTS system)
endif()

set(raftcpp
        main.cpp
   )

add_executable(${project_name} ${raftcpp})
include_directories(
        rest_rpc
        rest_rpc/third/msgpack/include
)

target_link_libraries(${project_name} -lstdc++fs)