find_package(Threads)
add_executable(main main.cpp)
target_link_libraries(main PUBLIC
        target Threads::Threads)