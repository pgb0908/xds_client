
# gRPC
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
set(_PROTOBUF_PROTOC ${CONAN_BIN_DIRS_PROTOBUF}/protoc)
message(STATUS "_PROTOBUF_PROTOC ${_PROTOBUF_PROTOC}")
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

set(proto_dest "${CMAKE_CURRENT_SOURCE_DIR}/proto-src")

macro(proto_generate _PATH _INCLUDE _GLOB)
    file(${_GLOB} PROTO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${_PATH}/*.proto)
    foreach (_target IN LISTS PROTO_FILES)
        get_filename_component(_directory ${_target} DIRECTORY)
        get_filename_component(_filename ${_target} NAME_WLE)
        get_filename_component(PROTO_FILES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${_target}" PATH)
        file(MAKE_DIRECTORY "${proto_dest}/${_directory}")

        set(target_proto_srcs "${proto_dest}/${_directory}/${_filename}.pb.cc")
        set(target_proto_hdrs "${proto_dest}/${_directory}/${_filename}.pb.h")

        add_custom_command(
                OUTPUT "${target_proto_srcs}" "${target_proto_hdrs}"
                COMMAND ${_PROTOBUF_PROTOC}
                ARGS --cpp_out "${proto_dest}/${_INCLUDE}"
                -I"${CMAKE_CURRENT_SOURCE_DIR}/${_INCLUDE}"
                --proto_path "${CMAKE_CURRENT_SOURCE_DIR}/proto"
                "${CMAKE_CURRENT_SOURCE_DIR}/${_target}"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_target}"
        )

        LIST(APPEND ENVOY_PROTO_SRCS ${target_proto_srcs})
        LIST(APPEND ENVOY_PROTO_HDRS ${target_proto_hdrs})
    endforeach()
endmacro(proto_generate)


proto_generate(proto proto GLOB_RECURSE)




#set (proto_name greetings)
set (proto_name xds)

# Get proto files
get_filename_component(proto "proto/${proto_name}.proto" ABSOLUTE)
get_filename_component(proto_dir "${proto}" DIRECTORY)

set(CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

# Generate source files
set(proto_srcs "${PROTO_SRC_DIR}/${proto_name}.pb.cc")
set(proto_hdrs "${PROTO_SRC_DIR}/${proto_name}.pb.h")
set(grpc_srcs "${PROTO_SRC_DIR}/${proto_name}.grpc.pb.cc")
set(grpc_hdrs "${PROTO_SRC_DIR}/${proto_name}.grpc.pb.h")
add_custom_command(
        OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
        COMMAND
        ${_PROTOBUF_PROTOC} ARGS "--grpc_out=${PROTO_SRC_DIR}"
        "--cpp_out=${PROTO_SRC_DIR}" "--proto_path=${proto_dir}"
        "--plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${proto}"
        DEPENDS "${proto}"
        COMMENT "Generating C++ from hello.proto"
)

#[[protobuf_generate_cpp(proto_srcs proto_hdrs ${PROTO_SRC_DIR} ${proto})
grpc_generate_cpp(grpc_srcs grpc_hdrs ${PROTO_SRC_DIR} ${proto})]]

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/proto-src/proto)



add_library(target STATIC
        ../src/ads/AdsClient.cpp
        ../src/asnyc_ads/AsyncAdsClient.cpp
        ${proto_srcs}
        ${grpc_srcs}
        ${ENVOY_PROTO_SRCS}
)

TARGET_INCLUDE_DIRECTORIES(target PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/proto-src/proto
)


target_link_libraries(target PRIVATE ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
