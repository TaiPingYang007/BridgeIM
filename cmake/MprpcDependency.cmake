# MprpcDependency.cmake
# 查找 mprpc 静态库、protobuf、zookeeper_mt，定义 bridgeim_mprpc imported target。

# 解析 mprpc 安装前缀，优先级从高到低：
#   1. -DMPRPC_ROOT=...
#   2. 环境变量 MPRPC_ROOT
#   3. submodule 构建产物 third_party/mprpc/dist（见 scripts/build/mprpc.sh）
#   4. 姊妹项目 ../03_rpc_framework/dist/mprpc（本地并排开发默认）
if (NOT DEFINED MPRPC_ROOT OR MPRPC_ROOT STREQUAL "")
    if (DEFINED ENV{MPRPC_ROOT} AND NOT "$ENV{MPRPC_ROOT}" STREQUAL "")
        set(MPRPC_ROOT "$ENV{MPRPC_ROOT}" CACHE PATH "mprpc installation prefix.")
    elseif (EXISTS "${CMAKE_SOURCE_DIR}/third_party/mprpc/dist/include/mprpcchannel.h")
        set(MPRPC_ROOT "${CMAKE_SOURCE_DIR}/third_party/mprpc/dist" CACHE PATH
            "mprpc installation prefix.")
    else()
        set(MPRPC_ROOT "${CMAKE_SOURCE_DIR}/../03_rpc_framework/dist/mprpc" CACHE PATH
            "mprpc installation prefix.")
    endif()
endif()

find_path(BRIDGEIM_MPRPC_INCLUDE_DIR
    NAMES mprpcchannel.h
    HINTS "${MPRPC_ROOT}/include"
    NO_DEFAULT_PATH
)
find_library(BRIDGEIM_MPRPC_LIBRARY
    NAMES mprpc
    HINTS "${MPRPC_ROOT}/lib"
    NO_DEFAULT_PATH
)
find_library(BRIDGEIM_PROTOBUF_LIBRARY protobuf)
find_library(BRIDGEIM_ZOOKEEPER_LIBRARY zookeeper_mt)

if (NOT BRIDGEIM_MPRPC_INCLUDE_DIR OR NOT BRIDGEIM_MPRPC_LIBRARY)
    message(FATAL_ERROR
        "mprpc was not found under ${MPRPC_ROOT}.\n"
        "Provide it via one of: -DMPRPC_ROOT=<path>, env MPRPC_ROOT, "
        "a vendored build at third_party/mprpc/dist (include/ + lib/), or build "
        "03_rpc_framework into ../03_rpc_framework/dist/mprpc. "
        "See README sections 'Build' and 'Troubleshooting'.")
endif()
if (NOT BRIDGEIM_PROTOBUF_LIBRARY)
    message(FATAL_ERROR "protobuf library not found. Install protobuf development libraries.")
endif()
if (NOT BRIDGEIM_ZOOKEEPER_LIBRARY)
    message(FATAL_ERROR "zookeeper_mt library not found. Install ZooKeeper C client development libraries.")
endif()

add_library(bridgeim_mprpc STATIC IMPORTED)
set_target_properties(bridgeim_mprpc PROPERTIES
    IMPORTED_LOCATION "${BRIDGEIM_MPRPC_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${BRIDGEIM_MPRPC_INCLUDE_DIR}"
)
