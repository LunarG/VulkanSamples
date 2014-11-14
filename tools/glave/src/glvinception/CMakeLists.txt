cmake_minimum_required(VERSION 2.8)
project(glvinception)

include("${SRC_DIR}/build_options.cmake")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRC_LIST
    ${SRC_LIST}
    glvinception.c
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

include_directories(
    ${SRC_DIR}
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/glvtrace
    ${SRC_DIR}/thirdparty
)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} 
    glvcommon
)

build_options_finalize()

set_target_properties(glvinception PROPERTIES LINKER_LANGUAGE C)
