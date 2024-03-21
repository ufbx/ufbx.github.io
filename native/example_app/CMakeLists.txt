cmake_minimum_required(VERSION 3.0)
project(example)
set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE SOURCE_FILES "*.c" "*.h")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

add_executable(example ${SOURCE_FILES})
