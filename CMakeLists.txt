cmake_minimum_required(VERSION 3.16)

project(multi_thread_file_indexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

# SQLite is used for the persistent index store (--save / --load).
find_library(SQLITE3_LIBRARY NAMES sqlite3)
if(NOT SQLITE3_LIBRARY)
    message(FATAL_ERROR "libsqlite3 not found. Install it (e.g. 'brew install sqlite' or 'apt install libsqlite3-dev').")
endif()

# --- Main executable ---
add_executable(multi_thread_file_indexer
    src/main.cpp
)
target_include_directories(multi_thread_file_indexer PRIVATE include)
target_link_libraries(multi_thread_file_indexer PRIVATE Threads::Threads ${SQLITE3_LIBRARY})
target_compile_options(multi_thread_file_indexer PRIVATE -Wall -Wextra)

# --- Unit tests ---
enable_testing()
add_executable(unit_tests
    tests/test_main.cpp
)
target_include_directories(unit_tests PRIVATE include)
target_link_libraries(unit_tests PRIVATE Threads::Threads ${SQLITE3_LIBRARY})
target_compile_options(unit_tests PRIVATE -Wall -Wextra)
add_test(NAME unit_tests COMMAND unit_tests)

# --- Optional ThreadSanitizer build of the tests ---
# Configure with:  cmake -DENABLE_TSAN=ON ..
option(ENABLE_TSAN "Build unit_tests with ThreadSanitizer" OFF)
if(ENABLE_TSAN)
    target_compile_options(unit_tests PRIVATE -fsanitize=thread -g -O1)
    target_link_options(unit_tests PRIVATE -fsanitize=thread)
endif()
