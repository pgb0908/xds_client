find_package(Threads)
add_executable(asnyc_ads main.cpp AsyncAdsClient.cpp)
target_link_libraries(asnyc_ads PUBLIC
        target Threads::Threads)