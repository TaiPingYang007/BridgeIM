# src/server/CMakeLists.txt
# 只负责 ChatServer 可执行文件。
# model 库在 cmake/BridgeImModels.cmake 中定义。
# 外部依赖在 cmake/ProjectDependencies.cmake 中统一查找。

set(SRC_FILES
    main.cpp
    logger.cpp
    chatserver.cpp
    chatservice.cpp
    redis/redis.cpp
)

add_executable(ChatServer ${SRC_FILES})

target_include_directories(ChatServer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/server
    ${CMAKE_SOURCE_DIR}/include/server/model
    ${CMAKE_SOURCE_DIR}/include/server/redis
)

target_compile_definitions(ChatServer PRIVATE
    CHATSERVER_LOG_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_target_properties(ChatServer PROPERTIES
    BUILD_RPATH "${CHATSERVER_MUDUO_LIB_DIR}"
)

target_link_libraries(ChatServer PRIVATE
    bridgeim_common
    bridgeim_server_models
    chatserver_muduo
    nlohmann_json::nlohmann_json
    ${CHATSERVER_MYSQLCLIENT_LIBRARY}
    ${CHATSERVER_HIREDIS_LIBRARY}
    Threads::Threads
)

