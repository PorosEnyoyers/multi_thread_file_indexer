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

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(indexer_tests tests/test_indexer.cpp)
target_include_directories(indexer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/Custom_Container_Library/include
)
target_link_libraries(indexer_tests PRIVATE Threads::Threads GTest::gtest_main)

# target_compile_options(indexer_tests PRIVATE -fsanitize=address,undefined -g -fno-omit-frame-pointer)
# target_link_options(indexer_tests PRIVATE -fsanitize=address,undefined)

include(GoogleTest)
gtest_discover_tests(indexer_tests)

# target_compile_options(multi_thread_file_indexer PRIVATE -fsanitize=address,undefined -g -fno-omit-frame-pointer)
# target_link_options(multi_thread_file_indexer PRIVATE -fsanitize=address,undefined)

set(SANITIZE "address,undefined" CACHE STRING "Value for -fsanitize=")
foreach(tgt multi_thread_file_indexer indexer_tests)
    target_compile_options(${tgt} PRIVATE -fsanitize=${SANITIZE} -g -fno-omit-frame-pointer)
    target_link_options(${tgt} PRIVATE -fsanitize=${SANITIZE})
endforeach()