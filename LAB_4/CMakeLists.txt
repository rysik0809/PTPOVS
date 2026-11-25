cmake_minimum_required(VERSION 3.16)

project(lab4_markdown
        VERSION 1.0
        DESCRIPTION "Лабораторная работа №4. Многопоточность и асинхронность"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# =========================================================================
# 1. ПОДКЛЮЧАЕМ 3-Ю ЛАБОРАТОРНУЮ РАБОТУ
# =========================================================================
# Указываем путь к папке, где лежит CMakeLists.txt 3-й лабы.
# Скорее всего, папки LAB_3 и LAB_4 лежат рядом.
add_subdirectory(../LAB_3 ${CMAKE_CURRENT_BINARY_DIR}/lab3_build)

# =========================================================================
# 2. СОБИРАЕМ 4-Ю ЛАБОРАТОРНУЮ РАБОТУ
# =========================================================================
set(LAB4_SOURCES
    ${SRC_DIR}/file_result.cpp
    ${SRC_DIR}/file_processor.cpp
    ${SRC_DIR}/file_watcher.cpp
    ${SRC_DIR}/worker_pool.cpp
    ${SRC_DIR}/async_processor.cpp
    ${SRC_DIR}/console_ui.cpp
)

add_executable(lab4
    ${LAB4_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# Говорим 4-й лабе, где искать её собственные заголовочные файлы (.h)
target_include_directories(lab4 PRIVATE ${SRC_DIR})

# =========================================================================
# 3. ЛИНКУЕМ 4-Ю ЛАБУ С БИБЛИОТЕКОЙ ИЗ 3-Й
# =========================================================================
# Указываем имя цели из 3-й лабы. Используем статическую библиотеку.
target_link_libraries(lab4 PRIVATE lab3_static)

find_package(Threads REQUIRED)
target_link_libraries(lab4 PRIVATE Threads::Threads)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lab4 PRIVATE -Wall -Wextra -Wpedantic)
endif()