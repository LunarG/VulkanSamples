set(LIBRARY_NAME WSIWindow)


file(GLOB SRC_LIST "*.h" "*.cpp")
add_library (${LIBRARY_NAME} STATIC ${SRC_LIST})
target_include_directories (${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#include_directories("${PROJECT_SOURCE_DIR}")

#SET(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions(-std=c++11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")  #for find_package

#==============================LINUX==============================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(BUILD_WSI_XCB_SUPPORT     "Build XCB WSI support"     ON)
    option(BUILD_WSI_XLIB_SUPPORT    "Build Xlib WSI support"    OFF)
    #option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" OFF)
    #option(BUILD_WSI_MIR_SUPPORT     "Build Mir WSI support"     OFF)

    if (BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
        find_package(XCB REQUIRED)
        message("XCB INCLUDE : ${XCB_INCLUDE_DIRS}")
        message("XCB LIBRARY : ${XCB_LIBRARIES}")
        include_directories(${XCB_INCLUDE_DIRS})                 # /usr/include
        target_link_libraries(${LIBRARY_NAME} ${XCB_LIBRARIES})  # /usr/lib/x86_64-linux-gnu/libxcb.so
        #find_library(XKB "xkbcommon")                            # /usr/lib/x86_64-linux-gnu/
        #target_link_libraries(${PROJECT_NAME} ${XKB})            # keyboard support as a static link library (adds 243Kb)
        LINK_DIRECTORIES("/usr/lib/x86_64-linux-gnu/")
        TARGET_LINK_LIBRARIES(${LIBRARY_NAME} "libxkbcommon.so")  #xkb keyboard support as a shared library
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    endif()

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
# Note: Android currently has its own CMakeLists.txt in the Android/app folder.
#if(ANDROID)
#    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
#endif()
#=================================================================


#=========================Find Vulkan SDK=========================
if(NOT DEFINED ENV{VULKAN_SDK})
    message("WARNING: VULKAN_SDK environment-variable not found!")
    if(WIN32)
        set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.0.21.1")         # Default path to SDK for Windows
    else()
        set(ENV{VULKAN_SDK}  "~/VulkanSDK/1.0.21.1/x86_64")  # Default path to SDK for Linux
    endif()
endif()

#set(VULKAN_SDK $ENV{VULKAN_SDK})
set(VULKAN_SDK $ENV{VULKAN_SDK} CACHE FILEPATH "Path to where Vulkan's SDK include directory can be found.")
#=================================================================
include_directories("${VULKAN_SDK}/include")

if(WIN32)
    target_link_libraries(${LIBRARY_NAME} "${VULKAN_SDK}/bin/vulkan-1.lib")
else()
    target_link_libraries(${LIBRARY_NAME} "${VULKAN_SDK}/lib/libvulkan.so")
endif()
