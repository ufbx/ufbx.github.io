cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE SRC_FILES "*.c" "*.h")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES})
add_executable(js_viewer WIN32 ${SRC_FILES})
target_link_libraries(js_viewer viewer)

if(MSVC)
  target_compile_options(viewer PRIVATE /W3 /WX)
else()
  target_compile_options(viewer PRIVATE -Wall -Werror)
endif()

