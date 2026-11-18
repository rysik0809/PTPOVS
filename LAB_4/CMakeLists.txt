cmake_minimum_required(VERSION 3.16)

project(lab4_markdown
        VERSION 1.0
        DESCRIPTION "Лабораторная работа №4. Многопоточность и асинхронность"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Код Лабораторной №2/№3
set(LAB23_SOURCES
    ${SRC_DIR}/markdown_analyzer.cpp
    ${SRC_DIR}/person.cpp
    ${SRC_DIR}/random_generator.cpp
)

# Новый код Лабораторной №4
set(LAB4_SOURCES
    ${SRC_DIR}/file_result.cpp
    ${SRC_DIR}/file_processor.cpp
    ${SRC_DIR}/file_watcher.cpp
    ${SRC_DIR}/worker_pool.cpp
    ${SRC_DIR}/async_processor.cpp
    ${SRC_DIR}/console_ui.cpp
)

add_executable(lab4
    ${LAB23_SOURCES}
    ${LAB4_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_include_directories(lab4 PRIVATE ${SRC_DIR})

find_package(Threads REQUIRED)
target_link_libraries(lab4 PRIVATE Threads::Threads)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lab4 PRIVATE -Wall -Wextra -Wpedantic)
endif()