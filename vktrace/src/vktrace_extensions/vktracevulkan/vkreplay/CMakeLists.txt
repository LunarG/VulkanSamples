cmake_minimum_required(VERSION 2.8)

project(vulkan_replay)

include("${SRC_DIR}/build_options.cmake")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-replay-vk-funcs     vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vkreplay_vk_func_ptrs.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-replay-c            vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vkreplay_vk_replay_gen.cpp)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-replay-obj-mapper-h vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/vkreplay_vk_objmapper.h)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OS_REPLAYER_LIBS
    xcb
)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(OS_REPLAYER_LIBS  )
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

set(SRC_LIST
    ${SRC_LIST}
    vkreplay.cpp
    vkreplay_settings.cpp
    vkreplay_vkreplay.cpp
    vkreplay_vkdisplay.cpp
    codegen/vkreplay_vk_replay_gen.cpp
)

set (HDR_LIST
    vkreplay.h
    vkreplay_settings.h
    vkreplay_vkdisplay.h
    vkreplay_vkreplay.h
    codegen/vkreplay_vk_func_ptrs.h
    codegen/vkreplay_vk_objmapper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_enum_string_helper.h
    ${CODEGEN_GLVVK_DIR}/vktrace_vk_packet_id.h
    ${CODEGEN_GLVVK_DIR}/vktrace_vk_vk_packets.h
    ${CODEGEN_GLVVK_DIR}/vktrace_vk_vk_wsi_swapchain_packets.h
    ${CODEGEN_GLVVK_DIR}/vktrace_vk_vk_wsi_device_swapchain_packets.h
    ${CODEGEN_GLVVK_DIR}/vktrace_vk_vk_debug_report_lunarg_packets.h
)

include_directories(
    codegen
    ${SRC_DIR}/vktrace_replay
    ${SRC_DIR}/vktrace_common
    ${SRC_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CODEGEN_GLVVK_DIR}
    ${GLV_VULKAN_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils
)

add_library(${PROJECT_NAME} STATIC ${SRC_LIST} ${HDR_LIST})

target_link_libraries(${PROJECT_NAME} 
    ${OS_REPLAYER_LIBS}
    ${GLV_VULKAN_LIB}
    vktrace_common
)

build_options_finalize()
