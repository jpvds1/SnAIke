cmake_minimum_required(VERSION 3.18)
project(gym_snake_test LANGUAGES CXX)

# Force C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_error)
if (_ipo_supported)
    message(STATUS "[snake] IPO/LTO: enabled")
else()
    message(WARNING "[snake] IPO/LTO not supported: $(_ipo_error)")
endif()

include_directories(BEFORE core agents training app)

set(SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/snake.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/env.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/agents/neuralNetwork.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/agents/geneticAgent.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/training/trainer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/menu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

# Build a standalone native executable
add_executable(snake_environment_test ${SOURCE_FILES})

target_link_libraries(snake_environment_test PRIVATE Threads::Threads)

if (_ipo_supported)
    set_property(TARGET snake_environment_test
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()            

# Compiler optimizations
target_compile_options(snake_environment_test PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-O3 -march=native>
    $<$<CXX_COMPILER_ID:MSVC>:/O2>
)