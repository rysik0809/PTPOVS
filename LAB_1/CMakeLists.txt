cmake_minimum_required(VERSION 3.16)
project(LAB_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Threads REQUIRED)

# --- nlohmann/json ---
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)
# ---------------------

add_executable(app
    src/main.cpp
    src/text_utils.cpp
    src/text_utils.hpp
    src/word_stats.cpp
    src/word_stats.hpp
    src/algo_tasks.cpp
    src/algo_tasks.hpp
    src/i18n.cpp
    src/i18n.hpp
)

target_include_directories(app PRIVATE src)
target_link_libraries(app PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/locales
            $<TARGET_FILE_DIR:app>/locales
    COMMENT "Копирую locales/ в каталог сборки"
)

if (MSVC)
    target_compile_options(app PRIVATE /W4)
else()
    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
endif()