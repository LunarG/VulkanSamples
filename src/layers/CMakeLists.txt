cmake_minimum_required(VERSION 2.8.11)

set(SHADER_DIR ${SAMPLES_DATA_DIR}/${VULKAN_VERSION})
set(SFILE overlay)
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

set(SDK_SOURCE "${CMAKE_SOURCE_DIR}/../LoaderAndTools")

if (EXISTS "${SDK_SOURCE}")

	if (WIN32)
		message(STATUS "Overlay layer example not enabled on Windows yet")
	else()
		find_library(LAYERUTILS_DLIB REQUIRED NAMES layer_utils
			HINTS "${SDK_SOURCE}/build/layers"
			)

		add_library(VKLayerOverlay SHARED ${VULKAN_VERSION}-overlay.cpp
			${SDK_SOURCE}/layers/vk_layer_table.cpp
			${SHADER_DIR}/overlay-vert.spv
			${SHADER_DIR}/overlay-frag.spv
			overlay.json)
		target_link_libraries(VKLayerOverlay ${LAYERUTILS_DLIB})
		set_target_properties(VKLayerOverlay PROPERTIES CXX_FLAGS "-Wno-unused-function")
		set_target_properties(VKLayerOverlay PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")

		add_custom_command(OUTPUT overlay.json
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/linux/overlay.json overlay.json
			VERBATIM)

		# ugly: reference unshipped bits of LoaderAndTools that we need for implementing
		# a layer from the outside.
		include_directories(
			"${SDK_SOURCE}/build/layers"
			"${SDK_SOURCE}/layers"
			"${SDK_SOURCE}/loader"
			"${EXTDIR}/stb"
			)
	endif()

else()
	message(STATUS "Colocated LoaderAndTools tree not found, not building Overlay example")
endif()

