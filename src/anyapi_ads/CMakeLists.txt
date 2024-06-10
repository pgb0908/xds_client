include_directories(${CMAKE_CURRENT_SOURCE_DIR}/proto-src/proto)

find_package(Threads)
add_executable(anyapi main.cpp AdsClient.cpp )
target_link_libraries(anyapi PUBLIC
        target Threads::Threads)