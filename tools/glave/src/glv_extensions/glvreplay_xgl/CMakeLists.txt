cmake_minimum_required(VERSION 2.8)

project(glvreplay_xgl)

include("${SRC_DIR}/build_options.cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../../..)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${SRC_DIR}/../../../include ${SRC_DIR}/../../../layers ${SRC_DIR}/glv_extensions/glvreplay_xgl)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_packet_id.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/xgl_enum_string_helper.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xgl_structs.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xglwsix11ext_structs.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xgldbg_structs.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvreplay_xgl_replay.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvreplay_xgl_replay.cpp
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk-generate.py glave-packet-id > glvtrace_xgl_packet_id.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk-generate.py glave-core-structs > glvtrace_xgl_xgl_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk-generate.py glave-wsi-trace-structs > glvtrace_xgl_xglwsix11ext_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk-generate.py glave-dbg-trace-structs > glvtrace_xgl_xgldbg_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk-generate.py glave-replay-h > glvreplay_xgl_replay.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk-generate.py glave-replay-c > glvreplay_xgl_replay.cpp
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk-helper.py --gen_enum_string_helper ${SRC_DIR}/../../../include/xgl.h --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/codegen
                   DEPENDS ${SRC_DIR}/../scripts/vk-generate.py
                           ${SRC_DIR}/../scripts/vk-helper.py
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXCB_NVIDIA")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXCB_NVIDIA")
    set(PATH_TO_WIN_XGL ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/${CMAKE_CFG_INTDIR})
    set(OS_REPLAYER_LIBS
        ${PATH_TO_WIN_XGL}/XGL.lib
        xcb_nvidia
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
    codegen/glvreplay_xgl_replay.cpp
)

set (HDR_LIST
    codegen/glvtrace_xgl_packet_id.h
    codegen/glvtrace_xgl_xgl_structs.h
    codegen/glvtrace_xgl_xglwsix11ext_structs.h
    codegen/glvtrace_xgl_xgldbg_structs.h
    codegen/xgl_enum_string_helper.h
    glvreplay_xgl.h
    glvreplay_xgl_settings.h
    codegen/glvreplay_xgl_replay.h
)

include_directories(
    codegen
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

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PATH_TO_WIN_XGL}/XGL.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

build_options_finalize()
