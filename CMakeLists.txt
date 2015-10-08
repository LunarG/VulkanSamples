project (VULKAN_SAMPLES)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

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


set (EXTDIR "${CMAKE_SOURCE_DIR}/ext")
set (GLMINCLUDES "${EXTDIR}")

# to run with source glslang libs/headers
if(UNIX)
    if (EXISTS "${CMAKE_SOURCE_DIR}/../glslang")
         set (GLSLANGDIR "${CMAKE_SOURCE_DIR}/../glslang")
    else()
         set (GLSLANGDIR "${CMAKE_SOURCE_DIR}/../source/glslang")
    endif()
else()
    set (GLSLANGDIR "${CMAKE_SOURCE_DIR}/../glslang")
endif()

get_filename_component(GLMINC_PREFIX "${GLMINCLUDES}" ABSOLUTE)
if(NOT EXISTS ${GLMINC_PREFIX})
    message(FATAL_ERROR "Necessary glm headers do not exist: " ${GLMINC_PREFIX})
endif()

get_filename_component(GLSLANG_PREFIX "${GLSLANGDIR}" ABSOLUTE)
if(NOT EXISTS ${GLSLANG_PREFIX})
    message(FATAL_ERROR "Necessary glslang components do not exist: " ${GLSLANG_PREFIX})
endif()

add_definitions(-DVULKAN_SAMPLES_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(EXISTS "${CMAKE_SOURCE_DIR}/../LoaderAndTools")
    include_directories("${CMAKE_SOURCE_DIR}/../LoaderAndTools/include")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)
if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}\\..\\Include)
else()
    include_directories("/usr/include/vulkan")
endif()

add_subdirectory(utils)

add_subdirectory(src)

