project(WSIWindow_Test)
cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)

include_directories("${PROJECT_SOURCE_DIR}")
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

#-----------------WSIWindow-----------------
add_subdirectory(WSIWindow)
target_link_libraries(${PROJECT_NAME} WSIWindow)
#-------------------------------------------


