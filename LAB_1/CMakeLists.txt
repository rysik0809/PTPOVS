cmake_minimum_required(VERSION 3.16)
project(LAB_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(app
    src/main.cpp
    src/text_utils.cpp
    src/word_stats.cpp
    src/algo_tasks.cpp
)

target_include_directories(app PRIVATE include)
target_link_libraries(app PRIVATE Threads::Threads)

if (MSVC)
    target_compile_options(app PRIVATE /W4)
else()
    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
endif()