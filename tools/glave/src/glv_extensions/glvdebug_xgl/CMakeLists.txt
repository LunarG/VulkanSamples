cmake_minimum_required(VERSION 2.8)

# this project is currently only available on Linux
#if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

project(glvdebug_xgl)

include("${SRC_DIR}/build_options.cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../../..)

find_package(Qt5 COMPONENTS Widgets Gui Core Svg QUIET)

if(NOT Qt5_FOUND)
# After Qt5.3 is installed, you may need to add the following to the cmake command line:
# -DQt5_DIR=/home/<username>/Qt/5.3/gcc_64/lib/cmake/Qt5
message(WARNING "WARNING: glvdebug_xgl will be excluded because Qt5 was not found.")
else()

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${SRC_DIR}/../../../include)
# This file is shared between replayer and tracer and are generated to each dir.  Should generate them to common area
add_custom_command(OUTPUT glvtrace_xgl_packet_id.h glvtrace_xgl_xgl_structs.h glvtrace_xgl_xglwsix11ext_structs.h glvtrace_xgl_xgldbg_structs.h xgl_enum_string_helper.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../../../glave-generate.py glave-packet-id > glvtrace_xgl_packet_id.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../../../glave-generate.py glave-core-structs > glvtrace_xgl_xgl_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../../../glave-generate.py glave-wsi-trace-structs > glvtrace_xgl_xglwsix11ext_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../../../glave-generate.py glave-dbg-trace-structs > glvtrace_xgl_xgldbg_structs.h
        COMMAND ${PYTHON_CMD} ${SRC_DIR}/../../../xgl_helper.py --gen_enum_string_helper ${SRC_DIR}/../../../include/xgl.h --abs_out_dir ./
	           DEPENDS ${SRC_DIR}/../../../glave-generate.py
                           ${SRC_DIR}/../../../xgl_helper.py)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(PATH_TO_WIN_XGL ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/${CMAKE_CFG_INTDIR})
    set(OS_REPLAYER_LIBS
        ${PATH_TO_WIN_XGL}/XGL.lib
)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OS_REPLAYER_LIBS
    ${CMAKE_CURRENT_BINARY_DIR}/../../../../../loader/libXGL.so
    xcb
)
endif()

set(SRC_LIST
    ${SRC_LIST}
    glvdebug_xgl.cpp
    glvdebug_xgl_settings.cpp
    glvdebug_xgl_qcontroller.cpp
    glvdebug_xgl_qfile_model.cpp
    ${SRC_DIR}/glvdebug/glvdebug_QReplayWorker.cpp
    ${SRC_DIR}/glvreplay/glvreplay_factory.cpp
)

# This should only contain headers that define a QOBJECT
# Typically that means just headers for UI objects
set(UI_HEADER_LIST
    glvdebug_xgl_qcontroller.h
    glvdebug_xgl_qfile_model.h
    glvdebug_xgl_qgroupframesproxymodel.h
    ${SRC_DIR}/glvdebug/glvdebug_qgroupthreadsproxymodel.h
    ${SRC_DIR}/glvdebug/glvdebug_qimageviewer.h
    ${SRC_DIR}/glvdebug/glvdebug_qsvgviewer.h
    ${SRC_DIR}/glvdebug/glvdebug_QTraceFileModel.h
    ${SRC_DIR}/glvdebug/glvdebug_QReplayWidget.h
    ${SRC_DIR}/glvdebug/glvdebug_QReplayWorker.h
)

set(HDR_LIST
    glvtrace_xgl_packet_id.h
    glvdebug_xgl_settings.h
    glvdebug_xgl_qgroupframesproxymodel.h
    glvtrace_xgl_xgl_structs.h
    glvtrace_xgl_xglwsix11ext_structs.h
    glvtrace_xgl_xgldbg_structs.h
    ${SRC_DIR}/glvdebug/glvdebug_qgroupthreadsproxymodel.h
    ${SRC_DIR}/glvdebug/glvdebug_controller.h
    ${SRC_DIR}/glvreplay/glvreplay_factory.h
)

include_directories(
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/glvdebug
    ${SRC_DIR}/glvreplay
    ${SRC_DIR}/glv_extensions/glvtrace_xgl
    ${SRC_DIR}/thirdparty
    ${Qt5Widgets_INCLUDE_DIRS}
)

QT5_WRAP_CPP(QT_GEN_HEADER_MOC_LIST ${UI_HEADER_LIST})

add_library(${PROJECT_NAME} SHARED ${SRC_LIST} ${HDR_LIST}
    ${QT_GEN_HEADER_MOC_LIST}
)

target_link_libraries(${PROJECT_NAME} 
    Qt5::Widgets
    Qt5::Core
    Qt5::Svg
    ${OS_REPLAYER_LIBS}
    glvcommon
)

build_options_finalize()

endif(NOT Qt5_FOUND)
#endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
