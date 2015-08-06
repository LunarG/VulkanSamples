cmake_minimum_required(VERSION 2.8)

project(glvreplay_vk)

include("${SRC_DIR}/build_options.cmake")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/codegen)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-replay-vk-funcs     vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvreplay_vk_func_ptrs.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-replay-c            vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvreplay_vk_replay_gen.cpp)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DIR}/vktrace_generate.py glave-replay-obj-mapper-h vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/codegen/glvreplay_vk_objmapper.h)

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
    glvreplay_vk.cpp
    glvreplay_vk_settings.cpp
    glvreplay_vk_vkreplay.cpp
    glvreplay_vk_vkdisplay.cpp
    codegen/glvreplay_vk_replay_gen.cpp
)

set (HDR_LIST
    glvreplay_vk.h
    glvreplay_vk_settings.h
    glvreplay_vk_vkdisplay.h
    glvreplay_vk_vkreplay.h
    codegen/glvreplay_vk_func_ptrs.h
    codegen/glvreplay_vk_objmapper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils/vk_enum_string_helper.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_packet_id.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_packets.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_swapchain_packets.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_device_swapchain_packets.h
    ${CODEGEN_GLVVK_DIR}/glv_vk_vk_debug_report_lunarg_packets.h
)

include_directories(
    codegen
    ${SRC_DIR}/glvreplay
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CODEGEN_GLVVK_DIR}
    ${GLV_VULKAN_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan/codegen_utils
)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST} ${HDR_LIST})

target_link_libraries(${PROJECT_NAME} 
    ${OS_REPLAYER_LIBS}
    ${GLV_VULKAN_LIB}
    getopt_bundled
    glvcommon
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PATH_TO_WIN_VULKAN}/vulkan.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

build_options_finalize()
