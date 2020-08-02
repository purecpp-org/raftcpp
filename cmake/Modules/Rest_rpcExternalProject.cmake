# rest_rpc external project
# target:
#  - rest_rpc_ep
# defines:
#  - REST_RPC_HOME
#  - REST_RPC_INCLUDE_DIR
#  - MSGPACK_INCLUDE_DIR

#set(REST_RPC_VERSION "1.8.0")

set(REST_RPC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/rest-rpc-install")
set(REST_RPC_INCLUDE_DIR "${REST_RPC_PREFIX}/include")
set(MSGPACK_INCLUDE_DIR "${REST_RPC_PREFIX}/third/msgpack/include")

set(REST_RPC_URL_MD5 "bb35e65ccb4928bdb7496d81840195bf")

ExternalProject_Add(rest_rpc_ep
        PREFIX external/rest-rpc
        URL "https://github.com/qicosmos/rest_rpc/archive/master.zip"
        URL_MD5 ${REST_RPC_URL_MD5}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND bash -c "git submodule init && git submodule update"
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

SET(REST_RPC_HOME ${CMAKE_CURRENT_BINARY_DIR}/external/rest-rpc/src/rest_rpc_ep)
SET(REST_RPC_INCLUDE_DIR ${REST_RPC_HOME}/include)
SET(MSGPACK_INCLUDE_DIR ${REST_RPC_HOME}/third/msgpack/include)
