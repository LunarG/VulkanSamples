# The name of our project is "XGL". CMakeLists files in this project can
# refer to the root source directory of the project as ${XGL_SOURCE_DIR} and
# to the root binary directory of the project as ${XGL_BINARY_DIR}.
cmake_minimum_required (VERSION 2.6)
project (XGL)
# set (CMAKE_VERBOSE_MAKEFILE 1)

# Header file for CMake settings
include_directories("${PROJECT_SOURCE_DIR}/include")

include(FindPkgConfig)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Werror -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    endif()
endif()

# Hard code our LunarGLASS and glslang paths for now
get_filename_component(GLSLANG_PREFIX ../glslang ABSOLUTE)
get_filename_component(LUNARGLASS_PREFIX ../LunarGLASS ABSOLUTE)

if(NOT EXISTS ${GLSLANG_PREFIX})
    message(FATAL_ERROR "Necessary glslang components do not exist: " ${GLSLANG_PREFIX})
endif()

if(NOT EXISTS ${LUNARGLASS_PREFIX})
    message(FATAL_ERROR "Necessary LunarGLASS components do not exist: " ${GLSLANG_PREFIX})
endif()

# loader: Generic XGL ICD loader
# icd: Device dependent (DD) XGL components
# tests: XGL tests
add_subdirectory(loader)
add_subdirectory(icd)
add_subdirectory(tests)
add_subdirectory(layers)
add_subdirectory(demos)
add_subdirectory(tools/glave)
