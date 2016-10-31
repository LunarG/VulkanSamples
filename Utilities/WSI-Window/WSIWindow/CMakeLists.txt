set(LIBRARY_NAME WSIWindow)

file(GLOB SRC_LIST "*.h" "*.cpp")                   # List all source files from current directory
add_library (${LIBRARY_NAME} STATIC ${SRC_LIST})    # and add them to the WSIWindow library.
target_include_directories (${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-std=c++11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")  #for find_package

#============================VALIDATION===========================
#    Validation layers is always off for Release builds
option(ENABLE_VALIDATION "Enable Vulkan Validation in Debug builds"  ON)
if (ENABLE_VALIDATION)
    add_definitions(-DENABLE_VALIDATION)
endif()
#=================================================================
#==============================LINUX==============================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(BUILD_WSI_XCB_SUPPORT     "Build XCB WSI support"      ON)
    #option(BUILD_WSI_XLIB_SUPPORT    "Build Xlib WSI support"    OFF)
    #option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" OFF)
    #option(BUILD_WSI_MIR_SUPPORT     "Build Mir WSI support"     OFF)
    option(ENABLE_MULTITOUCH    "X11 Multi-touch-screen support"  ON)  #adds 8.5KB to exe size

    if (BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
        #---XCB---  (window)
        find_package(XCB REQUIRED)
        include_directories(${XCB_INCLUDE_DIRS})                 # /usr/include
        target_link_libraries(${LIBRARY_NAME} ${XCB_LIBRARIES})  # /usr/lib/x86_64-linux-gnu/libxcb.so
        #---XKB--- (keyboard)
        find_library(XKB "xkbcommon" DOC "XKB Keyboard library") # xkb keyboard support
        target_link_libraries(${LIBRARY_NAME} ${XKB})            # /usr/lib/x86_64-linux-gnu/libxkbcommon.so

        #---X11---
        find_package(X11 REQUIRED)
        include_directories(${X11_INCLUDE_DIR})
        target_link_libraries(${LIBRARY_NAME} ${X11_LIBRARIES})
        #---X11-XCB---
        find_package(X11_XCB REQUIRED)
        include_directories(${X11_XCB_INCLUDE_DIR})
        target_link_libraries(${LIBRARY_NAME} ${X11_XCB_LIBRARIES})
        #---XInput--- (MultiTouch)
        if (ENABLE_MULTITOUCH)
            target_link_libraries(${LIBRARY_NAME} ${X11_Xinput_LIB})
            add_definitions(-DENABLE_MULTITOUCH)
        endif()
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
#=================================================================
#=============================WINDOWS=============================
if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    #set(VULKAN_SDK $ENV{VULKAN_SDK} CACHE PATH "Path to the Vulkan SDK.")
    #target_link_libraries(${LIBRARY_NAME} "${VULKAN_SDK}/bin/vulkan-1.lib")
    #target_include_directories(${LIBRARY_NAME} PUBLIC "${VULKAN_SDK}/include")
    
    if(NOT ${CMAKE_SYSTEM_VERSION} VERSION_LESS 10)                          # Check for Windows 10+
        target_compile_definitions(${LIBRARY_NAME} PUBLIC -DWIN10PLUS)       # define WIN10PLUS
    endif()

endif()
#=================================================================
#=============================ANDROID=============================
# Note: Android has its own CMakeLists.txt in the Android/app folder.
#if(ANDROID)
#    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
#endif()
#=================================================================
#=======================Find Vulkan Loader========================
find_library(VULKAN_LOADER vulkan   DOC "Path to Vulkan loader library.")
find_library(VULKAN_LOADER vulkan   PATH "$ENV{VULKAN_SDK}/lib")             # Search in Vulkan SDK on Linux  : /lib/libvulkan.so
find_library(VULKAN_LOADER vulkan-1 PATH "$ENV{VULKAN_SDK}/bin")             # Search in Vulkan SDK on Windows: /bin/vulkan-1.lib
find_library(VULKAN_LOADER vulkan   PATH "../../../build/Default/loader")    # Search in VulkanSamples on Linux
find_library(VULKAN_LOADER vulkan-1 PATH "../../../build/Default/loader")    # Search in VulkanSamples on Windows
target_link_libraries(${LIBRARY_NAME} ${VULKAN_LOADER})                      # Link to loader
message("Found VULKAN_LOADER  at: ${VULKAN_LOADER}")
#=================================================================
#=======================Find Vulkan Header========================
find_path(VULKAN_INCLUDE vulkan/vulkan.h DOC "Path to Vulkan header file.")
find_path(VULKAN_INCLUDE vulkan/vulkan.h PATH "$ENV{VULKAN_SDK}/include")    # Search in Vulkan SDK
find_path(VULKAN_INCLUDE vulkan/vulkan.h PATH "../../../include")            # Search in VulkanSamples
target_include_directories(${LIBRARY_NAME} PUBLIC "${VULKAN_INCLUDE}")       # include vulkan.h
message("Found VULKAN_INCLUDE at: ${VULKAN_INCLUDE}")
#=================================================================
