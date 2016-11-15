# The name of our project is "VULKAN". CMakeLists files in this project can
# refer to the root source directory of the project as ${VULKAN_SOURCE_DIR} and
# to the root binary directory of the project as ${VULKAN_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8.11)
project (VULKAN_SAMPLES)
# set (CMAKE_VERBOSE_MAKEFILE 1)

# The API_NAME allows renaming builds to avoid conflicts with installed SDKs
# The MAJOR number of the version we're building, used in naming
# <api-name>-<major>.dll (and other files).
set(API_NAME "Vulkan" CACHE STRING "API name to use when building")
set(MAJOR "1")
string(TOLOWER ${API_NAME} API_LOWERCASE)

find_package(PythonInterp 3 REQUIRED)

# Set CMAKE_SYSTEM_NAME to Android if we're building Android Studio project.
if(ANDROID)
    set (CMAKE_SYSTEM_NAME Android)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    set(DisplayServer Win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
    set(DisplayServer Android)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #   Note: Supported configurations are XCB, XCB + Xlib, Wayland.
    #         MIR is stubbed and untested
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

    if (NOT BUILD_WSI_XCB_SUPPORT AND NOT BUILD_WSI_XLIB_SUPPORT AND NOT BUILD_WSI_WAYLAND_SUPPORT AND NOT BUILD_WSI_MIR_SUPPORT)
        set(DisplayServer Display)
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Header file for CMake settings
include_directories("${PROJECT_SOURCE_DIR}/include")

if(NOT WIN32)
    include(FindPkgConfig)
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -fno-strict-aliasing -fno-builtin-memcmp")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS} -std=c++11 -fno-rtti")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

if(WIN32)
    # Disable RTTI
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
endif()

if(NOT WIN32)
    if (BUILD_WSI_XCB_SUPPORT)
        find_package(XCB REQUIRED)
    endif()
    set (BUILDTGT_DIR build)
    set (BINDATA_DIR Bin)
    set (LIBSOURCE_DIR Lib)
else()
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

option(BUILD_LOADER "Build loader" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_LAYERS "Build layers" ON)
option(BUILD_DEMOS "Build demos" ON)
option(BUILD_VKJSON "Build vkjson" ON)
option(BUILD_UTILITIES "Build WSIWindow and Teapots" ON)
option(CUSTOM_GLSLANG_BIN_ROOT "Use the user defined GLSLANG_BINARY_ROOT" OFF)
option(CUSTOM_SPIRV_TOOLS_BIN_ROOT "Use the user defined SPIRV_TOOLS_BINARY_ROOT" OFF)

#Choose natural default paths for glslang and SPIRV-Tools binaries to support custom definition by the user on the CMake command line or in the GUI
set(GLSLANG_BINARY_ROOT "${CMAKE_BINARY_DIR}/../glslang" CACHE STRING "User defined path to the glslang binaries for this project")
set(SPIRV_TOOLS_BINARY_ROOT "${CMAKE_BINARY_DIR}/../SPIRV-Tools" CACHE STRING "User defined path to the SPIRV-Tools binaries for this project")

# Define a variable for a default root location to the gslang, SPIRV-Tools and other external sources and cache it to allow the user to customize it as needed
set(EXTERNAL_SOURCE_ROOT "${CMAKE_SOURCE_DIR}/external" CACHE STRING "Root path to external sources such as glslang and SPIRV-Tools")


