find_package(Threads)
add_executable(ads main.cpp AdsClient.cpp )
target_link_libraries(ads PUBLIC
        target Threads::Threads)