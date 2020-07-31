# nanolog external project
# target:
#  - nanoloh_ep
# defines:
#  - NANOLOG_INCLUDE_DIR

set(MSGPACK_VERSION "v0.1.0")
set(DOWNLOAD_URL "https://github.com/qicosmos/nanolog/archive/${v0.1.0}.tar.gz")
set(MD5 71556be345c125b3c3fba52fedc6957f)

if(CMAKE_BUILD_TYPE)
    string(TOUPPER ${CMAKE_BUILD_TYPE} UPPERCASE_BUILD_TYPE)
endif()

ExternalProject_Add(nanolog_ep
        PREFIX external/nanolog
        URL ${DOWNLOAD_URL}
        URL_MD5 ${MD5})

set(NANOLOG_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/nanolog/src/nanolog_ep/include)
