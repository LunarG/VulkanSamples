include (FindPkgConfig)

find_package (OpenGL REQUIRED)
find_package (GLUT REQUIRED)

SET(COMMON
    common.c
    )

SET(COMMON_CPP
    xglgpu.cpp
    xgldevice.cpp
    xglimage.cpp
   )

include_directories(
    "${PROJECT_SOURCE_DIR}/tests/gtest-1.7.0/include"
    "${PROJECT_SOURCE_DIR}/icd/common"
    )


add_executable(xglinfo xglinfo.c ${COMMON})
add_executable(xglbase init.cpp ${COMMON_CPP})
set_target_properties(xglbase
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xglbase XGL gtest gtest_main)

add_executable(xgl_image_tests image_tests.cpp ${COMMON_CPP})
set_target_properties(xgl_image_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_image_tests XGL gtest gtest_main)

add_executable(xgl_render_tests render_tests.cpp ${COMMON_CPP} displayengine.cpp)
set_target_properties(xgl_render_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_render_tests XGL gtest gtest_main ${OPENGL_gl_LIBRARY} ${GLUT_glut_LIBRARY})

target_link_libraries(xglinfo XGL)

add_subdirectory(gtest-1.7.0)
