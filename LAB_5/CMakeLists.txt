cmake_minimum_required(VERSION 3.16)

project(lab5_markdown
        VERSION 1.0
        DESCRIPTION "Лабораторная работа №5. Обработка исключительных ситуаций"
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

# Код Лабораторной №4
set(LAB4_SOURCES
    ${SRC_DIR}/file_result.cpp
    ${SRC_DIR}/file_processor.cpp
    ${SRC_DIR}/file_watcher.cpp
    ${SRC_DIR}/worker_pool.cpp
    ${SRC_DIR}/async_processor.cpp
    ${SRC_DIR}/console_ui.cpp
)

# Код Лабораторной №5
set(LAB5_SOURCES
    ${SRC_DIR}/exceptions.cpp
    ${SRC_DIR}/logger.cpp
)

add_executable(lab5
    ${LAB23_SOURCES}
    ${LAB4_SOURCES}
    ${LAB5_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_include_directories(lab5 PRIVATE ${SRC_DIR})

find_package(Threads REQUIRED)
target_link_libraries(lab5 PRIVATE Threads::Threads)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lab5 PRIVATE -Wall -Wextra -Wpedantic)
endif()