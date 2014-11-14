cmake_minimum_required(VERSION 2.8)
project(glvreplay)

include("${SRC_DIR}/build_options.cmake")

set(SRC_LIST
    ${SRC_LIST}
    glvreplay_factory.h
    glvreplay_seq.h
    glvreplay_window.h
    glvreplay_main.cpp
    glvreplay_seq.cpp
    glvreplay_factory.cpp
)

include_directories(
    ${SRC_DIR}/glvreplay
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/thirdparty
)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} 
    getopt_bundled
    glvcommon
)

build_options_finalize()
