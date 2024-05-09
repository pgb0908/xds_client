find_package(Threads)
add_executable(ads main.cpp )
target_link_libraries(ads PUBLIC
        AdsClient.cpp
        target Threads::Threads)