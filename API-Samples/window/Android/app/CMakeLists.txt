# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.
cmake_minimum_required(VERSION 3.4.1)

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)

  message("Platform = ${ANDROID_PLATFORM}")

# Set path to header files.
  include_directories( ${ANDROID_NDK}/sources/android/native_app_glue )
  include_directories( src/cpp                                        )
  include_directories( ../..                                          )
  include_directories( ../../WSIWindow                                )

# Set path to source files.
  aux_source_directory( ${ANDROID_NDK}/sources/android/native_app_glue SRC_LIST)
  aux_source_directory( src/cpp                                        SRC_LIST)
  aux_source_directory( ../..                                          SRC_LIST)
  aux_source_directory( ../../WSIWindow                                SRC_LIST)

# Create native shared library
add_library(native SHARED ${SRC_LIST})

# Link to external libraries
target_link_libraries(native log android)
