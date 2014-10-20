include (FindPkgConfig)

find_package (OpenGL REQUIRED)
find_package (GLUT REQUIRED)
if(NOT EXISTS /usr/include/glm/glm.hpp)
    message(FATAL_ERROR "Necessary libglm-dev headers cannot be found: sudo apt-get install libglm-dev")
endif()

SET(COMMON
    common.c
    )

SET(COMMON_CPP
    xglgpu.cpp
    xgldevice.cpp
    xglimage.cpp
    xglrenderframework.cpp
    xgltestframework.cpp
   )

# Expect libraries to be in either the build (release build) or dbuild (debug) directories
if(EXISTS ${GLSLANG_PREFIX}/build/install/lib)
    set(GLSLANG_BUILD ${GLSLANG_PREFIX}/build)
elseif(EXISTS ${GLSLANG_PREFIX}/dbuild/install/lib)
    set(GLSLANG_BUILD ${GLSLANG_PREFIX}/dbuild)
else()
    message(FATAL_ERROR "Necessary glslang libraries cannot be found: " ${GLSLANG_PREFIX})
endif()

link_directories(
   "${GLSLANG_BUILD}/install/lib"
   )


set(TEST_LIBRARIES
   ${OPENGL_gl_LIBRARY}
   ${GLUT_glut_LIBRARY}
   glslang
   OGLCompiler
   OSDependent
   BIL
   )

include_directories(
    "${PROJECT_SOURCE_DIR}/tests/gtest-1.7.0/include"
    "${PROJECT_SOURCE_DIR}/icd/common"
    "${GLSLANG_PREFIX}/glslang/Include"
    "${GLSLANG_PREFIX}/glslang/Public"
    "${GLSLANG_PREFIX}/BIL"
    )


add_executable(xglinfo xglinfo.c ${COMMON})
add_executable(xglbase init.cpp ${COMMON_CPP})
set_target_properties(xglbase
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xglbase XGL gtest gtest_main ${TEST_LIBRARIES})

add_executable(xgl_image_tests image_tests.cpp ${COMMON_CPP})
set_target_properties(xgl_image_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_image_tests XGL gtest gtest_main ${TEST_LIBRARIES})

add_executable(xgl_render_tests render_tests.cpp ${COMMON_CPP} displayengine.cpp)
set_target_properties(xgl_render_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_render_tests XGL gtest gtest_main ${TEST_LIBRARIES})

add_executable(xgl_compiler_render_tests compiler_render_tests.cpp ${COMMON_CPP} displayengine.cpp)
set_target_properties(xgl_compiler_render_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_compiler_render_tests XGL gtest gtest_main ${TEST_LIBRARIES})

add_executable(xgl_base_render_test base_render_test.cpp ${COMMON_CPP} displayengine.cpp)
set_target_properties(xgl_base_render_test
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_base_render_test XGL gtest gtest_main ${TEST_LIBRARIES})

target_link_libraries(xglinfo XGL)

add_subdirectory(gtest-1.7.0)
