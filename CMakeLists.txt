# The name of our project is "XGL". CMakeLists files in this project can 
# refer to the root source directory of the project as ${XGL_SOURCE_DIR} and 
# to the root binary directory of the project as ${XGL_BINARY_DIR}. 
cmake_minimum_required (VERSION 2.6) 
project (XGL) 
set (XGL_VERSION_MAJOR 0)
set (XGL_VERSION_MINOR 1)

# Header file for CMake settings
configure_file (
   "${PROJECT_SOURCE_DIR}/include/XGLConfig.h.in"
   "${PROJECT_BINARY_DIR}/XGLConfig.h"
   )

include_directories("${PROJECT_BINARY_DIR}")

# xgl global includes
include_directories("${PROJECT_SOURCE_DIR}/include")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall")

# loader: Generic XGL ICD loader
# main: Device independent (DI) XGL library
# drivers: Device dependent (DD) XGL components
# tests: XGL tests
add_subdirectory (loader)
add_subdirectory (drivers)
add_subdirectory (tests)
