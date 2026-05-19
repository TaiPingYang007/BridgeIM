add_library(bridgeim_common_db INTERFACE)
target_include_directories(bridgeim_common_db INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

add_library(bridgeim_common_concurrency INTERFACE)
target_include_directories(bridgeim_common_concurrency INTERFACE
    ${CMAKE_SOURCE_DIR}/include
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
