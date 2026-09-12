cmake_minimum_required(VERSION 3.15)
project(WordCounter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(app
    src/file_io.cpp
    src/main.cpp
    src/word_counter
)

target_link_libraries(app PRIVATE Threads::Threads)