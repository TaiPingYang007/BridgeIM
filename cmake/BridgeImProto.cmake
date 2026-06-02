# BridgeImProto.cmake
# 用 protoc 自动生成 .pb.h/.pb.cc，编译为 bridgeim_proto_* 静态库。

set(BRIDGEIM_GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated/proto")
file(MAKE_DIRECTORY "${BRIDGEIM_GENERATED_PROTO_DIR}")

set(BRIDGEIM_OFFLINE_MESSAGE_PROTO "${PROJECT_SOURCE_DIR}/proto/offline_message.proto")
set(BRIDGEIM_OFFLINE_MESSAGE_PROTO_SRC
    "${BRIDGEIM_GENERATED_PROTO_DIR}/offline_message.pb.cc")
set(BRIDGEIM_OFFLINE_MESSAGE_PROTO_HDR
    "${BRIDGEIM_GENERATED_PROTO_DIR}/offline_message.pb.h")

add_custom_command(
    OUTPUT
        "${BRIDGEIM_OFFLINE_MESSAGE_PROTO_SRC}"
        "${BRIDGEIM_OFFLINE_MESSAGE_PROTO_HDR}"
    COMMAND protobuf::protoc
        --proto_path=${PROJECT_SOURCE_DIR}/proto
        --cpp_out=${BRIDGEIM_GENERATED_PROTO_DIR}
        ${BRIDGEIM_OFFLINE_MESSAGE_PROTO}
    DEPENDS "${BRIDGEIM_OFFLINE_MESSAGE_PROTO}"
    COMMENT "Generating offline_message protobuf sources"
    VERBATIM
)

add_library(bridgeim_proto_offline_message STATIC
    "${BRIDGEIM_OFFLINE_MESSAGE_PROTO_SRC}"
)
target_include_directories(bridgeim_proto_offline_message PUBLIC
    "${BRIDGEIM_GENERATED_PROTO_DIR}"
)
target_link_libraries(bridgeim_proto_offline_message PUBLIC
    protobuf::libprotobuf
)

# ── friend_service.proto ────────────────────────────────────────────────────────

set(BRIDGEIM_FRIEND_SERVICE_PROTO "${PROJECT_SOURCE_DIR}/proto/friend_service.proto")
set(BRIDGEIM_FRIEND_SERVICE_PROTO_SRC
    "${BRIDGEIM_GENERATED_PROTO_DIR}/friend_service.pb.cc")
set(BRIDGEIM_FRIEND_SERVICE_PROTO_HDR
    "${BRIDGEIM_GENERATED_PROTO_DIR}/friend_service.pb.h")

add_custom_command(
    OUTPUT
        "${BRIDGEIM_FRIEND_SERVICE_PROTO_SRC}"
        "${BRIDGEIM_FRIEND_SERVICE_PROTO_HDR}"
    COMMAND protobuf::protoc
        --proto_path=${PROJECT_SOURCE_DIR}/proto
        --cpp_out=${BRIDGEIM_GENERATED_PROTO_DIR}
        ${BRIDGEIM_FRIEND_SERVICE_PROTO}
    DEPENDS "${BRIDGEIM_FRIEND_SERVICE_PROTO}"
    COMMENT "Generating friend_service protobuf sources"
    VERBATIM
)

add_library(bridgeim_proto_friend_service STATIC
    "${BRIDGEIM_FRIEND_SERVICE_PROTO_SRC}"
)
target_include_directories(bridgeim_proto_friend_service PUBLIC
    "${BRIDGEIM_GENERATED_PROTO_DIR}"
)
target_link_libraries(bridgeim_proto_friend_service PUBLIC
    protobuf::libprotobuf
)
