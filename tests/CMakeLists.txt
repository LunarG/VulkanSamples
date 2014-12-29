include (FindPkgConfig)

find_package (OpenGL REQUIRED)
find_package (GLUT REQUIRED)
find_package(ImageMagick COMPONENTS MagickWand)

if(NOT EXISTS /usr/include/glm/glm.hpp)
    message(FATAL_ERROR "Necessary libglm-dev headers cannot be found: sudo apt-get install libglm-dev")
endif()

if(NOT ImageMagick_FOUND)
    message(FATAL_ERROR "Missing ImageMagick library: sudo apt-get install libmagickwand-dev")
endif()

SET(COMMON_CPP
    xglrenderframework.cpp
    xgltestbinding.cpp
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
   ${ImageMagick_LIBRARIES}
   )

include_directories(
    "${PROJECT_SOURCE_DIR}/tests/gtest-1.7.0/include"
    "${PROJECT_SOURCE_DIR}/icd/common"
    "${GLSLANG_PREFIX}/glslang/Include"
    "${GLSLANG_PREFIX}/glslang/Public"
    "${GLSLANG_PREFIX}/BIL"
    ${ImageMagick_INCLUDE_DIRS}
    )

add_definitions(
    -DMAGICKCORE_QUANTUM_DEPTH=16
    -DMAGICKCORE_HDRI_ENABLE=0
    )

# extra setup for out-of-tree builds
if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
    add_custom_target(binary-dir-symlinks ALL
        COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/golden
        COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests.sh
        VERBATIM
        )
endif()

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

add_executable(xgl_render_tests render_tests.cpp ${COMMON_CPP})
set_target_properties(xgl_render_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_render_tests XGL gtest gtest_main ${TEST_LIBRARIES})

add_executable(xgl_blit_tests blit_tests.cpp ${COMMON_CPP})
set_target_properties(xgl_blit_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xgl_blit_tests XGL gtest gtest_main ${TEST_LIBRARIES})

add_subdirectory(gtest-1.7.0)
