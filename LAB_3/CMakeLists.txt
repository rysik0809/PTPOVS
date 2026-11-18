cmake_minimum_required(VERSION 3.16)

project(lab3_markdown
        VERSION 1.0
        DESCRIPTION "Лабораторная работа №3"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_SRC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_DEMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/demo)

set(LAB3_SOURCES
    ${PROJECT_SRC_DIR}/markdown_analyzer.cpp
    ${PROJECT_SRC_DIR}/person.cpp
    ${PROJECT_SRC_DIR}/random_generator.cpp
)

add_library(lab3_static STATIC ${LAB3_SOURCES})
target_include_directories(lab3_static PUBLIC ${PROJECT_SRC_DIR})
target_compile_features(lab3_static PUBLIC cxx_std_17)

add_library(lab3_shared SHARED ${LAB3_SOURCES})
target_include_directories(lab3_shared PUBLIC ${PROJECT_SRC_DIR})
target_compile_features(lab3_shared PUBLIC cxx_std_17)
target_compile_definitions(lab3_shared PRIVATE LAB3_BUILD_SHARED)

add_executable(demo_static ${PROJECT_DEMO_DIR}/main.cpp)
target_link_libraries(demo_static PRIVATE lab3_static)

add_executable(demo_shared ${PROJECT_DEMO_DIR}/main.cpp)
target_compile_definitions(demo_shared PRIVATE LAB3_USE_SHARED)
target_link_libraries(demo_shared PRIVATE lab3_shared)

if (WIN32)
    add_custom_command(TARGET demo_shared POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:lab3_shared>
            $<TARGET_FILE_DIR:demo_shared>
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target lab3_static lab3_shared demo_static demo_shared)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()