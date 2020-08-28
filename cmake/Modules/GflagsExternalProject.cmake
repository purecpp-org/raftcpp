set(GFLAGS_VERSION        "v2.2.2")
set(GFLAGS_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external/gflags-install")
set(GFLAGS_INSTALL_DIR ${GFLAGS_PREFIX}/gflags)
set(GFLAGS_INCLUDE_DIR "${GFLAGS_INSTALL_DIR}/include" CACHE PATH "gflags include directory." FORCE)
set(GFLAGS_URL https://github.com/gflags/gflags/archive/${GFLAGS_VERSION}.zip)


if(MSVC)
  set(GFLAGS_LIBRARIES "${GFLAGS_INSTALL_DIR}/lib/gflags_static.lib" CACHE FILEPATH "GFLAGS_LIBRARIES" FORCE)
else(MSVC)
  set(GFLAGS_LIBRARIES "${GFLAGS_INSTALL_DIR}/lib/libgflags.a" CACHE FILEPATH "GFLAGS_LIBRARIES" FORCE)
endif(MSVC)

include_directories(SYSTEM ${GFLAGS_INCLUDE_DIR})
ExternalProject_Add(
    gflags_ep
    ${EXTERNAL_PROJECT_LOG_ARGS}      
    URL ${GFLAGS_URL}
    PREFIX          ${GFLAGS_PREFIX}
    UPDATE_COMMAND  ""
    CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=${GFLAGS_INSTALL_DIR}
    CMAKE_ARGS      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    CMAKE_ARGS      -DBUILD_TESTING=OFF
    CMAKE_ARGS      -DCMAKE_DEBUG_POSTFIX=""
                           
)


if(MSVC)
  add_custom_command(TARGET gflags_ep POST_BUILD
    COMMAND if $<CONFIG:Debug>==1 (${CMAKE_COMMAND} -E copy ${GFLAGS_INSTALL_DIR}/lib/gflags_static_debug.lib ${GFLAGS_INSTALL_DIR}/lib/gflags_static.lib)
  )
endif(MSVC)

add_library(gflags STATIC IMPORTED GLOBAL)

set_property(TARGET gflags PROPERTY IMPORTED_LOCATION ${GFLAGS_LIBRARIES})
add_dependencies(gflags gflags_ep)
if(MSVC)
  set_target_properties(gflags
    PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES
    shlwapi.lib)
endif(MSVC)
