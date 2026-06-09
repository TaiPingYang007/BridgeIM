# ProtobufTargets.cmake
# 用 protoc 自动生成 .pb.h/.pb.cc，并编译为 bridgeim_proto_* 静态库。

set(BRIDGEIM_GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated/proto")
file(MAKE_DIRECTORY "${BRIDGEIM_GENERATED_PROTO_DIR}")

function(bridgeim_add_proto_library target_name proto_file)
    get_filename_component(proto_stem "${proto_file}" NAME_WE)
    set(proto_src "${BRIDGEIM_GENERATED_PROTO_DIR}/${proto_stem}.pb.cc")
    set(proto_hdr "${BRIDGEIM_GENERATED_PROTO_DIR}/${proto_stem}.pb.h")

    add_custom_command(
        OUTPUT
            "${proto_src}"
            "${proto_hdr}"
        COMMAND protobuf::protoc
            --proto_path=${PROJECT_SOURCE_DIR}/proto
            --cpp_out=${BRIDGEIM_GENERATED_PROTO_DIR}
            "${proto_file}"
        DEPENDS "${proto_file}"
        COMMENT "Generating ${proto_stem} protobuf sources"
        VERBATIM
    )

    add_library(${target_name} STATIC "${proto_src}")
    target_include_directories(${target_name} PUBLIC
        "${BRIDGEIM_GENERATED_PROTO_DIR}"
    )
    target_link_libraries(${target_name} PUBLIC
        protobuf::libprotobuf
    )
endfunction()

bridgeim_add_proto_library(
    bridgeim_proto_offline_message
    "${PROJECT_SOURCE_DIR}/proto/offline_message.proto"
)

bridgeim_add_proto_library(
    bridgeim_proto_friend_service
    "${PROJECT_SOURCE_DIR}/proto/friend_service.proto"
)
