cmake_minimum_required(VERSION 3.10)
project(ufbx_doc_viewer)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(viewer)
if(EMSCRIPTEN)
  add_subdirectory(js_viewer)
else()
  add_subdirectory(window)
endif()
