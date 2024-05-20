
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


#[[
proto_generate(proto/envoy/admin proto GLOB_RECURSE)
proto_generate(proto/envoy/annotations proto GLOB_RECURSE)
proto_generate(proto/envoy/api proto GLOB_RECURSE)
proto_generate(proto/envoy/config proto GLOB_RECURSE)
proto_generate(proto/envoy/data proto GLOB_RECURSE)
proto_generate(proto/envoy/extensions proto GLOB_RECURSE)
proto_generate(proto/envoy/service/accesslog proto GLOB_RECURSE)
proto_generate(proto/envoy/service/auth proto GLOB_RECURSE)
proto_generate(proto/envoy/service/cluster proto GLOB_RECURSE)
proto_generate(proto/envoy/service/endpoint proto GLOB_RECURSE)
proto_generate(proto/envoy/service/event_reporting proto GLOB_RECURSE)
proto_generate(proto/envoy/service/ext_proc proto GLOB_RECURSE)
proto_generate(proto/envoy/service/extension proto GLOB_RECURSE)
proto_generate(proto/envoy/service/health proto GLOB_RECURSE)
proto_generate(proto/envoy/service/listener proto GLOB_RECURSE)
proto_generate(proto/envoy/service/load_stats proto GLOB_RECURSE)
proto_generate(proto/envoy/service/metrics proto GLOB_RECURSE)
proto_generate(proto/envoy/service/rate_limit_quota proto GLOB_RECURSE)
proto_generate(proto/envoy/service/ratelimit proto GLOB_RECURSE)
proto_generate(proto/envoy/service/route proto GLOB_RECURSE)
proto_generate(proto/envoy/service/runtime proto GLOB_RECURSE)
proto_generate(proto/envoy/service/secret proto GLOB_RECURSE)
proto_generate(proto/envoy/service/status proto GLOB_RECURSE)
proto_generate(proto/envoy/service/tap proto GLOB_RECURSE)
proto_generate(proto/envoy/service/trace proto GLOB_RECURSE)
proto_generate(proto/envoy/service/discovery proto GLOB_RECURSE)

proto_generate(proto/envoy/type proto GLOB_RECURSE)
proto_generate(proto/envoy/watchdog proto GLOB_RECURSE)
proto_generate(proto/google proto GLOB_RECURSE)
proto_generate(proto/io proto GLOB_RECURSE)
proto_generate(proto/opencensus proto GLOB_RECURSE)
proto_generate(proto/opentelemetry proto GLOB_RECURSE)
proto_generate(proto/udpa proto GLOB_RECURSE)
proto_generate(proto/validate proto GLOB_RECURSE)
proto_generate(proto/xds proto GLOB_RECURSE)
]]


macro(proto_service_generate _PATH _INCLUDE _GLOB)
    file(${_GLOB} PROTO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${_PATH}/*.proto)
    foreach (_target IN LISTS PROTO_FILES)
        get_filename_component(_directory ${_target} DIRECTORY)
        get_filename_component(_filename ${_target} NAME_WLE)
        get_filename_component(PROTO_FILES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${_target}" PATH)
        file(MAKE_DIRECTORY "${proto_dest}/${_directory}")

        set(target_proto_srcs "${proto_dest}/${_directory}/${_filename}.pb.cc")
        set(target_proto_hdrs "${proto_dest}/${_directory}/${_filename}.pb.h")
        set(target_grpc_srcs "${proto_dest}/${_directory}/${_filename}.grpc.pb.cc")
        set(target_grpc_hdrs "${proto_dest}/${_directory}/${_filename}.grpc.pb.h")

        add_custom_command(
                OUTPUT "${target_proto_srcs}" "${target_proto_hdrs}" "${target_grpc_srcs}" "${target_grpc_hdrs}"
                COMMAND ${_PROTOBUF_PROTOC}
                ARGS --grpc_out "${proto_dest}/${_INCLUDE}"
                --cpp_out "${proto_dest}/${_INCLUDE}"
                -I"${CMAKE_CURRENT_SOURCE_DIR}/${_INCLUDE}"
                --proto_path "${CMAKE_CURRENT_SOURCE_DIR}/proto"
                "--plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}"  #이거 안해주면 에러 발생
                "${CMAKE_CURRENT_SOURCE_DIR}/${_target}"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_target}"
        )

        LIST(APPEND ENVOY_PROTO_SRCS ${target_proto_srcs})
        LIST(APPEND ENVOY_PROTO_HDRS ${target_proto_hdrs})
        LIST(APPEND ENVOY_PROTO_SRCS ${target_grpc_srcs})
        LIST(APPEND ENVOY_PROTO_HDRS ${target_grpc_hdrs})
    endforeach()
endmacro(proto_service_generate)

#[[protobuf_generate_cpp(proto_srcs proto_hdrs ${PROTO_SRC_DIR} ${proto})
grpc_generate_cpp(grpc_srcs grpc_hdrs ${PROTO_SRC_DIR} ${proto})]]
proto_service_generate(proto proto GLOB_RECURSE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/proto-src/proto)

add_library(target STATIC
        #../src/ads/AdsClient.cpp
        ../src/asnyc_ads/AsyncAdsClient.cpp
        ${ENVOY_PROTO_SRCS}
)

TARGET_INCLUDE_DIRECTORIES(target PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/proto-src/proto
)


target_link_libraries(target PRIVATE ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
