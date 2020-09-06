# spdlog external project
# target:
#  - spdlog_ep
# defines:
#  - SPDLOG_HOME
#  - SPDLOG_INCLUDE_DIR

set(SPDLOG_VERSION "1.8.0")

set(SPDLOG_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/spdlog-install")
set(SPDLOG_INSTALL_DIR ${SPDLOG_PREFIX}/spdlog)
set(SPDLOG_URL_MD5 "cbd179161d1ed185bd9f3f242c424fd7")

ExternalProject_Add(spdlog_ep
        PREFIX external/spdlog
        URL "https://github.com/gabime/spdlog/archive/v${SPDLOG_VERSION}.tar.gz"
        URL_MD5 ${SPDLOG_URL_MD5}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

SET(SPDLOG_HOME ${CMAKE_CURRENT_BINARY_DIR}/external/spdlog/src/spdlog_ep)
SET(SPDLOG_INCLUDE_DIR ${SPDLOG_HOME}/include)

if(MSVC)
    set(SPDLOG_LIBRARIES "${SPDLOG_INSTALL_DIR}/lib/spdlog_static.lib" CACHE FILEPATH "SPDLOG_LIBRARIES" FORCE)
else(MSVC)
    set(SPDLOG_LIBRARIES "${SPDLOG_INSTALL_DIR}/lib/libspdlog.a" CACHE FILEPATH "SPDLOG_LIBRARIES" FORCE)
endif(MSVC)

if(MSVC)
    add_custom_command(TARGET spdlog_ep POST_BUILD
            COMMAND if $<CONFIG:Debug>==1 (${CMAKE_COMMAND} -E copy ${SPDLOG_INSTALL_DIR}/lib/spdlog_static_debug.lib ${SPDLOG_INSTALL_DIR}/lib/spdlog_static.lib)
            )
endif(MSVC)

add_library(spdlog STATIC IMPORTED GLOBAL)

set_property(TARGET spdlog PROPERTY IMPORTED_LOCATION ${SPDLOG_LIBRARIES})
add_dependencies(spdlog spdlog_ep)
if(MSVC)
    set_target_properties(spdlog
            PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES
            spdlog.lib)
endif(MSVC)
