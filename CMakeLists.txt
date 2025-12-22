cmake_minimum_required(VERSION 3.16)

project(memory_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    include
)

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

add_executable(memory-simulator
    ${SRC_FILES}
)
