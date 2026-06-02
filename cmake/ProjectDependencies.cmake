# ProjectDependencies.cmake
# 统一查找所有外部依赖。按类别：数据库、RPC 框架、网络、序列化、系统库。

# ── Muduo ──────────────────────────────────────────────────────────────────────

if (NOT DEFINED MUDUO_ROOT OR MUDUO_ROOT STREQUAL "")
    if (DEFINED ENV{MUDUO_ROOT} AND NOT "$ENV{MUDUO_ROOT}" STREQUAL "")
        set(MUDUO_ROOT "$ENV{MUDUO_ROOT}" CACHE PATH "Muduo installation prefix.")
    else()
        set(MUDUO_ROOT "/usr/local" CACHE PATH "Muduo installation prefix.")
    endif()
endif()

find_path(CHATSERVER_MUDUO_INCLUDE_DIR
    NAMES muduo/net/EventLoop.h
    HINTS "${MUDUO_ROOT}/include"
    NO_DEFAULT_PATH
)
find_library(CHATSERVER_MUDUO_NET_LIBRARY
    NAMES muduo_net
    HINTS "${MUDUO_ROOT}/lib" "${MUDUO_ROOT}/lib64"
    NO_DEFAULT_PATH
)
find_library(CHATSERVER_MUDUO_BASE_LIBRARY
    NAMES muduo_base
    HINTS "${MUDUO_ROOT}/lib" "${MUDUO_ROOT}/lib64"
    NO_DEFAULT_PATH
)

if (NOT CHATSERVER_MUDUO_INCLUDE_DIR OR
    NOT CHATSERVER_MUDUO_NET_LIBRARY OR
    NOT CHATSERVER_MUDUO_BASE_LIBRARY)
    message(FATAL_ERROR
        "Muduo was not found under ${MUDUO_ROOT}. "
        "Install muduo locally (recommended: /usr/local) or set MUDUO_ROOT explicitly.")
endif()

get_filename_component(CHATSERVER_MUDUO_LIB_DIR
    "${CHATSERVER_MUDUO_NET_LIBRARY}" DIRECTORY)

add_library(chatserver_muduo INTERFACE)
target_include_directories(chatserver_muduo INTERFACE "${CHATSERVER_MUDUO_INCLUDE_DIR}")
target_link_libraries(chatserver_muduo INTERFACE
    "${CHATSERVER_MUDUO_NET_LIBRARY}"
    "${CHATSERVER_MUDUO_BASE_LIBRARY}"
)

# ── nlohmann_json ──────────────────────────────────────────────────────────────

find_package(nlohmann_json 3 REQUIRED)

# ── MySQL client ───────────────────────────────────────────────────────────────

find_library(CHATSERVER_MYSQLCLIENT_LIBRARY mysqlclient)
if (NOT CHATSERVER_MYSQLCLIENT_LIBRARY)
    message(FATAL_ERROR "mysqlclient library not found. Install default-libmysqlclient-dev.")
endif()

# ── hiredis ────────────────────────────────────────────────────────────────────

find_library(CHATSERVER_HIREDIS_LIBRARY hiredis)
if (NOT CHATSERVER_HIREDIS_LIBRARY)
    message(FATAL_ERROR "hiredis library not found. Install libhiredis-dev.")
endif()

# ── RPC 框架（mprpc + protobuf + zookeeper_mt）────────────────────────────────

include(${CMAKE_SOURCE_DIR}/cmake/Mprpc.cmake)
