cmake_minimum_required(VERSION 3.16)

project(LAB_1 LANGUAGES CXX)

# C++17 нужен для std::string_view
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Все исходники
add_executable(app
    src/main.cpp
    src/ui.cpp
    src/file_io.cpp
    src/utf8_utils.cpp
    src/word_frequency.cpp
)

# Чтобы #include "ui.hpp" и остальные заголовки находились из src/
target_include_directories(app PRIVATE include)

# Предупреждения (не обязательно, но полезно)
if (MSVC)
    target_compile_options(app PRIVATE /W4)
else()
    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
endif()