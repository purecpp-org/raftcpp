# asio external project
# target:
#  - asio_ep
# defines:
#  - ASIO_HOME
#  - ASIO_INCLUDE_DIR

set(ASIO_VERSION "1.16.1")
set(ASIO_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/asio-install")
set(ASIO_INCLUDE_DIR "${ASIO_PREFIX}")

set(ASIO_URL_MD5 "7f82f1c5c8dc048b634ab44a4594d341")

ExternalProject_Add(asio_ep
        PREFIX external/asio
        URL "https://github.com/chriskohlhoff/asio/archive/asio-1-16-1.tar.gz"
        URL_MD5 ${ASIO_URL_MD5}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

SET(ASIO_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/asio/src/asio_ep/asio/include)
