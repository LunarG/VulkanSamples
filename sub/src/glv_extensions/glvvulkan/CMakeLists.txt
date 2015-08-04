PROJECT(glvvulkan)
cmake_minimum_required(VERSION 2.8)

#include(FindPkgConfig)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(PYTHON_CMD "python3")
else()
    set(PYTHON_CMD "py")
endif()

set(GLV_VULKAN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# if Glave is being built as part of a vulkan driver build, then use that target instead of the locally-commited binary.
#if (TARGET vulkan)
#    message(STATUS "Using external Vulkan header and library.")
#    set(GLV_VULKAN_LIB vulkan)
#    set(GLV_VULKAN_DRIVER_DIR ${CMAKE_SOURCE_DIR})
#    set(GLV_VULKAN_HEADER ${CMAKE_SOURCE_DIR}/include/vulkan.h)
#    set(GLV_VULKAN_WSI_LUNARG_HEADER ${CMAKE_SOURCE_DIR}/include/vk_wsi_lunarg.h)
#else()
    # Use a locally committed vulkan header and binary
    message(STATUS "Using Glave-supplied Vulkan header and library.")
    set(GLV_VULKAN_DRIVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan)
    set(GLV_VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
    set(GLV_VULKAN_HEADER ${GLV_VULKAN_INCLUDE_DIR}/vulkan.h)
    set(GLV_VULKAN_DEBUG_REPORT_LUNARG_HEADER ${GLV_VULKAN_INCLUDE_DIR}/vk_debug_report_lunarg.h)
    set(GLV_VULKAN_WSI_SWAPCHAIN_HEADER ${GLV_VULKAN_INCLUDE_DIR}/vk_wsi_swapchain.h)
    set(GLV_VULKAN_WSI_DEVICE_SWAPCHAIN_HEADER ${GLV_VULKAN_INCLUDE_DIR}/vk_wsi_device_swapchain.h)

    if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(PATH_TO_WIN_VULKAN ${GLV_VULKAN_DIR}/vulkan/lib/${CMAKE_CFG_INTDIR})
        set(GLV_VULKAN_LIB
            ${PATH_TO_WIN_VULKAN}/vulkan.lib
        )
    endif()

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(GLV_VULKAN_LIB
        ${GLV_VULKAN_DIR}/vulkan/lib/libvulkan.so
    )
    endif()
#endif()

#message(STATUS "GLV_VULKAN_LIB = " ${GLV_VULKAN_LIB})
#message(STATUS "GLV_VULKAN_DRIVER_DIR = " ${GLV_VULKAN_DRIVER_DIR})
#message(STATUS "GLV_VULKAN_HEADER = " ${GLV_VULKAN_HEADER})
#message(STATUS "GLV_VULKAN_WSI_LUNARG_HEADER = " ${GLV_VULKAN_WSI_LUNARG_HEADER})
#message(STATUS "GLV_VULKAN_DEBUG_REPORT_LUNARG_HEADER = " ${GLV_VULKAN_DEBUG_REPORT_LUNARG_HEADER})

# Run a codegen script to generate utilities that are vulkan-specific, dependent on the vulkan header files, and may be shared by the tracer, replayer, or debugger.
# Generally, these are likely to be things that SHOULD be provided by the vulkan SDK.
set(GLV_VULKAN_CODEGEN_UTILS "vulkan/codegen_utils")
file(MAKE_DIRECTORY ${GLV_VULKAN_DIR}/${GLV_VULKAN_CODEGEN_UTILS})

# generate files for vulkan.h
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_struct_wrappers ${GLV_VULKAN_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_enum_string_helper ${GLV_VULKAN_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})

# generate files for vk_wsi_swapchain.h
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_struct_wrappers ${GLV_VULKAN_WSI_SWAPCHAIN_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_enum_string_helper ${GLV_VULKAN_WSI_SWAPCHAIN_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})

# generate files for vk_wsi_device_swapchain.h
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_struct_wrappers ${GLV_VULKAN_WSI_DEVICE_SWAPCHAIN_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_enum_string_helper ${GLV_VULKAN_WSI_DEVICE_SWAPCHAIN_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})

# generate files for vk_debug_report_lunarg.h
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_struct_wrappers ${GLV_VULKAN_DEBUG_REPORT_LUNARG_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GLV_VULKAN_DRIVER_DIR}/vk_helper.py --gen_enum_string_helper ${GLV_VULKAN_DEBUG_REPORT_LUNARG_HEADER} --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/${GLV_VULKAN_CODEGEN_UTILS})

# Run a codegen script to generate glave-specific vulkan utils
set(CODEGEN_GLVVK_DIR "codegen_glvvk_utils")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${CODEGEN_GLVVK_DIR})
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate.py glave-packet-id vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_packet_id.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate.py glave-core-trace-packets vk_core OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_packets.h)
#execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate.py glave-ext-trace-packets vk_wsi_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_lunarg_packets.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate.py glave-ext-trace-packets vk_wsi_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_swapchain_packets.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate.py glave-ext-trace-packets vk_wsi_device_swapchain OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_wsi_device_swapchain_packets.h)
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate.py glave-ext-trace-packets vk_debug_report_lunarg OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CODEGEN_GLVVK_DIR}/glv_vk_vk_debug_report_lunarg_packets.h)

# Directories which actually contain vulkan-specific glave plugins.
#add_subdirectory(glvdebug_vk/)
add_subdirectory(glvreplay_vk/)
add_subdirectory(glvtrace_vk/)

