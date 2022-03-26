# rest_rpc external project
# target:
#  - rest_rpc_ep
# defines:
#  - REST_RPC_HOME
#  - REST_RPC_INCLUDE_DIR
#  - MSGPACK_INCLUDE_DIR

set(REST_RPC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/rest-rpc-install")
set(REST_RPC_INCLUDE_DIR "${REST_RPC_PREFIX}/include")
set(MSGPACK_INCLUDE_DIR "${REST_RPC_PREFIX}/third/msgpack/include")

ExternalProject_Add(rest_rpc_ep
        PREFIX external/rest-rpc
        GIT_REPOSITORY https://github.com/qicosmos/rest_rpc.git
        GIT_TAG 98262e4b0aa77c448c6e5608b77bfbc2c1ef0c1c
        GIT_REMOTE_UPDATE_STRATEGY REBASE
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

SET(REST_RPC_HOME ${CMAKE_CURRENT_BINARY_DIR}/external/rest-rpc/src/rest_rpc_ep)
SET(REST_RPC_INCLUDE_DIR ${REST_RPC_HOME}/include)
SET(MSGPACK_INCLUDE_DIR ${REST_RPC_HOME}/thirdparty/msgpack-c/include)
