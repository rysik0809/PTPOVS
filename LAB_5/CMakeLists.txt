cmake_minimum_required(VERSION 3.16)

project(lab5_markdown
        VERSION 1.0
        DESCRIPTION "Лабораторная работа №5. Обработка исключительных ситуаций"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# =========================================================================
# 1. ПОДКЛЮЧАЕМ 3-Ю ЛАБОРАТОРНУЮ РАБОТУ
# =========================================================================
add_subdirectory(../LAB_3 ${CMAKE_CURRENT_BINARY_DIR}/lab3_build)

# =========================================================================
# 2. СОБИРАЕМ 5-Ю ЛАБОРАТОРНУЮ РАБОТУ
# =========================================================================
set(LAB5_SOURCES
    ${SRC_DIR}/exceptions.cpp
    ${SRC_DIR}/file_result.cpp
    ${SRC_DIR}/file_processor.cpp
    ${SRC_DIR}/file_watcher.cpp
    ${SRC_DIR}/worker_pool.cpp
    ${SRC_DIR}/async_processor.cpp
    ${SRC_DIR}/console_ui.cpp
)

add_executable(lab5
    ${LAB5_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_include_directories(lab5 PRIVATE ${SRC_DIR})

# =========================================================================
# 3. ЛИНКУЕМ С БИБЛИОТЕКОЙ ИЗ 3-Й ЛАБОРАТОРНОЙ
# =========================================================================
target_link_libraries(lab5 PRIVATE lab3_static)

find_package(Threads REQUIRED)
target_link_libraries(lab5 PRIVATE Threads::Threads)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lab5 PRIVATE -Wall -Wextra -Wpedantic)
endif()