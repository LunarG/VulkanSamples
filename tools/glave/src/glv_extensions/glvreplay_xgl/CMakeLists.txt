cmake_minimum_required(VERSION 2.8)

# this project is currently only available on Linux
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

project(glvreplay_xgl)

include("${SRC_DIR}/build_options.cmake")

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${SRC_DIR}/../../../include)

# This file is shared between replayer and tracer and are generated to each dir.  Should generate them to common area
add_custom_command(OUTPUT glvtrace_xgl_packet_id.h
        COMMAND ${SRC_DIR}/../../../xgl-layer-generate.py glave-packet-id > glvtrace_xgl_packet_id.h
	           DEPENDS ${SRC_DIR}/../../../xgl-layer-generate.py)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(OS_REPLAYER_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/libXGL.dll
)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OS_REPLAYER_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/libXGL.so
    xcb
)
endif()

set(SRC_LIST
    ${SRC_LIST}
    glvreplay_xgl.cpp
    glvreplay_xgl_replay.cpp
)

set (HDR_LIST
    glvtrace_xgl_packet_id.h
    glvreplay_xgl.h
    glvreplay_xgl_replay.h
)

include_directories(
    ${SRC_DIR}/glvreplay
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/thirdparty
    ${SRC_DIR}/glv_extensions/glvtrace_xgl
)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST} ${HDR_LIST})

target_link_libraries(${PROJECT_NAME} 
    ${OS_REPLAYER_LIBS}
    getopt_bundled
    glvcommon
)

build_options_finalize()

endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
