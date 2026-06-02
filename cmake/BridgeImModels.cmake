# BridgeImModels.cmake
# 将 server 层共享 model 源文件编译为 bridgeim_server_models 静态库，
# 供 ChatServer 和各 RPC 服务进程共同链接。

add_library(bridgeim_server_models STATIC
    ${CMAKE_SOURCE_DIR}/src/server/model/usermodel.cpp
    ${CMAKE_SOURCE_DIR}/src/server/model/offlinemessagemodel.cpp
    ${CMAKE_SOURCE_DIR}/src/server/model/friendmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/server/model/friendrequestmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/server/model/groupmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/server/model/grouprequestmodel.cpp
)

target_include_directories(bridgeim_server_models PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/server
    ${CMAKE_SOURCE_DIR}/include/server/model
)

target_link_libraries(bridgeim_server_models PUBLIC
    bridgeim_common
)
