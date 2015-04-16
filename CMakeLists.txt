# The name of our project is "VULKAN". CMakeLists files in this project can
# refer to the root source directory of the project as ${VULKAN_SOURCE_DIR} and
# to the root binary directory of the project as ${VULKAN_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8.11)
project (VULKAN)
# set (CMAKE_VERBOSE_MAKEFILE 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Header file for CMake settings
include_directories("${PROJECT_SOURCE_DIR}/include")

include(FindPkgConfig)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -fno-strict-aliasing -fno-builtin-memcmp")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS}")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

if (NOT WIN32)
    # Hard code our LunarGLASS and glslang paths for now
    get_filename_component(GLSLANG_PREFIX ../glslang ABSOLUTE)
    get_filename_component(LUNARGLASS_PREFIX ../LunarGLASS ABSOLUTE)

    if(NOT EXISTS ${GLSLANG_PREFIX})
        message(FATAL_ERROR "Necessary glslang components do not exist: " ${GLSLANG_PREFIX})
    endif()

    if(NOT EXISTS ${LUNARGLASS_PREFIX})
        message(FATAL_ERROR "Necessary LunarGLASS components do not exist: " ${GLSLANG_PREFIX})
    endif()
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(PYTHON_CMD "python3")
    endif()
else()
        set(PYTHON_CMD "py")
endif()

# loader: Generic VULKAN ICD loader
# icd: Device dependent (DD) VULKAN components
# tests: VULKAN tests
add_subdirectory(loader)
add_subdirectory(icd)
if (NOT WIN32)
    add_subdirectory(tests)
endif()
add_subdirectory(layers)
add_subdirectory(libs)
add_subdirectory(demos)
#add_subdirectory(tools/glave)
