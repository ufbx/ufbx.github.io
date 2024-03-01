cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE SRC_FILES "*.c" "*.h")
file(REAL_PATH post_js.js POST_JS)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES})
add_executable(js_viewer WIN32 ${SRC_FILES})
target_link_libraries(js_viewer viewer)

if(MSVC)
  target_compile_options(js_viewer PRIVATE /W3 /WX)
else()
  target_compile_options(js_viewer PRIVATE -Wall -Werror)
endif()

if(EMSCRIPTEN)
  set_target_properties(js_viewer PROPERTIES LINK_DEPENDS ${POST_JS})
  target_link_options(js_viewer PRIVATE --post-js ${POST_JS})
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(js_viewer PRIVATE -g4 --source-map-base http://localhost:8080/native/)
    target_compile_options(js_viewer PRIVATE -g3)
  endif()
endif()

