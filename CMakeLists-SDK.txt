cmake_minimum_required(VERSION 3.10.2)
project (VULKAN_SAMPLES)
# set (CMAKE_VERBOSE_MAKEFILE 1)
set(API_NAME "Vulkan" CACHE STRING "API name to use when building")
string(TOLOWER ${API_NAME} API_LOWERCASE)
include(GNUInstallDirs)

if(EXISTS "${CMAKE_SOURCE_DIR}/../x86_64/include")
    set(SDK_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/../x86_64/include")
    message("SDK_INCLUDE_PATH = ${SDK_INCLUDE_PATH}")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/../x86_64/lib")
    set(SDK_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/../x86_64/lib")
    message("SDK_LIBRARY_PATH = ${SDK_LIBRARY_PATH}")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/../x86_64/bin")
    set(SDK_BINARY_PATH "${CMAKE_SOURCE_DIR}/../x86_64/bin")
    message("SDK_BINARY_PATH = ${SDK_BINARY_PATH}")
endif()

# The MAJOR number of the version we're building, used in naming
# vulkan-<major>.dll (and other files).
set(MAJOR "1")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(PythonInterp 3 REQUIRED)
set (CMAKE_INSTALL_PREFIX "")

set (UTILS_NAME vsamputils)

if(NOT WIN32)
    include(FindPkgConfig)
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" OFF)
    set(DEMOS_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for demos (XCB, XLIB, WAYLAND, DISPLAY)")
    set(SAMPLES_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for api-samples (XCB, WAYLAND, DISPLAY)")

    if (BUILD_WSI_XCB_SUPPORT)
         find_package(XCB REQUIRED)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
        find_package(Wayland REQUIRED)
    endif()

    set (BUILDTGT_DIR build)
    set (BINDATA_DIR x86_64/bin)
    set (LIBSOURCE_DIR Lib)
else()
    # For Windows, since 32-bit and 64-bit items can co-exist, we build each in its own build directory.
    # 32-bit target data goes in build32, and 64-bit target data goes into build.  So, include/link the
    # appropriate data at build time.
    if (CMAKE_CL_64)
        set (BUILDTGT_DIR build)
        set (BINDATA_DIR Bin)
        set (LIBSOURCE_DIR Lib)
    else()
        set (BUILDTGT_DIR build32)
        set (BINDATA_DIR Bin32)
        set (LIBSOURCE_DIR Lib32)
    endif()
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -fno-strict-aliasing -fno-builtin-memcmp")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS} -std=c++11")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

find_program(GLSLANG_VALIDATOR NAMES glslangValidator
             HINTS ${SDK_BINARY_PATH})
find_program(SPIRV_TOOLS_ASSEMBLER NAMES spirv-as
             HINTS ${SDK_BINARY_PATH})

set (GLMINCLUDES "${CMAKE_SOURCE_DIR}/API-Samples/utils")

get_filename_component(GLMINC_PREFIX "${GLMINCLUDES}" ABSOLUTE)
if(NOT EXISTS ${GLMINC_PREFIX})
    message(FATAL_ERROR "Necessary glm headers do not exist: " ${GLMINC_PREFIX})
endif()

add_definitions(-DVULKAN_SAMPLES_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/API-Samples/utils)
if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}\\..\\Include)
else()
    include_directories("/usr/include/vulkan" "${CMAKE_SOURCE_DIR}/../x86_64/include/vulkan" "${CMAKE_SOURCE_DIR}/include")
endif()

if(WIN32)
    set (MOVE_CMD "move")
    set (VULKAN_LOADER_NAME "${API_LOWERCASE}-${MAJOR}")
else()
    set (MOVE_CMD "mv")
    set (PTHREAD "pthread")
    set (VULKAN_LOADER_NAME "${API_LOWERCASE}")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/layers")
    set (VULKAN_LOADER ${VULKAN_LOADER_NAME})
else()
    find_library(VULKAN_LOADER NAMES ${VULKAN_LOADER_NAME}
                 HINTS "${CMAKE_SOURCE_DIR}/../${LIBSOURCE_DIR}" "${CMAKE_SOURCE_DIR}/../x86_64/lib" )
endif()

add_definitions(-DAPI_NAME="${API_NAME}")

add_subdirectory(API-Samples)
add_subdirectory(Sample-Programs/Hologram)