if (WIN32)
    if(CUSTOM_GLSLANG_BIN_ROOT)
        set(GSLANG_FINAL_BINARY_PATH ${GLSLANG_BINARY_ROOT}/${BUILDTGT_DIR})
    else()
        set(GSLANG_FINAL_BINARY_PATH "${EXTERNAL_SOURCE_ROOT}/glslang/${BUILDTGT_DIR}")
    endif()

    if(DISABLE_BUILD_PATH_DECORATION)
        set (DEBUG_DECORATION "")
        set (RELEASE_DECORATION "")
    else()
        set (DEBUG_DECORATION "Debug")
        set (RELEASE_DECORATION "Release")
    endif()

    # Take some steps to set up a variable pointing to the final glslang binaries given the variety of input options
    set (GLSLANG_SEARCH_PATH "${GSLANG_FINAL_BINARY_PATH}/glslang/${RELEASE_DECORATION}"
                             "${GSLANG_FINAL_BINARY_PATH}/glslang/OSDependent/Windows/${RELEASE_DECORATION}"
                             "${GSLANG_FINAL_BINARY_PATH}/hlsl/${RELEASE_DECORATION}"
                             "${GSLANG_FINAL_BINARY_PATH}/OGLCompilersDLL/${RELEASE_DECORATION}"
                             "${GSLANG_FINAL_BINARY_PATH}/SPIRV/${RELEASE_DECORATION}" )

    set (GLSLANG_DEBUG_SEARCH_PATH "${GSLANG_FINAL_BINARY_PATH}/glslang/${DEBUG_DECORATION}"
                                   "${GSLANG_FINAL_BINARY_PATH}/glslang/OSDependent/Windows/${DEBUG_DECORATION}"
                                   "${GSLANG_FINAL_BINARY_PATH}/hlsl/${DEBUG_DECORATION}"
                                   "${GSLANG_FINAL_BINARY_PATH}/OGLCompilersDLL/${DEBUG_DECORATION}"
                                   "${GSLANG_FINAL_BINARY_PATH}/SPIRV/${DEBUG_DECORATION}")

    if(CUSTOM_SPIRV_TOOLS_BIN_ROOT)
        set (SPIRV_TOOLS_SEARCH_PATH "${SPIRV_TOOLS_BINARY_ROOT}/${BUILDTGT_DIR}/source/${RELEASE_DECORATION}")
        set (SPIRV_TOOLS_DEBUG_SEARCH_PATH "${SPIRV_TOOLS_BINARY_ROOT}/${BUILDTGT_DIR}/source/${DEBUG_DECORATION}")
    else()
        set (SPIRV_TOOLS_SEARCH_PATH "${EXTERNAL_SOURCE_ROOT}/spirv-tools/${BUILDTGT_DIR}/source/${RELEASE_DECORATION}")
        set (SPIRV_TOOLS_DEBUG_SEARCH_PATH "${EXTERNAL_SOURCE_ROOT}/spirv-tools/${BUILDTGT_DIR}/source/${DEBUG_DECORATION}")
    endif()
else()
    #non windows
    if(CUSTOM_GLSLANG_BIN_ROOT)
        set (GLSLANG_SEARCH_PATH "${GLSLANG_BINARY_ROOT}/install/lib"
                                 "${GLSLANG_BINARY_ROOT}/glslang"
                                 "${GLSLANG_BINARY_ROOT}/glslang/OSDependent/Unix"
                                 "${GLSLANG_BINARY_ROOT}/OGLCompilersDLL"
                                 "${GLSLANG_BINARY_ROOT}/SPIRV"
                                 "${GLSLANG_BINARY_ROOT}/hlsl"
                                 "${GLSLANG_BINARY_ROOT}/StandAlone")
    else()
        set (GLSLANG_SEARCH_PATH "${EXTERNAL_SOURCE_ROOT}/glslang/${BUILDTGT_DIR}/install/lib" "${CMAKE_SOURCE_DIR}/../x86_64/lib/glslang" )
    endif()

    if(CUSTOM_SPIRV_TOOLS_BIN_ROOT)
        set (SPIRV_TOOLS_SEARCH_PATH "${SPIRV_TOOLS_BINARY_ROOT}/source" )
    else()
        set (SPIRV_TOOLS_SEARCH_PATH "${EXTERNAL_SOURCE_ROOT}/spirv-tools/${BUILDTGT_DIR}/source" "${CMAKE_SOURCE_DIR}/../x86_64/lib/spirv-tools" )
    endif()
endif()

find_program(GLSLANG_VALIDATOR NAMES glslangValidator
             HINTS "${CMAKE_SOURCE_DIR}/external/glslang/${BUILDTGT_DIR}/install/bin"
                   "${GLSLANG_BINARY_ROOT}/StandAlone"
                   "${PROJECT_SOURCE_DIR}/external/${BINDATA_DIR}")

find_path(GLSLANG_SPIRV_INCLUDE_DIR SPIRV/spirv.hpp HINTS "${EXTERNAL_SOURCE_ROOT}/glslang"
                                                    "${CMAKE_SOURCE_DIR}/../glslang"
                                              DOC "Path to SPIRV/spirv.hpp")

