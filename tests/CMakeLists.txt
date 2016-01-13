cmake_minimum_required(VERSION 2.8.11)

# On Windows, we must pair Debug and Release appropriately
if (WIN32)
   # For Windows, since 32-bit and 64-bit items can co-exist, we build each in its own build directory.
   # 32-bit target data goes in build32, and 64-bit target data goes into build64.  So, include/link the
   # appropriate data at build time.
   if (CMAKE_CL_64)
      set (BUILDTGT_DIR build64)
   else ()
      set (BUILDTGT_DIR build32)
   endif()
else()
    include (FindPkgConfig)
    find_package(XCB REQUIRED)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(ImageMagick COMPONENTS MagickWand)
if(NOT ImageMagick_FOUND)
    if(NOT WIN32)
        message(FATAL_ERROR "Missing ImageMagick library: sudo apt-get install libmagickwand-dev")
    else()
        message(FATAL_ERROR "Missing ImageMagick library: Get from http://www.imagemagick.org/script/binary-releases.php.  Be sure to check box to 'Install development headers and libraries'")
    endif()
endif()

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES /wd4267 -DMAGICKCORE_HDRI_ENABLE=0 -DMAGICKCORE_QUANTUM_DEPTH=16")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare -std=c++11 -DMAGICKCORE_HDRI_ENABLE=0 -DMAGICKCORE_QUANTUM_DEPTH=16")
endif()

set (LIBGLM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs)

set(COMMON_CPP
    vkrenderframework.cpp
    vktestbinding.cpp
    vktestframework.cpp
    test_environment.cpp
   )

set(TEST_LIBRARIES
    glslang
    OGLCompiler
    OSDependent
    SPIRV
    ${XCB_LIBRARIES}
    ${ImageMagick_LIBRARIES}
    )

add_library(glslang     STATIC IMPORTED)
add_library(OGLCompiler STATIC IMPORTED)
add_library(OSDependent STATIC IMPORTED)
add_library(SPIRV       STATIC IMPORTED)

# On Windows, we must pair Debug and Release appropriately
if (WIN32)

   set_target_properties(glslang PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/glslang/Release/glslang.lib"
                         IMPORTED_LOCATION_DEBUG "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/glslang/Debug/glslang.lib")
   set_target_properties(OGLCompiler PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/OGLCompilersDLL/Release/OGLCompiler.lib"
                         IMPORTED_LOCATION_DEBUG "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/OGLCompilersDLL/Debug/OGLCompiler.lib")
   set_target_properties(OSDependent PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/glslang/OSDependent/Windows/Release/OSDependent.lib"
                         IMPORTED_LOCATION_DEBUG "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/glslang/OSDependent/Windows/Debug/OSDependent.lib")
   set_target_properties(SPIRV PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/SPIRV/Release/SPIRV.lib"
                         IMPORTED_LOCATION_DEBUG "${GLSLANG_PREFIX}/${BUILDTGT_DIR}/SPIRV/Debug/SPIRV.lib")
else ()
   set_target_properties(glslang PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/build/install/lib/libglslang.a")
   set_target_properties(OGLCompiler PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/build/install/lib/libOGLCompiler.a")
   set_target_properties(OSDependent PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/build/install/lib/libOSDependent.a")
   set_target_properties(SPIRV PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_PREFIX}/build/install/lib/libSPIRV.a")
endif()

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
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/vktracereplay.sh
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/vkvalidatelayerdoc.sh
            VERBATIM
            )
    endif()
else()
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/golden GOLDEN)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/_run_all_tests.ps1 RUN_ALL)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/_run_all_tests_with_layers.ps1 RUN_ALL_WITH_LAYERS)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/_vktracereplay.ps1 VKTRACEREPLAY)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/_vkvalidatelayerdoc.ps1 VKVALIDATELAYERDOC)
        add_custom_target(binary-dir-symlinks ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUN_ALL} run_all_tests.ps1
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUN_ALL_WITH_LAYERS} run_all_tests_with_layers.ps1
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VKTRACEREPLAY} vktracereplay.ps1
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VKVALIDATELAYERDOC} vkvalidatelayerdoc.ps1
            COMMAND IF NOT EXIST golden mklink /J golden ${GOLDEN}
            VERBATIM
            )
    endif()
endif()

set (LIBVK "vulkan-${MAJOR}")

add_executable(vkbase init.cpp ${COMMON_CPP})
set_target_properties(vkbase
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vkbase ${LIBVK} gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_image_tests image_tests.cpp ${COMMON_CPP})
set_target_properties(vk_image_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_image_tests ${LIBVK} gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_render_tests render_tests.cpp ${COMMON_CPP})
set_target_properties(vk_render_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_render_tests ${LIBVK} gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_blit_tests blit_tests.cpp ${COMMON_CPP})
set_target_properties(vk_blit_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_blit_tests ${LIBVK} gtest gtest_main ${TEST_LIBRARIES})

add_executable(vk_layer_validation_tests layer_validation_tests.cpp ${COMMON_CPP})
set_target_properties(vk_layer_validation_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_layer_validation_tests ${LIBVK} gtest gtest_main layer_utils ${TEST_LIBRARIES})

add_subdirectory(gtest-1.7.0)
