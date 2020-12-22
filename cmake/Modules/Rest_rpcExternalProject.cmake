# rest_rpc external project
# target:
#  - rest_rpc_ep
# defines:
#  - REST_RPC_HOME
#  - REST_RPC_INCLUDE_DIR
#  - MSGPACK_INCLUDE_DIR

set(REST_RPC_VERSION "0.09")



set(REST_RPC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/rest-rpc-install")
set(REST_RPC_INCLUDE_DIR "${REST_RPC_PREFIX}/include")
set(MSGPACK_INCLUDE_DIR "${REST_RPC_PREFIX}/third/msgpack/include")

set(REST_RPC_URL_MD5 "eb4f76b2f4fbba05c305369cc6f965e9")


ExternalProject_Add(rest_rpc_ep
        PREFIX external/rest-rpc
        URL "https://github.com/qicosmos/rest_rpc/archive/V${REST_RPC_VERSION}.tar.gz"
        URL_MD5 ${REST_RPC_URL_MD5}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

SET(REST_RPC_HOME ${CMAKE_CURRENT_BINARY_DIR}/external/rest-rpc/src/rest_rpc_ep)
SET(REST_RPC_INCLUDE_DIR ${REST_RPC_HOME}/include)
SET(MSGPACK_INCLUDE_DIR ${REST_RPC_HOME}/third/msgpack/include)
