PROJECT(glave)
cmake_minimum_required(VERSION 2.8)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${SRC_DIR}/cmake/Modules/")
#set(CMAKE_EXTERNAL_PATH "${SRC_DIR}/../../external")

if (WIN32)
    # TODO: s/CMAKE_PREFIX_PATH/CMAKE_EXTERNAL_WINDOWS_PATH/g
#    set(CMAKE_PREFIX_PATH "${CMAKE_EXTERNAL_PATH}/windows")
    set(WIN32_PTHREADS_PATH "${SRC_DIR}/thirdparty/pthreads.2")
    set(WIN32_PTHREADS_INCLUDE_PATH "${WIN32_PTHREADS_PATH}/include")
endif()

find_package(PythonLibs)
find_package(PythonInterp)

if (NOT PythonLibs_FOUND OR NOT PythonInterp_FOUND)
    message(FATAL_ERROR "Missing Python 3.4 development libraries: sudo apt-get install libpython3.4-dev")
endif()

find_package(Qt5 COMPONENTS Widgets Gui Core)
if (NOT Qt5_FOUND)
    message(WARNING "Missing Qt5 development libraries: sudo apt-get install qt5-default")
endif()

set(CODE_GEN_DIR "glv_extensions/glave_xgl_gen")

#execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../xgl-helper.py --abs_out_dir ${CMAKE_CURRENT_SOURCE_DIR}/src/${CODE_GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../include/xgl.h)


message("")
message("cmake options:")
message("  -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}': Build debug or release. (Debug|Release)")
message("  -DCMAKE_VERBOSE='${CMAKE_VERBOSE}': Spew cmake project options. (On|Off)")
message("  -DBUILD_X64='${BUILD_X64}': Build 32 or 64-bit. (On|Off)")
message("")

#
#  Components to build
#

add_subdirectory(src/thirdparty/getopt)
add_subdirectory(src/glvcommon)
add_subdirectory(src/glvtrace)
add_subdirectory(src/glvreplay)
add_subdirectory(src/glvdebug)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_subdirectory(src/thirdparty/mhook/mhook-lib)
add_subdirectory(src/thirdparty/mhook/disasm-lib)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

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
SUBDIRLIST(SUBDIRS ${SRC_DIR}/glv_extensions)
message("Adding extensions: '${SUBDIRS}'")
FOREACH(subdir ${SUBDIRS})
    add_subdirectory(${SRC_DIR}/glv_extensions/${subdir})
ENDFOREACH()



