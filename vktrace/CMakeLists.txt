PROJECT(vktrace_project)
if (WIN32)
    # For Windows, we need CMake version 3.0+ in order to "find_package(Qt5)":
    cmake_minimum_required(VERSION 3.0)
else()
    cmake_minimum_required(VERSION 2.8)
endif()

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${SRC_DIR}/cmake/Modules/")
#set(CMAKE_EXTERNAL_PATH "${SRC_DIR}/../../external")

if (WIN32)
    # TODO: s/CMAKE_PREFIX_PATH/CMAKE_EXTERNAL_WINDOWS_PATH/g
#    set(CMAKE_PREFIX_PATH "${CMAKE_EXTERNAL_PATH}/windows")
    set(WIN32_PTHREADS_PATH "${SRC_DIR}/thirdparty/pthreads.2")
    set(WIN32_PTHREADS_INCLUDE_PATH "${WIN32_PTHREADS_PATH}/include")
endif()

set(PYTHON_EXECUTABLE ${PYTHON_CMD})
find_package(PythonInterp)

if (NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Missing PythonInterp. Install python interpreter 2.7 (on linux use cmd: sudo apt-get install python2.7)")
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../include/vulkan
)

message("")
message("cmake options:")
message("  -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}': Build debug or release. (Debug|Release)")
message("  -DCMAKE_VERBOSE='${CMAKE_VERBOSE}': Spew cmake project options. (On|Off)")
message("  -DBUILD_X64='${BUILD_X64}': Build 32 or 64-bit. (On|Off)")
message("")

#
#  Components to build
#
set(VKTRACE_VULKAN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(src/vktrace_common)
add_subdirectory(src/vktrace_trace)
add_subdirectory(src/vktrace_layer)
add_subdirectory(src/vktrace_replay)

# use macro from stackoverflow (link below) to get all the extensions that are on the current system
# http://stackoverflow.com/questions/7787823/cmake-how-to-get-the-name-of-all-subdirectories-of-a-directory
MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

# now generate the list and add each of the subdirectories
SUBDIRLIST(SUBDIRS ${SRC_DIR}/vktrace_extensions)
message("Adding extensions: '${SUBDIRS}'")
FOREACH(subdir ${SUBDIRS})
    add_subdirectory(${SRC_DIR}/vktrace_extensions/${subdir})
ENDFOREACH()



