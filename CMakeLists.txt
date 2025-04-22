cmake_minimum_required(VERSION 3.10)
project(FTCache CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB SRCS
    "src/*.cpp"
    "src/*.hpp"
)

# Include directories
include_directories(src)

# Main executable
add_executable(FTCache ${SRCS})

# Enable testing
enable_testing()

# Demo target: build and run with default parameters
add_custom_target(demo
    COMMAND FTCache
    DEPENDS FTCache
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running FTCache demo with defaults"
)

# Define a simple CTest test that runs the demo
add_test(NAME RunDemo COMMAND FTCache --ttl 5 --fail-node NodeB)

# Optional test run flag (only adds run_tests if explicitly requested)
option(ENABLE_TESTS "Enable FTCache run_tests target" OFF)
if(ENABLE_TESTS)
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS FTCache
        COMMENT "Running all tests"
    )
endif()
