cmake_minimum_required(VERSION 3.16)

project(lab2_markdown
        VERSION 1.0
        DESCRIPTION "Лабораторная работа №2. Вариант 2: Markdown"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(lab2_core STATIC
    ${PROJECT_SRC_DIR}/markdown_analyzer.cpp
    ${PROJECT_SRC_DIR}/person.cpp
    ${PROJECT_SRC_DIR}/random_generator.cpp
)

target_include_directories(lab2_core PUBLIC ${PROJECT_SRC_DIR})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lab2_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(lab2 ${PROJECT_SRC_DIR}/main.cpp)
target_link_libraries(lab2 PRIVATE lab2_core)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lab2 PRIVATE -Wall -Wextra -Wpedantic)
endif()