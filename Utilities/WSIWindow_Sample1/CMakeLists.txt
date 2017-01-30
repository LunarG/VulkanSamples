project(WSIWindow_Sample1)
cmake_minimum_required(VERSION 2.8)

if(WIN32)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-std=c++11)
endif()

include_directories("${PROJECT_SOURCE_DIR}")
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

#---------------Find WSIWindow---------------
#add_subdirectory(WSIWindow)

if(NOT TARGET WSIWindow)
    add_subdirectory(../WSIWindow ${CMAKE_BINARY_DIR}/WSIWindow)
endif()

target_link_libraries(${PROJECT_NAME} WSIWindow)
#-------------------------------------------
