set(
    BRIDGEIM_COMMON_DB_SRC
    db/MySQL.cpp
    db/ConnectionPool.cpp
    db/ConnectionPoolConfig.cpp
    db/DbSession.cpp
    db/QueryResult.cpp
)

find_library(BRIDGEIM_COMMON_MYSQLCLIENT_LIBRARY mysqlclient)
if (NOT BRIDGEIM_COMMON_MYSQLCLIENT_LIBRARY)
    message(FATAL_ERROR "mysqlclient library not found. Install default-libmysqlclient-dev.")
endif()

add_library(bridgeim_common_db STATIC ${BRIDGEIM_COMMON_DB_SRC})
target_include_directories(bridgeim_common_db PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(bridgeim_common_db PUBLIC
    ${BRIDGEIM_COMMON_MYSQLCLIENT_LIBRARY}
    Threads::Threads
)

set(
    BRIDGEIM_COMMON_CONCURRENCY_SRC
    concurrency/ThreadPool.cpp
)

add_library(bridgeim_common_concurrency STATIC ${BRIDGEIM_COMMON_CONCURRENCY_SRC})
target_include_directories(bridgeim_common_concurrency PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(bridgeim_common_concurrency PUBLIC
    Threads::Threads
)

add_library(bridgeim_common_rpc INTERFACE)
target_include_directories(bridgeim_common_rpc INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

add_library(bridgeim_common INTERFACE)
target_link_libraries(bridgeim_common INTERFACE
    bridgeim_common_db
    bridgeim_common_concurrency
    bridgeim_common_rpc
)
