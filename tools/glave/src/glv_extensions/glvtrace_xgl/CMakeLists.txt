cmake_minimum_required(VERSION 2.8)

project(glvtrace_xgl)

include("${SRC_DIR}/build_options.cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../../..)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${SRC_DIR}/../../../include)

add_custom_command(OUTPUT glvtrace_xgl_xgl.h glvtrace_xgl_xgl.c glvtrace_xgl_packet_id.h glvtrace_xgl_xgl_structs.h glvtrace_xgl_xglwsix11ext.h glvtrace_xgl_xglwsix11ext.c glvtrace_xgl_xglwsix11ext_structs.h glvtrace_xgl_xgldbg.h glvtrace_xgl_xgldbg.c glvtrace_xgl_xgldbg_structs.h xgl_struct_size_helper.h
	COMMAND ${SRC_DIR}/../../../glave-generate.py glave-trace-h > glvtrace_xgl_xgl.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-trace-c > glvtrace_xgl_xgl.c
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-packet-id > glvtrace_xgl_packet_id.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-core-structs > glvtrace_xgl_xgl_structs.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-wsi-trace-h > glvtrace_xgl_xglwsix11ext.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-wsi-trace-c > glvtrace_xgl_xglwsix11ext.c
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-wsi-trace-structs > glvtrace_xgl_xglwsix11ext_structs.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-dbg-trace-h > glvtrace_xgl_xgldbg.h
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-dbg-trace-c > glvtrace_xgl_xgldbg.c
        COMMAND ${SRC_DIR}/../../../glave-generate.py glave-dbg-trace-structs > glvtrace_xgl_xgldbg_structs.h
        COMMAND ${SRC_DIR}/../../../xgl_helper.py --gen_struct_wrappers ${SRC_DIR}/../../../include/xgl.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
	           DEPENDS ${SRC_DIR}/../../../xgl_helper.py
                           ${SRC_DIR}/../../../glave-generate.py)

set(SRC_LIST
    ${SRC_LIST}
    glvtrace_xgl.c
    glvtrace_xgl_xgl.c
    glvtrace_xgl_xgldbg.c
    glvtrace_xgl_xglwsix11ext.c
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

set (HDR_LIST
    glvtrace_xgl_packet_id.h
    glvtrace_xgl_xgl.h
    glvtrace_xgl_xgl_structs.h
    glvtrace_xgl_xgldbg.h
    glvtrace_xgl_xgldbg_structs.h
    glvtrace_xgl_xglwsix11ext.h
    glvtrace_xgl_xglwsix11ext_structs.h
    xgl_struct_size_helper.h
)

include_directories(
    ${SRC_DIR}
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/glvtrace
    ${SRC_DIR}/thirdparty
)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST} ${HDR_LIST})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries(${PROJECT_NAME} 
    glvcommon
    ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/libXGL.so
    -shared
    -ldl
)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES -DXCB_NVIDIA")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXGL_PROTOTYPES -DXCB_NVIDIA")
    set(PATH_TO_WIN_XGL ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/${CMAKE_CFG_INTDIR})
    set(PATH_TO_MHOOK ${CMAKE_CURRENT_BINARY_DIR}/../../thirdparty/mhook/mhook-lib/${CMAKE_CFG_INTDIR})
    set(PATH_TO_DISASM ${CMAKE_CURRENT_BINARY_DIR}/../../thirdparty/mhook/disasm-lib/${CMAKE_CFG_INTDIR})
    set(OS_REPLAYER_LIBS
        ${PATH_TO_WIN_XGL}/XGL.lib
        ${PATH_TO_MHOOK}/mhook.lib
        ${PATH_TO_DISASM}/disasm.lib
        xcb_nvidia
    )
    target_link_libraries(${PROJECT_NAME} 
        glvcommon
	${OS_REPLAYER_LIBS}
    )
endif()

build_options_finalize()

set_target_properties(glvtrace_xgl PROPERTIES LINKER_LANGUAGE C)
