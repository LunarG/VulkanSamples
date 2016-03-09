cmake_minimum_required(VERSION 2.8.11)

set(SHADER_DIR "${PROJECT_SOURCE_DIR}/Layer-Samples/data")
set(SFILE overlay)

if(WIN32)
    set (MOVE_CMD "move")
else()
    set (MOVE_CMD "mv")
endif()

add_custom_command(OUTPUT ${SHADER_DIR}/${SFILE}-vert.spv
	COMMAND ${GLSLANG_VALIDATOR} -s -V ${SHADER_DIR}/${SFILE}.vert
	COMMAND ${MOVE_CMD} vert.spv ${SHADER_DIR}/${SFILE}-vert.spv
	DEPENDS ${SHADER_DIR}/${SFILE}.vert ${GLSLANG_VALIDATOR}
)
add_custom_command(OUTPUT ${SHADER_DIR}/${SFILE}-frag.spv
	COMMAND ${GLSLANG_VALIDATOR} -s -V ${SHADER_DIR}/${SFILE}.frag
	COMMAND ${MOVE_CMD} frag.spv ${SHADER_DIR}/${SFILE}-frag.spv
	DEPENDS ${SHADER_DIR}/${SFILE}.frag ${GLSLANG_VALIDATOR}
)

message(STATUS "Shader dir: ${SHADER_DIR}")
message(STATUS "Cxxflags: ${CMAKE_CXX_FLAGS}")

set(UTILDIR "${CMAKE_SOURCE_DIR}/Layer-Samples/utils")

add_library(VKLayer_overlay SHARED overlay.cpp
        ${CMAKE_SOURCE_DIR}/layers/vk_layer_table.cpp
        ${SHADER_DIR}/overlay-vert.spv
        ${SHADER_DIR}/overlay-frag.spv
        overlay.json)

if (WIN32)
    set_target_properties(VKLayer_overlay PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_overlay.def")
    add_custom_command(OUTPUT overlay.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/windows/overlay.json $<CONFIGURATION>/overlay.json
        VERBATIM)
else()
    set_target_properties(VKLayer_overlay PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")
    add_custom_command(OUTPUT overlay.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/linux/overlay.json overlay.json
        VERBATIM)
endif()

target_link_libraries(VKLayer_overlay layer_utils)
set_target_properties(VKLayer_overlay PROPERTIES CXX_FLAGS "-Wno-unused-function")

include_directories(
        "${CMAKE_BINARY_DIR}/layers"
        "${CMAKE_SOURCE_DIR}/layers"
        "${CMAKE_SOURCE_DIR}/loader"
        "${UTILDIR}/stb"
        )

