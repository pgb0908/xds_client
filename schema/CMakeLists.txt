
#include("/home/bont/CLionProjects/xds_client/cmake/FindGRPC.cmake")
#include("/home/bont/CLionProjects/xds_client/cmake/FindProtobuf.cmake")


# gRPC
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
set(_PROTOBUF_PROTOC ${CONAN_BIN_DIRS_PROTOBUF}/protoc)
message(STATUS "_PROTOBUF_PROTOC ${_PROTOBUF_PROTOC}")
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

set (proto_name greetings)


# Get proto files
get_filename_component(proto "proto/${proto_name}.proto" ABSOLUTE)
get_filename_component(proto_dir "${proto}" DIRECTORY)

set(CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CURRENT_DIR}/proto)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

# Generate source files
#[[set(proto_srcs "${PROTO_SRC_DIR}/${proto_name}.pb.cc")
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
)]]
#SET_SOURCE_FILES_PROPERTIES(${proto_srcs} ${proto_hdrs} PROPERTIES GENERATED TRUE)

protobuf_generate_cpp(proto_srcs proto_hdrs ${PROTO_SRC_DIR} ${proto})
grpc_generate_cpp(grpc_srcs grpc_hdrs ${PROTO_SRC_DIR} ${proto})

include_directories(CURRENT_DIR)
add_library(target STATIC
        greetings_server.cpp
        greetings_client.cpp
        ${proto_srcs}
        ${grpc_srcs}
)

target_link_libraries(target PRIVATE ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})



#[[foreach (_target IN LISTS PROTO_FILES)
    get_filename_component(_directory ${_target} DIRECTORY)
    get_filename_component(_filename ${_target} NAME_WLE)
    get_filename_component(PROTO_FILES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${_target}" PATH)
    file(MAKE_DIRECTORY "${proto_dest}/${_directory}")

    set(target_proto_srcs "${proto_dest}/${_directory}/${_filename}.pb.cc")
    set(target_proto_hdrs "${proto_dest}/${_directory}/${_filename}.pb.h")

    add_custom_command(
            OUTPUT "${target_proto_srcs}" "${target_proto_hdrs}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --cpp_out ${proto_dest}
            -I"${CMAKE_CURRENT_SOURCE_DIR}/xds_api"
            --proto_path ${PROTO_FILES_PATH}
            ${CMAKE_CURRENT_SOURCE_DIR}/${_target}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_target}
            COMMENT "Generating C++ from ${_filename}.proto"
    )

    LIST(APPEND PROTO_SRCS ${target_proto_srcs})
endforeach()]]


#[[add_custom_command(
        OUTPUT ${OUTPUT_SOURCES}
        COMMAND ${_PROTOBUF_PROTOC}
        --cpp_out='/home/bont/CLionProjects/xds_client/schema/target'
        -I'/home/bont/CLionProjects/xds_client/schema/xds_api/aa'
        --proto_path='/home/bont/CLionProjects/xds_client/schema/xds_api/aa'
        "/home/bont/CLionProjects/xds_client/schema/xds_api/aa/test.proto"
        DEPENDS "/home/bont/CLionProjects/xds_client/schema/xds_api/aa/test.proto"
        COMMENT "Generating C++ from test.proto"
)]]




#[[foreach (_target IN LISTS PROTO_FILES_GRPC)
    get_filename_component(_directory ${_target} DIRECTORY)
    get_filename_component(_filename ${_target} NAME_WLE)
    get_filename_component(PROTO_FILES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${_target}" PATH)
    file(MAKE_DIRECTORY "${proto_dest}/${_directory}")

    set(target_grpc_srcs "${proto_dest}/${_directory}/${_filename}.grpc.pb.cc")
    set(target_grpc_hdrs "${proto_dest}/${_directory}/${_filename}.grpc.pb.h")

    add_custom_command(
            OUTPUT "${target_grpc_srcs}" "${target_grpc_hdrs}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${proto_dest}"
            -I"${CMAKE_CURRENT_SOURCE_DIR}/xds_api"
            --plugin=protoc-gen-grpc="${_gRPC_CPP_PLUGIN}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${_target}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_target}"
    )

    LIST(APPEND GRPC_SRCS ${target_grpc_srcs})
    LIST(APPEND GRPC_HDRS ${target_grpc_hdrs})
endforeach()]]

#[[ADD_LIBRARY(anylinkSchema SHARED
        /home/bont/CLionProjects/xds_client/schema/target/test.pb.cc
)]]

#[[
TARGET_LINK_LIBRARIES(anylinkSchema
        protobuf
)]]
