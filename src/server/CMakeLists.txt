set(SERVER_SOURCES
    main.cpp
    logger.cpp
    chatserver.cpp
    chatservice.cpp
    db/db.cpp
    model/usermodel.cpp
    model/offlinemessagemodel.cpp
    model/friendmodel.cpp
    model/friendrequestmodel.cpp
    model/groupmodel.cpp
    model/grouprequestmodel.cpp
    redis/redis.cpp
)

find_package(Threads REQUIRED)

add_executable(ChatServer ${SERVER_SOURCES})

target_include_directories(ChatServer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/server
    ${CMAKE_SOURCE_DIR}/include/server/model
    ${CMAKE_SOURCE_DIR}/include/server/db
    ${CMAKE_SOURCE_DIR}/include/server/redis
)

target_compile_definitions(ChatServer PRIVATE
    CHATSERVER_LOG_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

target_link_libraries(ChatServer PRIVATE
    muduo_net
    muduo_base
    mysqlclient
    hiredis
    Threads::Threads
)
