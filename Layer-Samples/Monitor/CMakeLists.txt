cmake_minimum_required(VERSION 2.8.11)

if(NOT WIN32)
    if (BUILD_WSI_XCB_SUPPORT)
        find_package(XCB REQUIRED)
        include_directories(${XCB_INCLUDE_DIRS})
        link_libraries(${XCB_LIBRARIES})
    else()
        message( WARNING "Monitor layer: Only XCB and Win32 supported at this time")
    endif()
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/layers")
    set(LAYERS_LOC ${CMAKE_SOURCE_DIR})
    set(LAYER_UTILS_LIB VkLayer_utils)
else()
    set(LAYERS_LOC ${CMAKE_SOURCE_DIR}/../source)
    find_library(LAYER_UTILS_LIB NAMES VkLayer_utils
        HINTS "${CMAKE_SOURCE_DIR}/../bin" "${CMAKE_SOURCE_DIR}/../x86_64/lib" )
endif()

set(UTILDIR "${CMAKE_SOURCE_DIR}/Layer-Samples/utils")

add_library(VKLayer_monitor SHARED monitor.cpp
        ${LAYERS_LOC}/layers/vk_layer_table.cpp
        monitor.json)

if (WIN32)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")

    set_target_properties(VKLayer_monitor PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_monitor.def")
    add_custom_command(OUTPUT monitor.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/windows/monitor.json $<CONFIGURATION>/monitor.json
        VERBATIM)
else()
    set_target_properties(VKLayer_monitor PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")
    add_custom_command(OUTPUT monitor.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/linux/monitor.json monitor.json
        VERBATIM)
endif()

target_link_libraries(VKLayer_monitor ${LAYER_UTILS_LIB})
set_target_properties(VKLayer_monitor PROPERTIES CXX_FLAGS "-Wno-unused-function")

if(SDK_INCLUDE_PATH)
    include_directories(
            "${CMAKE_BINARY_DIR}/layers"
            "${LAYERS_LOC}/layers"
            "${LAYERS_LOC}/loader"
            "${UTILDIR}/stb"
            "${SDK_INCLUDE_PATH}"
            )
else()
    include_directories(
            "${CMAKE_BINARY_DIR}/layers"
            "${LAYERS_LOC}/layers"
            "${LAYERS_LOC}/loader"
            "${UTILDIR}/stb"
            )
endif()
