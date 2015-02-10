cmake_minimum_required(VERSION 2.8)

project(glvreplay_xgl)

include("${SRC_DIR}/build_options.cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../../..)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${SRC_DIR}/../../../include ${SRC_DIR}/../../../layers ${SRC_DIR}/glv_extensions/glvreplay_xgl)

# This file is shared between replayer and tracer and are generated to each dir.  Should generate them to common area
add_custom_command(OUTPUT glvtrace_xgl_packet_id.h xgl_enum_string_helper.h glvtrace_xgl_xgl_structs.h glvtrace_xgl_xglwsix11ext_structs.h glvtrace_xgl_xgldbg_structs.h glvreplay_xgl_replay.h glvreplay_xgl_replay.cpp
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-packet-id > glvtrace_xgl_packet_id.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-core-structs > glvtrace_xgl_xgl_structs.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-wsi-trace-structs > glvtrace_xgl_xglwsix11ext_structs.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-dbg-trace-structs > glvtrace_xgl_xgldbg_structs.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-replay-h > glvreplay_xgl_replay.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-replay-c > glvreplay_xgl_replay.cpp
        COMMAND ${SRC_DIR}/../../../xgl_helper.py --gen_enum_string_helper ${SRC_DIR}/../../../include/xgl.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
	           DEPENDS ${SRC_DIR}/../../../glave-generate.py
	                   ${SRC_DIR}/../../../xgl_helper.py)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(OS_REPLAYER_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/${CMAKE_BUILD_TYPE}/XGL.lib
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
    glvreplay_xgl_settings.cpp
    glvreplay_xgl_replay.cpp
)

set (HDR_LIST
    glvtrace_xgl_packet_id.h
    glvtrace_xgl_xgl_structs.h
    glvtrace_xgl_xglwsix11ext_structs.h
    glvtrace_xgl_xgldbg_structs.h
    xgl_enum_string_helper.h
    glvreplay_xgl.h
    glvreplay_xgl_settings.h
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
    layer_utils
)

build_options_finalize()
