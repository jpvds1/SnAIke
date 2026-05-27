cmake_minimum_required(VERSION 3.18)
project(gym_snake_test LANGUAGES CXX)

# Force C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define Include Directories (target-specific is preferred over global include_directories)
# We use BEFORE to make sure your headers take priority
include_directories(BEFORE include)

# Gather all source files (explicitly relative to this CMakeLists file)
set(SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/snake.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

# Build a standalone native executable
add_executable(snake_environment_test ${SOURCE_FILES})

# Compiler optimizations
target_compile_options(snake_environment_test PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CXX_COMPILER_ID:MSVC>:/O2>
)