project(glvdebug)
cmake_minimum_required(VERSION 2.8.9)

include("${SRC_DIR}/build_options.cmake")

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${SRC_DIR}/cmake/Modules/")

# we want cmake to link the Qt libs into the binary
# This policy was introduced in 2.8.11, so on newer versions, use the OLD policy to maintain consistent behavior
if (POLICY CMP0020)
   cmake_policy(SET CMP0020 OLD)
endif()

find_package(Qt5 COMPONENTS Widgets Gui Core QUIET)

if(NOT Qt5_FOUND)
# After Qt5.3 is installed, you may need to add the following to the cmake command line:
# -DQt5_DIR=/home/<username>/Qt/5.3/gcc_64/lib/cmake/Qt5
message(WARNING "WARNING: glvdebug will be excluded because Qt5 was not found.")
else()

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

require_pthreads()

include_directories(
    ${SRC_DIR}
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/glvreplay
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Qt5Widgets_INCLUDE_DIRS}
)

set(SRC_LIST
    main.cpp
    glvdebug.cpp
    glvdebug_settings.cpp
    glvdebug_output.cpp
    glvdebug_trace_file_utils.cpp
    glvdebug_qtimelineview.cpp
    glvdebug_timelineitem.cpp
    glvdebug_timelinemodel.cpp
    glvdebug_controller_factory.cpp
    ${SRC_DIR}/glvreplay/glvreplay_seq.cpp
    ${SRC_DIR}/glvreplay/glvreplay_factory.cpp
   )

# This should only contain headers that define a QOBJECT
# Typically that means just headers for UI objects
set(UI_HEADER_LIST
    glvdebug.h
    glvdebug_qtimelineview.h
    glvdebug_QReplayWidget.h
    glvdebug_QReplayWorker.h
    glvdebug_QTraceFileModel.h
   )

# These is for all headers
set(HEADER_LIST
    glvdebug.h
    glvdebug_settings.h
    glvdebug_output.h
    glvdebug_controller.h
    glvdebug_view.h
    glvdebug_trace_file_utils.h
    glvdebug_timelineitem.h
    glvdebug_timelinemodel.h
    glvdebug_controller_factory.h
    glvdebug_qtimelineview.h
    glvdebug_QReplayWidget.h
    glvdebug_QReplayWorker.h
    glvdebug_QTraceFileModel.h
   )

set(FORM_LIST
    glvdebug.ui
   )

set(RESOURCE_LIST
   )

QT5_WRAP_CPP(QT_GEN_HEADER_MOC_LIST ${UI_HEADER_LIST})
QT5_WRAP_UI(QT_GEN_FORM_HEADER_LIST ${FORM_LIST})
QT5_ADD_RESOURCES(QT_GEN_RESOURCE_RCC_LIST ${RESOURCE_LIST})

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    add_compiler_flag("-Wno-global-constructors -Wno-used-but-marked-unused")
endif()

# Platform specific compile flags.
if (NOT MSVC)
    add_compiler_flag("-fPIC")
endif()

add_executable(${PROJECT_NAME} ${SRC_LIST} ${HEADER_LIST}
    ${QT_GEN_HEADER_MOC_LIST}
    ${QT_GEN_FORM_HEADER_LIST}
    ${QT_GEN_RESOURCE_RCC_LIST}
   )

if (TARGET SDL)
    add_dependencies(${PROJECT_NAME} SDL)
endif ()

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Core
    glvcommon
    ${CMAKE_DL_LIBS}
    ${X11_X11_LIB}
)

if (MSVC)
  # copy the debug and release dlls for Qt5Widgets, Qt5Core, and Qt5Gui
  set (COPY_DEST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})

  get_property(qt5_qmake_executable TARGET ${Qt5Core_QMAKE_EXECUTABLE} PROPERTY IMPORT_LOCATION)
  execute_process(COMMAND ${qt5_qmake_executable} -query QT_INSTALL_PLUGINS OUTPUT_VARIABLE QT_INSTALL_PLUGINS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${qt5_qmake_executable} -query QT_INSTALL_BINS OUTPUT_VARIABLE QT_INSTALL_BINS OUTPUT_STRIP_TRAILING_WHITESPACE)

  # There are also several other files that need to be copied or created
  FILE(WRITE ${CMAKE_CURRENT_LIST_DIR}/qt.conf "[Paths]\nPlugins=.")
  add_custom_target(copy_deps_qt5 ALL
                    ${CMAKE_COMMAND} -E make_directory "${COPY_DEST}/platforms"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/qt.conf" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/Qt5Widgets.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/Qt5Widgetsd.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/Qt5Core.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/Qt5Cored.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/Qt5Gui.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/Qt5Guid.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/icudt52.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/icuin52.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_BINS}/icuuc52.dll" "${COPY_DEST}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_PLUGINS}/platforms/qwindows.dll" "${COPY_DEST}/platforms"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_INSTALL_PLUGINS}/platforms/qwindowsd.dll"  "${COPY_DEST}/platforms"
                    COMMENT "Copying glvdebug Qt5 dependencies to ${COPY_DEST}"
                    VERBATIM)
endif()

build_options_finalize()
endif(NOT Qt5_FOUND)
