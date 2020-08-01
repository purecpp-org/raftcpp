# asio external project
# target:
#  - asio_ep
# defines:
#  - ASIO_HOME
#  - ASIO_INCLUDE_DIR

set(ASIO_VERSION "1.16.1")

if(APPLE)
    set(ASIO_CMAKE_CXX_FLAGS "-fPIC -DASIO_USE_OWN_TR1_TUPLE=1 -Wno-unused-value -Wno-ignored-attributes")
elseif(NOT MSVC)
    set(ASIO_CMAKE_CXX_FLAGS "-fPIC")
endif()

if(CMAKE_BUILD_TYPE)
    string(TOUPPER ${CMAKE_BUILD_TYPE} UPPERCASE_BUILD_TYPE)
endif()

set(ASIO_CMAKE_CXX_FLAGS "${EP_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}} ${ASIO_CMAKE_CXX_FLAGS}")

set(ASIO_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/asio-install")
set(ASIO_INCLUDE_DIR "${ASIO_PREFIX}")


set(ASIO_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${ASIO_PREFIX}
        -DCMAKE_CXX_FLAGS=${ASIO_CMAKE_CXX_FLAGS})

set(ASIO_URL_MD5 "d93a188b2d93a06d9acdb1484287c071")

ExternalProject_Add(asio_ep
        PREFIX external/asio
        URL "https://sourceforge.net/projects/asio/files/asio/${GTEST_VERSION}%20%28Stable%29/boost_asio_1_16_1.tar.gz"
        URL_MD5 ${ASIO_URL_MD5}
        CMAKE_ARGS ${ASIO_CMAKE_ARGS}
        ${EP_LOG_OPTIONS})
