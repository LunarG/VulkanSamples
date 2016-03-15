# The name of our project is "VULKAN". CMakeLists files in this project can
# refer to the root source directory of the project as ${VULKAN_SOURCE_DIR} and
# to the root binary directory of the project as ${VULKAN_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8.11)
project (VULKAN)
# set (CMAKE_VERBOSE_MAKEFILE 1)

# The MAJOR number of the version we're building, used in naming
# vulkan-<major>.dll (and other files).
set(MAJOR "1")

find_package(PythonInterp 3 REQUIRED)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    set(DisplayServer Win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
    set(DisplayServer Android)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #   TODO: Basic support is present for Xlib but is untested.
    #         Mir support is stubbed in but unimplemented and untested.
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" OFF)
    option(BUILD_WSI_MIR_SUPPORT "Build Mir WSI support" OFF)

    if (BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
        set(DisplayServer Xcb)
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
        set(DisplayServer Xlib)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
        set(DisplayServer Wayland)
    endif()

    if (BUILD_WSI_MIR_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
        set(DisplayServer Mir)
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Header file for CMake settings
include_directories("${PROJECT_SOURCE_DIR}/include")

if(NOT WIN32)
    include(FindPkgConfig)
endif()

set (CMAKE_INSTALL_PREFIX "")

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

if(NOT WIN32)
    find_package(XCB REQUIRED)
    set (BUILDTGT_DIR build)
    set (BINDATA_DIR Bin)
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

option(BUILD_LOADER "Build loader" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_LAYERS "Build layers" ON)
option(BUILD_DEMOS "Build demos" ON)
option(BUILD_VKJSON "Build vkjson" ON)

find_program(GLSLANG_VALIDATOR NAMES glslangValidator
             HINTS "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/install/bin"
                   "${PROJECT_SOURCE_DIR}/../${BINDATA_DIR}" )

find_path(GLSLANG_SPIRV_INCLUDE_DIR SPIRV/spirv.hpp HINTS "${CMAKE_SOURCE_DIR}/../glslang" DOC "Path to SPIRV/spirv.hpp")
find_path(SPIRV_TOOLS_INCLUDE_DIR spirv-tools/libspirv.h HINTS "${CMAKE_SOURCE_DIR}/../spirv-tools/include"
                                                               "${CMAKE_SOURCE_DIR}/../source/spirv-tools/include"
                                                               "${CMAKE_SOURCE_DIR}/../spirv-tools/external/include"
                                                               "${CMAKE_SOURCE_DIR}/../source/spirv-tools/external/include"
                                                         DOC "Path to spirv-tools/libspirv.h")

if (WIN32)
    set (GLSLANG_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/glslang/Release"
                             "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/glslang/OSDependent/Windows/Release"
                             "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/OGLCompilersDLL/Release"
                             "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/SPIRV/Release" )
    set (SPIRV_TOOLS_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../spirv-tools/${BUILDTGT_DIR}/Release")
else()
    set (GLSLANG_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../glslang/build/install/lib" "${CMAKE_SOURCE_DIR}/../x86_64/lib/glslang" )
    set (SPIRV_TOOLS_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../spirv-tools/build" "${CMAKE_SOURCE_DIR}/../x86_64/lib/spirv-tools" )
endif()

find_library(GLSLANG_LIB NAMES glslang
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(OGLCompiler_LIB NAMES OGLCompiler
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(OSDependent_LIB NAMES OSDependent
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(SPIRV_LIB NAMES SPIRV
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(SPIRV_TOOLS_LIB NAMES SPIRV-Tools
             HINTS ${SPIRV_TOOLS_SEARCH_PATH} )

# On Windows, we must pair Debug and Release appropriately
if (WIN32)
    set (GLSLANG_DEBUG_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/glslang/Debug"
                                   "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/glslang/OSDependent/Windows/Debug"
                                   "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/OGLCompilersDLL/Debug"
                                   "${CMAKE_SOURCE_DIR}/../glslang/${BUILDTGT_DIR}/SPIRV/Debug")
    set (SPIRV_TOOLS_DEBUG_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../spirv-tools/${BUILDTGT_DIR}/Debug")

    add_library(glslang     STATIC IMPORTED)
    add_library(OGLCompiler STATIC IMPORTED)
    add_library(OSDependent STATIC IMPORTED)
    add_library(SPIRV       STATIC IMPORTED)
    add_library(Loader      STATIC IMPORTED)
    add_library(SPIRV-Tools STATIC IMPORTED)

    find_library(GLSLANG_DLIB NAMES glslang
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(OGLCompiler_DLIB NAMES OGLCompiler
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(OSDependent_DLIB NAMES OSDependent
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(SPIRV_DLIB NAMES SPIRV
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(SPIRV_TOOLS_DLIB NAMES SPIRV-Tools
                 HINTS ${SPIRV_TOOLS_DEBUG_SEARCH_PATH} )

    set_target_properties(glslang PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_LIB}"
                         IMPORTED_LOCATION_DEBUG "${GLSLANG_DLIB}")
    set_target_properties(OGLCompiler PROPERTIES
                         IMPORTED_LOCATION       "${OGLCompiler_LIB}"
                         IMPORTED_LOCATION_DEBUG "${OGLCompiler_DLIB}")
    set_target_properties(OSDependent PROPERTIES
                         IMPORTED_LOCATION       "${OSDependent_LIB}"
                         IMPORTED_LOCATION_DEBUG "${OSDependent_DLIB}")
    set_target_properties(SPIRV PROPERTIES
                         IMPORTED_LOCATION       "${SPIRV_LIB}"
                         IMPORTED_LOCATION_DEBUG "${SPIRV_DLIB}")
    set_target_properties(SPIRV-Tools PROPERTIES
                         IMPORTED_LOCATION       "${SPIRV_TOOLS_LIB}"
                         IMPORTED_LOCATION_DEBUG "${SPIRV_TOOLS_DLIB}")

    set (GLSLANG_LIBRARIES glslang OGLCompiler OSDependent SPIRV)
    set (SPIRV_TOOLS_LIBRARIES SPIRV-Tools)
else ()
    set (GLSLANG_LIBRARIES ${GLSLANG_LIB} ${OGLCompiler_LIB} ${OSDependent_LIB} ${SPIRV_LIB})
    set (SPIRV_TOOLS_LIBRARIES ${SPIRV_TOOLS_LIB})
endif()

set (PYTHON_CMD ${PYTHON_EXECUTABLE})

if(NOT WIN32)
    include(GNUInstallDirs)
    add_definitions(-DSYSCONFDIR="${CMAKE_INSTALL_SYSCONFDIR}")
    add_definitions(-DDATADIR="${CMAKE_INSTALL_DATADIR}")
    if (CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    elseif (CMAKE_INSTALL_PREFIX STREQUAL "")
    else()
        add_definitions(-DLOCALPREFIX="${CMAKE_INSTALL_PREFIX}")
    endif()
endif()

# loader: Generic VULKAN ICD loader
# tests: VULKAN tests
if(BUILD_LOADER)
    add_subdirectory(loader)
endif()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(BUILD_LAYERS)
    add_subdirectory(layers)
endif()

if(BUILD_DEMOS)
    add_subdirectory(demos)
endif()

if(BUILD_VKJSON)
    add_subdirectory(libs/vkjson)
endif()
