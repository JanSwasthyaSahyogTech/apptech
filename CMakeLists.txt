cmake_minimum_required(VERSION 3.10)
project(AppTech VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# HeightDebouncer library (platform-independent logic)
add_library(height_debouncer_lib
    src/height_debouncer.cpp
)

# Unit tests executable
add_executable(test_height_debouncer
    test/test_height_debouncer.cpp
)

target_link_libraries(test_height_debouncer
    height_debouncer_lib
)

# Enable testing
enable_testing()
add_test(NAME HeightDebouncerTests COMMAND test_height_debouncer)

# Custom target to run tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_height_debouncer
)
