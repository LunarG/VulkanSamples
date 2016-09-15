project(window)
cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)

#set(VULKAN_SDK $ENV{VULKAN_SDK})
#include_directories("${VULKAN_SDK}/include")

include_directories("${PROJECT_SOURCE_DIR}")

aux_source_directory(. SRC_LIST)
#aux_source_directory(WSIWindow SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

#-----------------WSIWindow-----------------
add_subdirectory(WSIWindow)
target_link_libraries(${PROJECT_NAME} WSIWindow)
#-------------------------------------------


