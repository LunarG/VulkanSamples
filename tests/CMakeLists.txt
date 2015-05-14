include (FindPkgConfig)

if(NOT WIN32)
    find_package(XCB REQUIRED)
endif()

find_package(ImageMagick COMPONENTS MagickWand)

if(NOT EXISTS /usr/include/glm/glm.hpp)
    message(FATAL_ERROR "Necessary libglm-dev headers cannot be found: sudo apt-get install libglm-dev")
endif()

if(NOT ImageMagick_FOUND)
    message(FATAL_ERROR "Missing ImageMagick library: sudo apt-get install libmagickwand-dev")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -Wno-sign-compare")

SET(COMMON_CPP
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
    "${GLSLANG_PREFIX}/glslang/Include"
    "${GLSLANG_PREFIX}/glslang/Public"
    "${GLSLANG_PREFIX}/SPIRV"
    ${XCB_INCLUDE_DIRS}
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
        COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests_with_layers.sh
        COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/layer_test_suite.py
        VERBATIM
        )
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
