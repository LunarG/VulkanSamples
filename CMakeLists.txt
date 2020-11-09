# The name of our project is "VULKAN_SAMPLES". CMakeLists files in this project can
# refer to the root source directory of the project as ${VULKAN_SOURCE_DIR} and
# to the root binary directory of the project as ${VULKAN_BINARY_DIR}.
cmake_minimum_required(VERSION 3.10.2)
project (VULKAN_SAMPLES)
# set (CMAKE_VERBOSE_MAKEFILE 1)

# The API_NAME allows renaming builds to avoid conflicts with installed SDKs
# The MAJOR number of the version we're building, used in naming
# <api-name>-<major>.dll (and other files).
set(API_NAME "Vulkan" CACHE STRING "API name to use when building")
set(MAJOR "1")
string(TOLOWER ${API_NAME} API_LOWERCASE)
set(VULKAN_HEADERS_INSTALL_DIR "HEADERS-NOTFOUND" CACHE PATH "Absolute path to a Vulkan-Headers install directory")
if(NOT VULKAN_LOADER_INSTALL_DIR AND NOT DEFINED ENV{VULKAN_LOADER_INSTALL_DIR})
    message(STATUS "Using local loader")
    if(CMAKE_CL_64)
        set(VULKAN_LOADER_INSTALL_DIR "${CMAKE_SOURCE_DIR}/external/x64")
    else()
        set(VULKAN_LOADER_INSTALL_DIR "${CMAKE_SOURCE_DIR}/external/x86")
    endif()
endif()
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};${VULKAN_HEADERS_INSTALL_DIR};${VULKAN_LOADER_INSTALL_DIR};
                       $ENV{VULKAN_HEADERS_INSTALL_DIR};$ENV{VULKAN_LOADER_INSTALL_DIR})
  
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(PythonInterp 3 REQUIRED)
message(STATUS "Using find_package to locate Vulkan")
find_package(Vulkan)
message(STATUS "Vulkan FOUND = ${Vulkan_FOUND}")
message(STATUS "Vulkan Include = ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan Lib = ${Vulkan_LIBRARY}")

option(USE_CCACHE "Use ccache" OFF)
if (USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif(CCACHE_FOUND)
endif()

include(GNUInstallDirs)
# Set a better default install location for Windows only if the user did not provide one.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "default install path" FORCE)
endif()

# Enable cmake folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FALLBACK_CONFIG_DIRS "/etc/xdg" CACHE STRING
        "Search path to use when XDG_CONFIG_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant.")
    set(FALLBACK_DATA_DIRS "/usr/local/share:/usr/share" CACHE STRING
        "Search path to use when XDG_DATA_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant.")

    include(FindPkgConfig)
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" ON)
    set(DEMOS_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for demos (XCB, XLIB, WAYLAND, DISPLAY)")
    set(SAMPLES_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for api-samples (XCB, WAYLAND, DISPLAY)")

    if (BUILD_WSI_XCB_SUPPORT)
        find_package(XCB REQUIRED)
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
        find_package(X11 REQUIRED)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
        find_package(Wayland REQUIRED)
        include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
    endif()
endif()

if(WIN32)
    # Treat warnings as errors
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/WX>")
    # Disable RTTI
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/GR->")
    # Warn about nested declarations
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34456>")
    # Warn about potentially uninitialized variables
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34701>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34703>")
    # Warn about different indirection types.
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34057>")
    # Warn about signed/unsigned mismatch.
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34245>")
endif()

set(SCRIPTS_DIR "${PROJECT_SOURCE_DIR}/scripts")
set(GLSLANG_VALIDATOR_NAME "glslangValidator")
message(STATUS "Using cmake find_program to look for glslangValidator")
if(WIN32)
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_glslangvalidator.py glslang-master-windows-x64-Release.zip)
    set(GLSLANG_VALIDATOR_NAME "glslangValidator.exe")
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_spirv_tools.py SPIRV-Tools-master-windows-x64-Release.zip)
    set(SPIRV_TOOLS_ASSEMBLER_NAME "spirv-as.exe")
elseif(APPLE)
    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_glslangvalidator.py glslang-master-osx-Release.zip)
elseif(UNIX AND NOT APPLE) # i.e. Linux
    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_glslangvalidator.py glslang-master-linux-Release.zip)
    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_spirv_tools.py SPIRV-Tools-master-linux-RelWithDebInfo.zip)
    set(SPIRV_TOOLS_ASSEMBLER_NAME "spirv-as")
