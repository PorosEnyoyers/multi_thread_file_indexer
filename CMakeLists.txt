cmake_minimum_required(VERSION 3.20)
project(multi_thread_file_indexer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Threads REQUIRED)

add_executable(multi_thread_file_indexer
    src/main.cpp
    src/User_Interface.cpp
)

target_include_directories(multi_thread_file_indexer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/Custom_Container_Library/include
)

target_link_libraries(multi_thread_file_indexer PRIVATE
    Threads::Threads
)

target_compile_options(multi_thread_file_indexer PRIVATE
    -Wall -Wextra
)