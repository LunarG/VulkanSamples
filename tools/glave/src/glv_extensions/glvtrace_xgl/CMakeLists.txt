cmake_minimum_required(VERSION 2.8)

project(glvtrace_xgl)

include("${SRC_DIR}/build_options.cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../../..)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${SRC_DIR}/../../../include)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xgl.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xgl.c
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glv_vk_packet_id.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glv_vk_vk_structs.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xglwsix11ext.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xglwsix11ext.c
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glv_vk_vkwsix11ext_structs.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xgldbg.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_xgl_xgldbg.c
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glv_vk_vkdbg_structs.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/xgl_struct_size_helper.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/xgl_struct_size_helper.c
                          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/xgl_enum_string_helper.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-trace-h > glvtrace_xgl_xgl.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-trace-c > glvtrace_xgl_xgl.c
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-packet-id > glv_vk_packet_id.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-core-structs > glv_vk_vk_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-wsi-trace-h > glvtrace_xgl_xglwsix11ext.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-wsi-trace-c > glvtrace_xgl_xglwsix11ext.c
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-wsi-trace-structs > glv_vk_vkwsix11ext_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-dbg-trace-h > glvtrace_xgl_xgldbg.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-dbg-trace-c > glvtrace_xgl_xgldbg.c
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../scripts/vk_generate.py glave-dbg-trace-structs > glv_vk_vkdbg_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../../../xgl_helper.py --gen_struct_wrappers ${SRC_DIR}/../../../include/xgl.h --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/codegen
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../../../xgl_helper.py --gen_enum_string_helper ${SRC_DIR}/../../../include/xgl.h --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/codegen
                   DEPENDS ${SRC_DIR}/../../../xgl_helper.py
                           ${SRC_DIR}/../scripts/vk_generate.py
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)

set(SRC_LIST
    ${SRC_LIST}
    glvtrace_xgl.c
    glvtrace_xgl_trace.c
    codegen/xgl_struct_size_helper.c
    codegen/glvtrace_xgl_xgl.c
    codegen/glvtrace_xgl_xgldbg.c
    codegen/glvtrace_xgl_xglwsix11ext.c
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

set (HDR_LIST
    codegen/glv_vk_packet_id.h
    codegen/glvtrace_xgl_xgl.h
    codegen/glv_vk_vk_structs.h
    codegen/glvtrace_xgl_xgldbg.h
    codegen/glv_vk_vkdbg_structs.h
    codegen/glvtrace_xgl_xglwsix11ext.h
    codegen/glv_vk_vkwsix11ext_structs.h
    codegen/xgl_struct_size_helper.h
    codegen/xgl_enum_string_helper.h
)

include_directories(
    codegen
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
