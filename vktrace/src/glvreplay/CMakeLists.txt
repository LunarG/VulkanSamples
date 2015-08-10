cmake_minimum_required(VERSION 2.8)
project(vkreplay)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../glv_extensions/glvvulkan/glvreplay_vk/
)

set (LIBRARIES glvcommon vulkan_replay)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} 
    ${LIBRARIES}
)

build_options_finalize()
