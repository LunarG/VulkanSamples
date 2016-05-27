cmake_minimum_required(VERSION 2.8.11)

# On Windows, we must pair Debug and Release appropriately
if (WIN32)
   # For Windows, since 32-bit and 64-bit items can co-exist, we build each in its own build directory.
   # 32-bit target data goes in build32, and 64-bit target data goes into build.  So, include/link the
   # appropriate data at build time.
   if (CMAKE_CL_64)
      set (BUILDTGT_DIR build)
   else ()
      set (BUILDTGT_DIR build32)
   endif()
else()
    include (FindPkgConfig)
    find_package(XCB REQUIRED)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")

    # If MSVC, disable some signed/unsigned mismatch warnings.
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267")
    endif()

else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare -std=c++11")
endif()

set (LIBGLM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs)

set(COMMON_CPP
    vkrenderframework.cpp
    vktestbinding.cpp
    vktestframework.cpp
    test_environment.cpp
   )

include_directories(
    "${PROJECT_SOURCE_DIR}/tests/gtest-1.7.0/include"
    "${PROJECT_SOURCE_DIR}/icd/common"
    "${PROJECT_SOURCE_DIR}/layers"
    ${GLSLANG_SPIRV_INCLUDE_DIR}
    ${LIBGLM_INCLUDE_DIR}
    )

if (NOT WIN32)
    # extra setup for out-of-tree builds
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        add_custom_target(binary-dir-symlinks ALL
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests.sh
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/vkvalidatelayerdoc.sh
            VERBATIM
            )
    endif()
else()
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/_run_all_tests.ps1 RUN_ALL)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/_vkvalidatelayerdoc.ps1 VALIDATE_DOC)
        add_custom_target(binary-dir-symlinks ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUN_ALL} run_all_tests.ps1
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VALIDATE_DOC} vkvalidatelayerdoc.ps1
            VERBATIM
            )
    endif()
endif()

if(WIN32)
   set (LIBVK "vulkan-${MAJOR}")
elseif(UNIX)
   set (LIBVK "vulkan")
else()
endif()

add_executable(vk_layer_validation_tests layer_validation_tests.cpp ${COMMON_CPP})
set_target_properties(vk_layer_validation_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
if(NOT WIN32)
   target_link_libraries(vk_layer_validation_tests ${LIBVK} ${XCB_LIBRARIES} ${X11_LIBRARIES} gtest gtest_main VkLayer_utils ${GLSLANG_LIBRARIES})
endif()
if(WIN32)
   target_link_libraries(vk_layer_validation_tests ${LIBVK} gtest gtest_main VkLayer_utils ${GLSLANG_LIBRARIES})
endif()

add_executable(vk_loader_validation_tests loader_validation_tests.cpp ${COMMON_CPP})
set_target_properties(vk_loader_validation_tests
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(vk_loader_validation_tests ${LIBVK} gtest gtest_main VkLayer_utils ${GLSLANG_LIBRARIES})

add_subdirectory(gtest-1.7.0)
