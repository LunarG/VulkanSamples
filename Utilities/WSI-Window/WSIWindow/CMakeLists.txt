set(LIBRARY_NAME WSIWindow)

file(GLOB SRC_LIST "*.h" "*.cpp")                   # List all source files from current directory
add_library (${LIBRARY_NAME} STATIC ${SRC_LIST})    # and add them to the WSIWindow library.
target_include_directories (${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-std=c++11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")  #for find_package

#==============================LINUX==============================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(BUILD_WSI_XCB_SUPPORT     "Build XCB WSI support"     ON)
    #option(BUILD_WSI_XLIB_SUPPORT    "Build Xlib WSI support"    OFF)
    #option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" OFF)
    #option(BUILD_WSI_MIR_SUPPORT     "Build Mir WSI support"     OFF)

    if (BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
        #---XCB---  (window)
        find_package(XCB REQUIRED)
        include_directories(${XCB_INCLUDE_DIRS})                 # /usr/include
        target_link_libraries(${LIBRARY_NAME} ${XCB_LIBRARIES})  # /usr/lib/x86_64-linux-gnu/libxcb.so
        #---XKB--- (keyboard)
        #find_library(XKB "xkbcommon")                            # /usr/lib/x86_64-linux-gnu/
        #target_link_libraries(${PROJECT_NAME} ${XKB})            # keyboard support as a static link library (adds 243Kb)
        LINK_DIRECTORIES("/usr/lib/x86_64-linux-gnu/")
        TARGET_LINK_LIBRARIES(${LIBRARY_NAME} "libxkbcommon.so")  #xkb keyboard support as a shared library
    endif()

#    if (BUILD_WSI_XLIB_SUPPORT)
#        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
#    endif()

#    if (BUILD_WSI_WAYLAND_SUPPORT)
#        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
#    endif()

#    if (BUILD_WSI_MIR_SUPPORT)
#        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
#    endif()

endif()
#=============================WINDOWS=============================
if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
endif()
#=================================================================
#=============================ANDROID=============================
# Note: Android has its own CMakeLists.txt in the Android/app folder.
#if(ANDROID)
#    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
#endif()
#=================================================================

#=========================Find Vulkan SDK=========================        # Get Path from the VULKAN_SDK environment variable.
if(NOT DEFINED ENV{VULKAN_SDK})
    message("WARNING: VULKAN_SDK env-variable was not found!")
endif()
set(VULKAN_SDK $ENV{VULKAN_SDK} CACHE PATH "Path to the Vulkan SDK.")     # Give user a chance to change it, using cmake-gui.
#=================================================================
#include_directories("${VULKAN_SDK}/include")
target_include_directories(${LIBRARY_NAME} PUBLIC "${VULKAN_SDK}/include")

if(WIN32)
    target_link_libraries(${LIBRARY_NAME} "${VULKAN_SDK}/bin/vulkan-1.lib")
else()
    target_link_libraries(${LIBRARY_NAME} "${VULKAN_SDK}/lib/libvulkan.so")
endif()
