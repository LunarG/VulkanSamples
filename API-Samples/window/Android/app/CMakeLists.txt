# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

# Set path to Vulkan SDK
  set(VULKAN_SDK "/home/rene/VulkanSDK/1.0.21.1/x86_64")

# Set path to header files.
  include_directories( ${ANDROID_NDK}/sources/android/native_app_glue )
  include_directories( ${VULKAN_SDK}/include                          )
  include_directories( vulkan_wrapper                                 )

# Set path to source files.
  aux_source_directory( ${ANDROID_NDK}/sources/android/native_app_glue SRC_LIST)
  aux_source_directory( vulkan_wrapper                                 SRC_LIST)
  aux_source_directory( src/main/cpp                                   SRC_LIST)
  aux_source_directory( ../..                                          SRC_LIST)

# Create native shared library
add_library( native             # Sets the name of the library.
             SHARED             # Sets the library as a SHARED library.
             ${SRC_LIST})       # List of source files

# Link to external libraries
target_link_libraries(native log android)
