set(GRPC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/grpc_ep")
# set(GRPC_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc)
# # set(GRPC_INCLUDE_DIR "${GRPC_DIR}/include")
# # set(GRPC_LIBRARY "${GRPC_DIR}/lib")
# # set(GRPC_GRPC++_LIBRARY "${GRPC_DIR}/lib/")
# # set(GRPC_GRPC++_REFLECTION_LIBRARY "${GRPC_DIR}/lib/")
# # set(GRPC_CPP_PLUGIN "${GRPC_DIR}/bin/grpc_cpp_plugin")

set(PROTOC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/protobuf_ep")
set(PROTOC_DIR ${CMAKE_CURRENT_BINARY_DIR}/protobuf)
set(PROTOBUF_INCLUDE_DIR "${PROTOC_DIR}/include")
set(PROTOBUF_LIBRARY "${PROTOC_DIR}/lib")
set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOC_DIR}/bin/protoc")

# # list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/cmake)
# # set(PROTOBUF_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/cmake/protobuf)
# # set(GRPC_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/cmake/grpc)

# #link_directories(${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/)

# set(GRPC_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc/include)
# set(GRPC_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc/bin)
# set(GRPC_LIBRARY  ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc.a)
# set(GRPC_GRPC++_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc++.a)
# set(GRPC_GRPC++_REFLECTION_LIBRARY  ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc++_reflection.a)
# set(GRPC_CPP_PLUGIN  ${CMAKE_CURRENT_BINARY_DIR}/grpc/bin/grpc_cpp_plugin)

# set(PROTOBUF_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc/include)
# set(PROTOBUF_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libprotobuf.a)
# set(PROTOBUF_PROTOC_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/grpc/bin/protoc)
# set(CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib)

ExternalProject_Add(protobuf_ep
        PREFIX ${PROTOC_PREFIX}
        GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
        GIT_TAG 22d0e265de7d2b3d2e9a00d071313502e7d4cccf
        GIT_REMOTE_UPDATE_STRATEGY REBASE
        UPDATE_DISCONNECTED 1
        CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -Dprotobuf_BUILD_TESTS:BOOL=OFF
        -Dprotobuf_BUILD_EXAMPLES:BOOL=OFF
        -Dprotobuf_WITH_ZLIB:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:PATH=${PROTOC_DIR}
        SOURCE_SUBDIR cmake
        )

include(FetchContent)
FetchContent_Populate(
        grpc_ep
        GIT_REPOSITORY "https://github.com/grpc/grpc.git"
        GIT_TAG 494b08ada4009ead0d0b70e44d354be72f9c283a
        GIT_REMOTE_UPDATE_STRATEGY REBASE
        UPDATE_DISCONNECTED 1
        SOURCE_DIR ${GRPC_PREFIX}
)

# ExternalProject_Add(grpc_ep
#         PREFIX ${GRPC_PREFIX}
#         GIT_REPOSITORY "https://github.com/grpc/grpc.git"
#         GIT_TAG 494b08ada4009ead0d0b70e44d354be72f9c283a
#         GIT_REMOTE_UPDATE_STRATEGY REBASE
#         UPDATE_DISCONNECTED 1
#         CMAKE_CACHE_ARGS
#         -DgRPC_INSTALL:BOOL=OFF
#         -DgRPC_BUILD_TESTS:BOOL=OFF
#         #-DCMAKE_INSTALL_PREFIX:PATH=${GRPC_DIR}
#         UPDATE_DISCONNECTED 1
#         )

set(grpc_DIR ${GRPC_PREFIX})
set(grpc_INCL_DIR ${grpc_DIR})
add_subdirectory(${grpc_DIR})
include_directories(${grpc_INCL_DIR})

message(STATUS "Using gRPC via add_subdirectory.")

# After using add_subdirectory, we can now use the grpc targets directly from
# this build.
set(protobuf_MODULE_COMPATIBLE TRUE)
set(PROTOBUF_LIBPROTOBUF libprotobuf)
set(GRPC_GRPC++_REFLECTION_LIBRARY grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(PROTOBUF_PROTOC protoc)
else()
    set(PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

set(GRPC_GRPC++_LIBRARY grpc++)

if(CMAKE_CROSSCOMPILING)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
else()
    set(GRPC_CPP_PLUGIN $<TARGET_FILE:grpc_cpp_plugin>)
endif()


get_filename_component(raft_proto "./protos/raft.proto" ABSOLUTE)
get_filename_component(raft_proto_path "${raft_proto}" PATH)

# Generated sources  生成的文件的路径
set(raft_proto_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/raft.pb.cc")
set(raft_proto_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/raft.pb.h")
set(raft_grpc_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/raft.grpc.pb.cc")
set(raft_grpc_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/raft.grpc.pb.h")

add_custom_command(
	OUTPUT 	"${raft_proto_srcs}" 
		"${raft_proto_hdrs}"
		"${raft_grpc_srcs}"
		"${raft_grpc_hdrs}"
	COMMAND ${PROTOBUF_PROTOC}
	ARGS 	--grpc_out "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/"
		--cpp_out "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/"
		-I "${raft_proto_path}"
		--plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
		"${raft_proto}"
	DEPENDS "${raft_proto}"
)


# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/")

# ADD_EXECUTABLE(raft_rpc src/rpc/proto.cc ${PROTO_HDRS} ${GRPC_HDRS})

# include(${PROJECT_SOURCE_DIR}/cmake/Modules/FindGRPC.cmake)
# include(${PROJECT_SOURCE_DIR}/cmake/Modules/FindProtobuf.cmake)

# set(PROTOS
#         ${CMAKE_CURRENT_SOURCE_DIR}/protos/raft.proto
#         )

# set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
# file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
# include_directories(${PROTO_SRC_DIR})

# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
# grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})



# link_directories(${CMAKE_CURRENT_BINARY_DIR}/grpc/lib)
#FIND_LIBRARY(GRPC_LIBRARY grpc::grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib NO_DEFAULT_PATH)
# link_libraries(GRPC_LIBRARY)




# set(SLIBS  ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libprotobuf.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libares.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libboringssl.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgpr.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc++.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc++_core_stats.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc++_error_details.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc_plugin_support.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc_unsecure.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc++_unsecure.a \
# ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libz.a \
#         -Wl,--no-as-needed ${CMAKE_CURRENT_BINARY_DIR}/grpc/lib/libgrpc++_reflection.a -Wl,--as-needed
# )