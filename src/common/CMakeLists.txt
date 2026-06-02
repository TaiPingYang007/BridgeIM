# src/common/CMakeLists.txt
# 只负责 common 层三个子库：
#   bridgeim_common_db         — MySQL 连接池、DbSession、QueryResult
#   bridgeim_common_concurrency — 业务线程池
#   bridgeim_common_rpc        — RPC 框架接口（链接外部 mprpc + proto + Muduo + ZK）
# 以及聚合接口 target bridgeim_common。

# ── bridgeim_common_db ─────────────────────────────────────────────────────────

add_library(bridgeim_common_db STATIC
    db/MySQL.cpp
    db/ConnectionPool.cpp
    db/ConnectionPoolConfig.cpp
    db/DbSession.cpp
    db/QueryResult.cpp
)
target_include_directories(bridgeim_common_db PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bridgeim_common_db PUBLIC
    ${CHATSERVER_MYSQLCLIENT_LIBRARY}
    Threads::Threads
)

# ── bridgeim_common_concurrency ────────────────────────────────────────────────

add_library(bridgeim_common_concurrency STATIC
    concurrency/ThreadPool.cpp
)
target_include_directories(bridgeim_common_concurrency PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bridgeim_common_concurrency PUBLIC Threads::Threads)

# ── bridgeim_common_rpc ────────────────────────────────────────────────────────

add_library(bridgeim_common_rpc INTERFACE)
target_link_libraries(bridgeim_common_rpc INTERFACE
    bridgeim_mprpc
    bridgeim_proto_offline_message
    chatserver_muduo
    ${BRIDGEIM_PROTOBUF_LIBRARY}
    ${BRIDGEIM_ZOOKEEPER_LIBRARY}
    Threads::Threads
)

# ── bridgeim_common（聚合接口）────────────────────────────────────────────────

add_library(bridgeim_common INTERFACE)
target_link_libraries(bridgeim_common INTERFACE
    bridgeim_common_db
    bridgeim_common_concurrency
    bridgeim_common_rpc
)

