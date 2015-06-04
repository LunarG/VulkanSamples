cmake_minimum_required(VERSION 2.8.11)
include (FindPkgConfig)

if(NOT WIN32)
    find_package(XCB REQUIRED)
endif()

find_package(ImageMagick COMPONENTS MagickWand)
if(NOT ImageMagick_FOUND)
    if(NOT WIN32)
        message(FATAL_ERROR "Missing ImageMagick library: sudo apt-get install libmagickwand-dev")
    else()
        message(FATAL_ERROR "Missing ImageMagick library: Get from http://www.imagemagick.org/script/binary-releases.php.  Be sure to check box to 'Install development headers and libraries'")
    endif()
endif()

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES /wd4267")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -Wno-sign-compare -std=c++11")
endif()

set (LIBGLM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs)

set(COMMON_CPP
    vkrenderframework.cpp
    vktestbinding.cpp
    vktestframework.cpp
    test_environment.cpp
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
    glslang
    OGLCompiler
    OSDependent
    SPIRV
    ${XCB_LIBRARIES}
    ${ImageMagick_LIBRARIES}
    )

include_directories(
    "${PROJECT_SOURCE_DIR}/tests/gtest-1.7.0/include"
    "${PROJECT_SOURCE_DIR}/icd/common"
    "${PROJECT_SOURCE_DIR}/layers"
    ${GLSLANG_PREFIX}
    ${XCB_INCLUDE_DIRS}
    ${LIBGLM_INCLUDE_DIR}
    ${ImageMagick_INCLUDE_DIRS}
    )

if (NOT WIN32)
    # extra setup for out-of-tree builds
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        add_custom_target(binary-dir-symlinks ALL
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/golden
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests.sh
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests_with_layers.sh
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/layer_test_suite.py
            VERBATIM
            )
    endif()
else()
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/golden GOLDEN)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests.ps1 RUN_ALL)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests_with_layers.ps1 RUN_ALL_WITH_LAYERS)
        add_custom_target(binary-dir-symlinks ALL
            COMMAND copy ${RUN_ALL} run_all_tests.ps1
            COMMAND copy ${RUN_ALL_WITH_LAYERS} run_all_tests_with_layers.ps1
            COMMAND mklink /J golden ${GOLDEN}
            VERBATIM
            )
    endif()
endif()

add_executable(vkbase init.cpp ${COMMON_CPP})
set_target_properties(vkbase
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vkbase vulkan gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_image_tests image_tests.cpp ${COMMON_CPP})
set_target_properties(vk_image_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_image_tests vulkan gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_render_tests render_tests.cpp ${COMMON_CPP})
set_target_properties(vk_render_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_render_tests vulkan gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_blit_tests blit_tests.cpp ${COMMON_CPP})
set_target_properties(vk_blit_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_blit_tests vulkan gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_layer_validation_tests layer_validation_tests.cpp ${COMMON_CPP})
set_target_properties(vk_layer_validation_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_layer_validation_tests vulkan gtest gtest_main layer_utils ${TEST_LIBRARIES})

add_subdirectory(gtest-1.7.0)
