# rest_rpc external project
# target:
#  - rest_rpc_ep
# defines:
#  - REST_RPC_HOME
#  - REST_RPC_INCLUDE_DIR
#  - MSGPACK_INCLUDE_DIR

#set(REST_RPC_VERSION "1.8.0")

if(APPLE)
    set(REST_RPC_CMAKE_CXX_FLAGS "-fPIC -DREST_RPC_USE_OWN_TR1_TUPLE=1 -Wno-unused-value -Wno-ignored-attributes")
elseif(NOT MSVC)
    set(REST_RPC_CMAKE_CXX_FLAGS "-fPIC")
endif()

if(CMAKE_BUILD_TYPE)
    string(TOUPPER ${CMAKE_BUILD_TYPE} UPPERCASE_BUILD_TYPE)
endif()

set(REST_RPC_CMAKE_CXX_FLAGS "${EP_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}} ${REST_RPC_CMAKE_CXX_FLAGS}")

set(REST_RPC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/rest-rpc-install")
set(REST_RPC_INCLUDE_DIR "${REST_RPC_PREFIX}/include")
set(MSGPACK_INCLUDE_DIR "${REST_RPC_PREFIX}/third/msgpack/include")

set(REST_RPC_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${REST_RPC_PREFIX}
        -DCMAKE_CXX_FLAGS=${REST_RPC_CMAKE_CXX_FLAGS})

set(REST_RPC_URL_MD5 "bb35e65ccb4928bdb7496d81840195bf")

ExternalProject_Add(rest_rpc_ep
        PREFIX external/rest-rpc
        URL "https://github.com/qicosmos/rest_rpc/archive/master.zip"
        URL_MD5 ${REST_RPC_URL_MD5}
        CMAKE_ARGS ${REST_RPC_CMAKE_ARGS}
        ${EP_LOG_OPTIONS})