endif()
find_program(GLSLANG_VALIDATOR NAMES ${GLSLANG_VALIDATOR_NAME} HINTS "${PROJECT_SOURCE_DIR}/glslang/bin")
find_program(SPIRV_TOOLS_ASSEMBLER NAMES ${SPIRV_TOOLS_ASSEMBLER_NAME} HINTS "${PROJECT_SOURCE_DIR}/spirv-tools/bin")
	
if(NOT WIN32)
    set (BUILDTGT_DIR build)
    set (BINDATA_DIR Bin)
    set (LIBSOURCE_DIR Lib)
else()
    # is WIN32
    option(DISABLE_BUILD_PATH_DECORATION "Disable the decoration of the gslang and SPIRV-Tools build path with MSVC build type info" OFF)
    option(DISABLE_BUILDTGT_DIR_DECORATION "Disable the decoration of the gslang and SPIRV-Tools build path with target info" OFF)

    # For Windows, since 32-bit and 64-bit items can co-exist, we build each in its own build directory.
    # 32-bit target data goes in build32, and 64-bit target data goes into build.  So, include/link the
    # appropriate data at build time.
    if (DISABLE_BUILDTGT_DIR_DECORATION)
        set (BUILDTGT_DIR "")
        set (BINDATA_DIR "")
        set (LIBSOURCE_DIR "")
    elseif (CMAKE_CL_64)
        set (BUILDTGT_DIR build)
        set (BINDATA_DIR Bin)
        set (LIBSOURCE_DIR Lib)
    else()
        set (BUILDTGT_DIR build32)
        set (BINDATA_DIR Bin32)
        set (LIBSOURCE_DIR Lib32)
    endif()
endif()

add_definitions(-DAPI_NAME="${API_NAME}")

if (WIN32)
    if(DISABLE_BUILD_PATH_DECORATION)
        set (DEBUG_DECORATION "")
        set (RELEASE_DECORATION "")
    else()
        set (DEBUG_DECORATION "Debug")
        set (RELEASE_DECORATION "Release")
    endif()
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -fno-strict-aliasing -fno-builtin-memcmp")

    # For GCC version 7.1 or greater, we need to disable the implicit fallthrough warning since
    # there's no consistent way to satisfy all compilers until they all accept the C++17 standard
    if (CMAKE_COMPILER_IS_GNUCC AND NOT (CMAKE_CXX_COMPILER_VERSION LESS 7.1))
        set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -Wimplicit-fallthrough=0")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS} -std=c++11 -fno-rtti")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

set(VULKAN_HEADERS_INSTALL_DIR "VULKAN-HEADERS-NOTFOUND" CACHE PATH "Absolute path to a Vulkan-Headers install directory")
if(NOT VULKAN_HEADERS_INSTALL_DIR AND NOT DEFINED ENV{VULKAN_HEADERS_INSTALL_DIR})
    message(FATAL_ERROR "Must define location of vulkan headers -- see BUILD.md")
endif()

# Cmake command line option overrides environment variable
if(NOT VULKAN_HEADERS_INSTALL_DIR)
    set(VULKAN_HEADERS_INSTALL_DIR $ENV{VULKAN_HEADERS_INSTALL_DIR})
endif()

# Cmake command line option overrides environment variable
if(NOT VULKAN_LOADER_INSTALL_DIR)
    set(VULKAN_LOADER_INSTALL_DIR $ENV{VULKAN_LOADER_INSTALL_DIR})
endif()

set (PYTHON_CMD ${PYTHON_EXECUTABLE})

set (UTILS_NAME vsamputils)

set (GLMINCLUDES "${CMAKE_SOURCE_DIR}/API-Samples/utils")

get_filename_component(GLMINC_PREFIX "${GLMINCLUDES}" ABSOLUTE)
if(NOT EXISTS ${GLMINC_PREFIX})
    message(FATAL_ERROR "Necessary glm headers do not exist: " ${GLMINC_PREFIX})
endif()

add_definitions(-DVULKAN_SAMPLES_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/API-Samples/utils)

option(BUILD_API_SAMPLES "Build API Samples " ON)
option(BUILD_SAMPLE_LAYERS "Build Sample Layers " OFF) # Not brought forward after repository split

if((CMAKE_SYSTEM_NAME STREQUAL "Linux") AND (NOT BUILD_WSI_XCB_SUPPORT) AND (NOT BUILD_WSI_WAYLAND_SUPPORT))
    set(BUILD_API_SAMPLES OFF)
    set(BUILD_SAMPLE_LAYERS OFF)
endif()

if (BUILD_API_SAMPLES)
    add_subdirectory(API-Samples)
endif()

add_subdirectory(Sample-Programs/Hologram)

