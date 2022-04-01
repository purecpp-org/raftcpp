set(GRPC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/grpc_ep")
set(GRPC_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc)
set(GRPC_INCLUDE_DIR "${GRPC_DIR}/include")
set(GRPC_LIBRARY "${GRPC_DIR}/lib")
set(GRPC_GRPC++_REFLECTION_LIBRARY "${GRPC_DIR}/lib/libgrpc++_reflection.a")
set(GRPC_CPP_PLUGIN "${GRPC_DIR}/bin/grpc_cpp_plugin")

set(PROTOC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/protobuf_ep")
set(PROTOC_DIR ${CMAKE_CURRENT_BINARY_DIR}/protoc)
set(PROTOBUF_INCLUDE_DIR "${PROTOC_DIR}/include")
set(PROTOBUF_LIBRARY "${PROTOC_DIR}/lib")
set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOC_DIR}/bin/protoc")

include(${PROJECT_SOURCE_DIR}/cmake/Modules/FindGRPC.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Modules/FindProtobuf.cmake)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/grpc" "${CMAKE_CURRENT_BINARY_DIR}/protoc")

ExternalProject_Add(protobuf_ep
        PREFIX ${PROTOC_PREFIX}
        GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
        GIT_TAG 22d0e265de7d2b3d2e9a00d071313502e7d4cccf
        GIT_REMOTE_UPDATE_STRATEGY REBASE
        UPDATE_DISCONNECTED 1
        CMAKE_CACHE_ARGS
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-Dprotobuf_BUILD_TESTS:BOOL=OFF"
        "-Dprotobuf_BUILD_EXAMPLES:BOOL=OFF"
        "-Dprotobuf_WITH_ZLIB:BOOL=OFF"
        "-DCMAKE_INSTALL_PREFIX:PATH=${PROTOC_DIR}"
        SOURCE_SUBDIR cmake
        )

ExternalProject_Add(grpc_ep
        PREFIX ${GRPC_PREFIX}
        GIT_REPOSITORY "https://github.com/grpc/grpc.git"
        GIT_TAG 494b08ada4009ead0d0b70e44d354be72f9c283a
        GIT_REMOTE_UPDATE_STRATEGY REBASE
        UPDATE_DISCONNECTED 1
        CMAKE_CACHE_ARGS
        -DgRPC_INSTALL:BOOL=ON
        -DgRPC_BUILD_TESTS:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:PATH=${GRPC_DIR}
        UPDATE_DISCONNECTED 1
        )

set(PROTOS
        ${CMAKE_CURRENT_SOURCE_DIR}/protos/raft.proto
        )

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

include_directories(${PROTOBUF_INCLUDE_DIR})
include_directories(${GRPC_INCLUDE_DIR})

# ADD_EXECUTABLE(raft_rpc src/rpc/proto.cc ${PROTO_HDRS} ${GRPC_HDRS})