# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.
cmake_minimum_required(VERSION 3.4.1)

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
add_definitions(-DUSE_VULKAN_WRAPPER)

  message("Platform = ${ANDROID_PLATFORM}")

# Set path to header files.
  include_directories( ${ANDROID_NDK}/sources/android/native_app_glue )
  include_directories( ../..                                          )
  include_directories( ../../../WSIWindow                             )
  include_directories( ../../../WSIWindow/android                     )
  include_directories( ../../../WSIWindow/VulkanWrapper               )
  include_directories( ../../../../include                            ) # /vulkan/vulkan.h in VulkanSamples
  include_directories( ../../../../../x86_64/include                  ) # /vulkan/vulkan.h in SDK

# Set path to source files.
  aux_source_directory( ${ANDROID_NDK}/sources/android/native_app_glue SRC_LIST)
  aux_source_directory( ../..                                          SRC_LIST)
  aux_source_directory( ../../../WSIWindow                             SRC_LIST)
  aux_source_directory( ../../../WSIWindow/android                     SRC_LIST)
  aux_source_directory( ../../../WSIWindow/VulkanWrapper               SRC_LIST)

# Create native shared library
add_library(native SHARED ${SRC_LIST})

# Link to external libraries
target_link_libraries(native log android)
