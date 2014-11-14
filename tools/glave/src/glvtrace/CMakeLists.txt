cmake_minimum_required(VERSION 2.8)
project(glvtrace)

include("${SRC_DIR}/build_options.cmake")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRC_LIST
    ${SRC_LIST}
    glvtrace.cpp
    glvtrace_process.h
    glvtrace_process.cpp
)

include_directories(
    ${SRC_DIR}
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/glvtrace
)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
    glvcommon
)

build_options_finalize()
