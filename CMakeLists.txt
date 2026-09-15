cmake_minimum_required(VERSION 3.16)

project(multi_thread_file_indexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_executable(multi_thread_file_indexer
    src/main.cpp
)

target_include_directories(multi_thread_file_indexer PRIVATE include)

target_link_libraries(multi_thread_file_indexer PRIVATE Threads::Threads)