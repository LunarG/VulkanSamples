project(glave_xgl_gen)
cmake_minimum_required(VERSION 2.8)

add_library(${PROJECT_NAME} xgl_struct_wrappers.h xgl_struct_wrappers.cpp xgl_struct_string_helper.h)

set(COMPILE_FLAGS "-fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")

include_directories(${SRC_DIR}/thirdparty/${GEN_API}/inc/)

target_include_directories (glave_xgl_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