find_path(SPIRV_TOOLS_INCLUDE_DIR spirv-tools/libspirv.h HINTS "${EXTERNAL_SOURCE_ROOT}/spirv-tools/include"
                                                   "${EXTERNAL_SOURCE_ROOT}/SPIRV-Tools/include"
                                                   "${CMAKE_SOURCE_DIR}/../spirv-tools/include"
                                                   "${CMAKE_SOURCE_DIR}/../SPIRV-Tools/include"
                                                   "${EXTERNAL_SOURCE_ROOT}/source/spirv-tools/external/include"
                                             DOC "Path to spirv-tools/libspirv.h")

find_library(GLSLANG_LIB NAMES glslang
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(OGLCompiler_LIB NAMES OGLCompiler
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(OSDependent_LIB NAMES OSDependent
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(HLSL_LIB NAMES HLSL
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(SPIRV_LIB NAMES SPIRV
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(SPIRV_REMAPPER_LIB NAMES SPVRemapper
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(SPIRV_TOOLS_LIB NAMES SPIRV-Tools
             HINTS ${SPIRV_TOOLS_SEARCH_PATH} )

if (WIN32)
    add_library(glslang     STATIC IMPORTED)
    add_library(OGLCompiler STATIC IMPORTED)
    add_library(OSDependent STATIC IMPORTED)
    add_library(HLSL        STATIC IMPORTED)
    add_library(SPIRV       STATIC IMPORTED)
    add_library(SPVRemapper       STATIC IMPORTED)
    add_library(Loader      STATIC IMPORTED)
    add_library(SPIRV-Tools STATIC IMPORTED)

    find_library(GLSLANG_DLIB NAMES glslangd
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(OGLCompiler_DLIB NAMES OGLCompilerd
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(OSDependent_DLIB NAMES OSDependentd
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(HLSL_DLIB NAMES HLSLd
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(SPIRV_DLIB NAMES SPIRVd
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(SPIRV_REMAPPER_DLIB NAMES SPVRemapperd
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
    set_target_properties(HLSL PROPERTIES
                         IMPORTED_LOCATION       "${HLSL_LIB}"
                         IMPORTED_LOCATION_DEBUG "${HLSL_DLIB}")
    set_target_properties(SPIRV PROPERTIES
                         IMPORTED_LOCATION       "${SPIRV_LIB}"
                         IMPORTED_LOCATION_DEBUG "${SPIRV_DLIB}")
    set_target_properties(SPVRemapper PROPERTIES
                         IMPORTED_LOCATION       "${SPIRV_REMAPPER_LIB}"
                         IMPORTED_LOCATION_DEBUG "${SPIRV_REMAPPER_DLIB}")
    set_target_properties(SPIRV-Tools PROPERTIES
                         IMPORTED_LOCATION       "${SPIRV_TOOLS_LIB}"
                         IMPORTED_LOCATION_DEBUG "${SPIRV_TOOLS_DLIB}")

    set (GLSLANG_LIBRARIES glslang OGLCompiler OSDependent HLSL SPIRV SPVRemapper)
    set (SPIRV_TOOLS_LIBRARIES SPIRV-Tools)
else ()
    set (GLSLANG_LIBRARIES ${GLSLANG_LIB} ${OGLCompiler_LIB} ${OSDependent_LIB} ${HLSL_LIB} ${SPIRV_LIB} ${SPIRV_REMAPPER_LIB})
    set (SPIRV_TOOLS_LIBRARIES ${SPIRV_TOOLS_LIB})
endif()

set (PYTHON_CMD ${PYTHON_EXECUTABLE})

if(NOT WIN32)
    include(GNUInstallDirs)

    add_definitions(-DSYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}")
    add_definitions(-DDATADIR="${CMAKE_INSTALL_FULL_DATADIR}")

    # Make sure /etc is searched by the loader
    if (NOT (CMAKE_INSTALL_FULL_SYSCONFDIR STREQUAL "/etc"))
        add_definitions(-DEXTRASYSCONFDIR="/etc")
    endif()

    # Make sure /usr/share is searched by the loader
    if (NOT (CMAKE_INSTALL_FULL_DATADIR STREQUAL "/usr/share"))
        add_definitions(-DEXTRADATADIR="/usr/share")
    endif()
endif()

if(UNIX)
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/vulkan" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
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

if(BUILD_UTILITIES)
    add_subdirectory(Utilities)
endif()

set (UTILS_NAME vsamputils)

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
    include_directories("/usr/include/vulkan")
endif()

add_subdirectory(API-Samples)
add_subdirectory(Sample-Programs/Hologram)
add_subdirectory(Layer-Samples/Overlay)
