cmake_minimum_required(VERSION 3.4.1)
include(ExternalProject)

# build layer_utils as a static lib
set(NDK_SRC_DIR ${ANDROID_NDK}/sources)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror \
        -Wno-unused-function -Wno-unused-const-variable \
        -Wno-c++98-compat-pedantic -DVK_PROTOTYPES \
        -DVK_USE_PLATFORM_ANDROID_KHR")

get_filename_component(SRC_DIR "${CMAKE_SOURCE_DIR}/../../.."  ABSOLUTE)
set(EXTERNAL_DIR "${SRC_DIR}/build-android/external")
set(COMMON_DIR "${SRC_DIR}//build-android/generated")

set(CMAKE_CXX_clone "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR \
        -fvisibility=hidden")
include_directories(${SRC_DIR}/include
                    ${COMMON_DIR}/include
                    ${SRC_DIR}/layers
                    ${SRC_DIR}/loader)
add_library(layer_utils STATIC
        ${COMMON_DIR}/common/vk_layer_config.cpp
        ${COMMON_DIR}/common/vk_layer_extension_utils.cpp
        ${COMMON_DIR}/common/vk_layer_utils.cpp
        ${COMMON_DIR}/common/vk_format_utils.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")

# TODO: adding back to build shaderc ourselves
add_library(SPIRV-Tools-prebuilt STATIC IMPORTED)
set_target_properties(SPIRV-Tools-prebuilt PROPERTIES IMPORTED_LOCATION
        ${EXTERNAL_DIR}/shaderc/android_test/obj/local/${ANDROID_ABI}/libSPIRV-Tools.a)

# build core_validation layers which including shader validation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR \
                     -fvisibility=hidden")
# the following is not working for clang
#                    -Wl,-Bsymbolic \
#                    -Wl,--exclude-libs,All")
add_library(VkLayer_core_validation SHARED
        ${SRC_DIR}/layers/core_validation.cpp
        ${COMMON_DIR}/common/descriptor_sets.cpp
        ${COMMON_DIR}/common/buffer_validation.cpp
        ${COMMON_DIR}/common/vk_layer_table.cpp)
target_include_directories(VkLayer_core_validation PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader
        ${EXTERNAL_DIR}/glslang
        ${EXTERNAL_DIR}/spirv-tools/include)
target_link_libraries(VkLayer_core_validation PRIVATE
        log layer_utils SPIRV-Tools-prebuilt)

add_library(VkLayer_parameter_validation SHARED
        ${SRC_DIR}/layers/parameter_validation.cpp
        ${COMMON_DIR}/common/vk_layer_table.cpp)
target_include_directories(VkLayer_parameter_validation PRIVATE
        ${SRC_DIR}/include
        ${COMMON_DIR}/include
        ${SRC_DIR}/layers
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_parameter_validation PRIVATE log layer_utils)

add_library(VkLayer_object_tracker SHARED
        ${SRC_DIR}/layers/object_tracker.cpp
        ${COMMON_DIR}/common/vk_layer_table.cpp)
target_include_directories(VkLayer_object_tracker PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_object_tracker PRIVATE log layer_utils)

add_library(VkLayer_threading SHARED
        ${SRC_DIR}/layers/threading.cpp
        ${COMMON_DIR}/common/vk_layer_table.cpp)
target_include_directories(VkLayer_threading PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_threading PRIVATE log layer_utils)

add_library(VkLayer_unique_objects SHARED
        ${SRC_DIR}/layers/unique_objects.cpp
        ${COMMON_DIR}/common/vk_layer_table.cpp)
target_include_directories(VkLayer_unique_objects PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_unique_objects PRIVATE log layer_utils)

add_library(VkLayer_swapchain SHARED
       ${SRC_DIR}/layers/swapchain.cpp
       ${COMMON_DIR}/common/vk_layer_table.cpp)
target_include_directories(VkLayer_swapchain PRIVATE
       ${SRC_DIR}/include
       ${SRC_DIR}/layers
       ${COMMON_DIR}/include
       ${SRC_DIR}/loader)
target_link_libraries(VkLayer_swapchain PRIVATE log layer_utils)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")
