add_executable(FriendService
    main.cpp
)

set_target_properties(FriendService PROPERTIES
    BUILD_RPATH "${CHATSERVER_MUDUO_LIB_DIR}"
)

target_include_directories(FriendService PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/include/server/model
)

target_link_libraries(FriendService PRIVATE
    bridgeim_common
    bridgeim_server_models
)
