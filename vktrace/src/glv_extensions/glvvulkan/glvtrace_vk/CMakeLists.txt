cmake_minimum_required(VERSION 2.8)

project(vulkan_trace)

include("${SRC_DIR}/build_options.cmake")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-trace-h     vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-trace-c     vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk.c)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-ext-trace-h vk_wsi_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk_wsi_swapchain.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-ext-trace-c vk_wsi_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk_wsi_swapchain.c)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-ext-trace-h vk_wsi_device_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk_wsi_device_swapchain.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-ext-trace-c vk_wsi_device_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk_wsi_device_swapchain.c)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-ext-trace-h vk_debug_report_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk_debug_report_lunarg.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-ext-trace-c vk_debug_report_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvtrace_vk_vk_debug_report_lunarg.c)

set(SRC_LIST
    ${SRC_LIST}
    glvtrace_vk.c
    glvtrace_vk_trace.c
    codegen/glvtrace_vk_vk.c
    codegen/glvtrace_vk_vk_debug_report_lunarg.c
    codegen/glvtrace_vk_vk_wsi_swapchain.c
    codegen/glvtrace_vk_vk_wsi_device_swapchain.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_struct_size_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_swapchain_struct_size_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_device_swapchain_struct_size_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_debug_report_lunarg_struct_size_helper.c
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

set (HDR_LIST
    glvtrace_vk_helpers.h
    codegen/glvtrace_vk_vk.h
    codegen/glvtrace_vk_vk_debug_report_lunarg.h
    codegen/glvtrace_vk_vk_wsi_swapchain.h
    codegen/glvtrace_vk_vk_wsi_device_swapchain.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_packet_id.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_packets.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_debug_report_lunarg_packets.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_swapchain_packets.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_device_swapchain_packets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_struct_size_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_swapchain_struct_size_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_device_swapchain_struct_size_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_debug_report_lunarg_struct_size_helper.h
)

include_directories(
    codegen
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/glvtrace
    ${SRC_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CODEGEN_GLVVK_DIR}
    ${GLV_VULKAN_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils
)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST} ${HDR_LIST})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(OS_TRACER_LIBS
        -shared
        -ldl
    )
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
    set(OS_TRACER_LIBS
        mhook
        disasm
    )
endif()

target_link_libraries(${PROJECT_NAME}
    glvcommon
    ${GLV_VULKAN_LIB}
    ${OS_TRACER_LIBS}
)

build_options_finalize()

set_target_properties(vulkan_trace PROPERTIES LINKER_LANGUAGE C)
