add_executable(OfflineMessageService
    main.cpp
)

set_target_properties(OfflineMessageService PROPERTIES
    BUILD_RPATH "${CHATSERVER_MUDUO_LIB_DIR}"
)

target_include_directories(OfflineMessageService PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/include/server/model
)

target_link_libraries(OfflineMessageService PRIVATE
    bridgeim_common
    bridgeim_server_models
)
