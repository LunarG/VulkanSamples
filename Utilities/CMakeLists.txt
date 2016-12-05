cmake_minimum_required(VERSION 2.8)

#=========================VULKAN_WRAPPER==========================
option(USE_VULKAN_WRAPPER "Build a dispatch-table, to skip loader trampoline code." OFF)
if (USE_VULKAN_WRAPPER)
    add_definitions(-DUSE_VULKAN_WRAPPER)
endif()
#=================================================================

add_subdirectory(WSIWindow)
add_subdirectory(WSIWindow_Sample1)
#add_subdirectory(WSIWindow_Teapots)
