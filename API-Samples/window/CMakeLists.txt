project(window)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

include_directories("${PROJECT_SOURCE_DIR}")
#include_directories("${PROJECT_SOURCE_DIR}/include")
add_definitions(-std=c++11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")  #for find_package

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(BUILD_WSI_XCB_SUPPORT     "Build XCB WSI support"     ON)
    option(BUILD_WSI_XLIB_SUPPORT    "Build Xlib WSI support"    OFF)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" OFF)
    option(BUILD_WSI_MIR_SUPPORT     "Build Mir WSI support"     OFF)

    if (BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
        find_package(XCB REQUIRED)
        message("XCB INCLUDE : ${XCB_INCLUDE_DIRS}")
        message("XCB LIBRARY : ${XCB_LIBRARIES}")
        include_directories(${XCB_INCLUDE_DIRS})                 # /usr/include
        target_link_libraries(${PROJECT_NAME} ${XCB_LIBRARIES})  # /usr/lib/x86_64-linux-gnu/libxcb.so
        #find_library(XKB "xkbcommon")                            # /usr/lib/x86_64-linux-gnu/
        #target_link_libraries(${PROJECT_NAME} ${XKB})            # keyboard support as static link library (adds 243Kb)

        LINK_DIRECTORIES("/usr/lib/x86_64-linux-gnu/")
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} "libxkbcommon.so")  #xkb keyboard support as shared library
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif()

    if (BUILD_WSI_MIR_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
    endif()
endif()

#=====================Find Vulkan SDK======================
if(NOT DEFINED ENV{VULKAN_SDK})
  set(ENV{VULKAN_SDK} "~/vulkan/VulkanSDK/1.0.21.1/x86_64")  # Default path to SDK
endif()
set(VULKAN_SDK $ENV{VULKAN_SDK} CACHE FILEPATH "Path to where Vulkan's SDK include directory can be found.")
#==========================================================

include_directories("${VULKAN_SDK}/include")

if(WIN32)
    target_link_libraries(${PROJECT_NAME} "${VULKAN_SDK}/lib/vulkan-1.dll")
else()
    target_link_libraries(${PROJECT_NAME} "${VULKAN_SDK}/lib/libvulkan.so")
endif()
