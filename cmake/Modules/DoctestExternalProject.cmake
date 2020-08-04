# doctest external project
# target:
#  - doctest_ep
# defines:
#  - DOCTEST_HOME
#  - DOCTEST_INCLUDE_DIR

set(DOCTEST_VERSION "2.4.0")

set(DOCTEST_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/doctest-install")
set(DOCTEST_INCLUDE_DIR "${DOCTEST_PREFIX}/doctest")

set(DOCTEST_URL_MD5 "0b679d6294f97be4fb11cb26e801fda6")

ExternalProject_Add(doctest_ep
        PREFIX external/doctest
        URL "http://github.com/onqtam/doctest/archive/${DOCTEST_VERSION}.tar.gz"
        URL_MD5 ${DOCTEST_URL_MD5}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

SET(DOCTEST_HOME ${CMAKE_CURRENT_BINARY_DIR}/external/doctest/src/doctest_ep)
SET(DOCTEST_INCLUDE_DIR ${DOCTEST_HOME}/doctest)
