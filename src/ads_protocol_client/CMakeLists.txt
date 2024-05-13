include_directories(${CMAKE_CURRENT_SOURCE_DIR}/proto-src/proto)

find_package(Threads)
add_executable(ads_protocol_client main.cpp AdsClient.cpp )
target_link_libraries(ads_protocol_client PUBLIC
        target Threads::Threads)