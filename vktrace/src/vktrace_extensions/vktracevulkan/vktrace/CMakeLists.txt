cmake_minimum_required(VERSION 2.8)

project(vulkan_trace)

include("${SRC_DIR}/build_options.cmake")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-trace-h     vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-trace-c     vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk.c)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-h vk_wsi_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_wsi_swapchain.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-c vk_wsi_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_wsi_swapchain.c)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-h vk_wsi_device_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_wsi_device_swapchain.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-c vk_wsi_device_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_wsi_device_swapchain.c)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-h vk_debug_report_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_debug_report_lunarg.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-c vk_debug_report_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_debug_report_lunarg.c)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-h vk_debug_marker_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_debug_marker_lunarg.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${VKTRACE_VULKAN_DIR}/vktrace_generate.py vktrace-ext-trace-c vk_debug_marker_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vktrace_vk_vk_debug_marker_lunarg.c)

if (WIN32)
    # Put vulkan_trace.dll in the same directory as vktrace.exe
    # so that vktrace.exe can find vulkan_trace.dll.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../../../..)
endif()


set(SRC_LIST
    ${SRC_LIST}
    vktrace_lib.c
    vktrace_lib_trace.c
    codegen/vktrace_vk_vk.c
    codegen/vktrace_vk_vk_debug_report_lunarg.c
    codegen/vktrace_vk_vk_debug_marker_lunarg.c
    codegen/vktrace_vk_vk_wsi_swapchain.c
    codegen/vktrace_vk_vk_wsi_device_swapchain.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_struct_size_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_swapchain_struct_size_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_device_swapchain_struct_size_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_debug_report_lunarg_struct_size_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_debug_marker_lunarg_struct_size_helper.c
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

set (HDR_LIST
    vktrace_lib_helpers.h
    codegen/vktrace_vk_vk.h
    codegen/vktrace_vk_vk_debug_report_lunarg.h
    codegen/vktrace_vk_vk_debug_marker_lunarg.h
    codegen/vktrace_vk_vk_wsi_swapchain.h
    codegen/vktrace_vk_vk_wsi_device_swapchain.h
    ${CODEGEN_VKTRACE_DIR}/vktrace_vk_packet_id.h
    ${CODEGEN_VKTRACE_DIR}/vktrace_vk_vk_packets.h
    ${CODEGEN_VKTRACE_DIR}/vktrace_vk_vk_debug_report_lunarg_packets.h
    ${CODEGEN_VKTRACE_DIR}/vktrace_vk_vk_debug_marker_lunarg_packets.h
    ${CODEGEN_VKTRACE_DIR}/vktrace_vk_vk_wsi_swapchain_packets.h
    ${CODEGEN_VKTRACE_DIR}/vktrace_vk_vk_wsi_device_swapchain_packets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_struct_size_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_swapchain_struct_size_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_wsi_device_swapchain_struct_size_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_debug_report_lunarg_struct_size_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_debug_marker_lunarg_struct_size_helper.h
)

include_directories(
    codegen
    ${SRC_DIR}/vktrace_common
    ${SRC_DIR}/vktrace_trace
    ${SRC_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CODEGEN_VKTRACE_DIR}
    ${VKTRACE_VULKAN_INCLUDE_DIR}
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
    vktrace_common
    ${VKTRACE_VULKAN_LIB}
    ${OS_TRACER_LIBS}
)

build_options_finalize()

set_target_properties(vulkan_trace PROPERTIES LINKER_LANGUAGE C)
