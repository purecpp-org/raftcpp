# asio external project
# target:
#  - asio_ep
# defines:
#  - ASIO_HOME
#  - ASIO_INCLUDE_DIR

set(ASIO_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/asio-install")
set(ASIO_INCLUDE_DIR "${ASIO_PREFIX}")

ExternalProject_Add(asio_ep
        PREFIX external/asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG bba12d10501418fd3789ce01c9f86a77d37df7ed
        GIT_REMOTE_UPDATE_STRATEGY REBASE
        UPDATE_DISCONNECTED 1
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

SET(ASIO_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/asio/src/asio_ep/asio/include)
