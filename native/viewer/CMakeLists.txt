cmake_minimum_required(VERSION 3.10)

set(SOKOL_SHDC "sokol-shdc" CACHE FILEPATH "Sokol shader compiler path")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen/shaders")
file(GLOB_RECURSE SHADERS "shaders/*.glsl")

foreach(SHADER_SRC ${SHADERS})
    get_filename_component(SHADER_DIR ${SHADER_SRC} DIRECTORY)
    get_filename_component(SHADER_STEM ${SHADER_SRC} NAME_WE)
    set(SHADER_DST "${CMAKE_CURRENT_BINARY_DIR}/gen/shaders/${SHADER_STEM}.h")
    add_custom_command(
        DEPENDS ${SHADER_SRC}
        OUTPUT ${SHADER_DST}
        COMMAND ${SOKOL_SHDC} --ifdef -b -l hlsl5:glsl300es:glsl330 -i ${SHADER_SRC} -o ${SHADER_DST}
    )
    list(APPEND GEN_OUTPUT ${SHADER_DST})
endforeach(SHADER_SRC)

add_custom_target(gen DEPENDS ${GEN_OUTPUT})

file(GLOB_RECURSE SRC_FILES "*.c" "*.h")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES})
add_library(viewer STATIC ${SRC_FILES})

add_dependencies(viewer gen)

target_include_directories(viewer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen")
target_include_directories(viewer INTERFACE ".")

if(MSVC)
  target_compile_options(viewer PRIVATE /W3 /WX)
else()
  target_compile_options(viewer PRIVATE -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-missing-braces)
endif()

if(EMSCRIPTEN)
  target_link_options(viewer PUBLIC -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sTOTAL_STACK=1MB -sTOTAL_MEMORY=8MB -sALLOW_MEMORY_GROWTH=1)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(viewer PUBLIC -sSAFE_HEAP=1 -sASSERTIONS=1)
    target_compile_options(viewer PRIVATE -g3)
  endif()
endif()