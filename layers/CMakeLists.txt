cmake_minimum_required (VERSION 2.8.11)

macro(run_vk_helper subcmd)
	add_custom_command(OUTPUT ${ARGN}
		COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk_helper.py --${subcmd} ${PROJECT_SOURCE_DIR}/include/vulkan.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${PROJECT_SOURCE_DIR}/vk_helper.py ${PROJECT_SOURCE_DIR}/include/vulkan.h
	)
endmacro()

macro(run_vk_layer_generate subcmd output)
	add_custom_command(OUTPUT ${output}
		COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-layer-generate.py ${subcmd} ${PROJECT_SOURCE_DIR}/include/vulkan.h > ${output}
		DEPENDS ${PROJECT_SOURCE_DIR}/vk-layer-generate.py ${PROJECT_SOURCE_DIR}/include/vulkan.h ${PROJECT_SOURCE_DIR}/vulkan.py
	)
endmacro()

if (WIN32)
    macro(add_vk_layer target)
	add_custom_command(OUTPUT VKLayer${target}.def
		COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py win-def-file VKLayer${target} layer > VKLayer${target}.def
		DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vk.py
	)
	add_library(VKLayer${target} SHARED ${ARGN} VKLayer${target}.def)
        target_link_Libraries(VKLayer${target} layer_utils)
	add_dependencies(VKLayer${target} layer_utils_static)
	add_dependencies(VKLayer${target} generate_vk_layer_helpers)
	add_dependencies(VKLayer${target} ${CMAKE_CURRENT_BINARY_DIR}/VKLayer${target}.def)
	set_target_properties(VKLayer${target} PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_BINARY_DIR}/VKLayer${target}.def")
    endmacro()
else()
    macro(add_vk_layer target)
	add_library(VKLayer${target} SHARED ${ARGN})
        target_link_Libraries(VKLayer${target} layer_utils)
	add_dependencies(VKLayer${target} generate_vk_layer_helpers)
	set_target_properties(VKLayer${target} PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")
    endmacro()
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../loader
	${CMAKE_CURRENT_BINARY_DIR}
)

if (WIN32)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS")
endif()
if (NOT WIN32)
    set (CMAKE_CXX_FLAGS "-std=c++11")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -Wpointer-arith")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -Wpointer-arith")
endif()

add_custom_command(OUTPUT vk_dispatch_table_helper.h
	COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py dispatch-table-ops layer > vk_dispatch_table_helper.h
	DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)

run_vk_helper(gen_enum_string_helper vk_enum_string_helper.h)
run_vk_helper(gen_struct_wrappers
	vk_struct_string_helper.h
        vk_struct_string_helper_cpp.h
	vk_struct_string_helper_no_addr.h
	vk_struct_string_helper_no_addr_cpp.h
        vk_struct_size_helper.h
        vk_struct_size_helper.c
	vk_struct_wrappers.h
	vk_struct_wrappers.cpp
)
run_vk_helper(gen_graphviz vk_struct_graphviz_helper.h)

add_custom_target(generate_vk_layer_helpers DEPENDS
	vk_dispatch_table_helper.h
	vk_enum_string_helper.h
	vk_struct_string_helper.h
	vk_struct_string_helper_no_addr.h
        vk_struct_string_helper_cpp.h
	vk_struct_string_helper_no_addr_cpp.h
        vk_struct_size_helper.h
        vk_struct_size_helper.c
	vk_struct_wrappers.h
	vk_struct_wrappers.cpp
	vk_struct_graphviz_helper.h
)

run_vk_layer_generate(Generic generic_layer.cpp)
run_vk_layer_generate(APIDump api_dump.cpp)
run_vk_layer_generate(ObjectTracker object_track.cpp)
run_vk_layer_generate(Threading threading.cpp)

add_library(layer_utils SHARED vk_layer_config.cpp)
if (WIN32)
    add_library(layer_utils_static STATIC vk_layer_config.cpp)
    set_target_properties(layer_utils_static PROPERTIES OUTPUT_NAME layer_utils)
    target_link_libraries(layer_utils)
endif()

add_vk_layer(Basic basic.cpp vk_layer_table.cpp)
add_vk_layer(Multi multi.cpp)
add_vk_layer(DrawState draw_state.cpp vk_layer_debug_marker_table.cpp vk_layer_table.cpp)
add_vk_layer(MemTracker mem_tracker.cpp vk_layer_table.cpp)
add_vk_layer(ShaderChecker shader_checker.cpp vk_layer_table.cpp)
add_vk_layer(Image image.cpp vk_layer_table.cpp)
# generated
add_vk_layer(Generic generic_layer.cpp vk_layer_table.cpp)
add_vk_layer(APIDump api_dump.cpp vk_layer_table.cpp)
add_vk_layer(ObjectTracker object_track.cpp vk_layer_table.cpp)
add_vk_layer(ParamChecker param_checker.cpp vk_layer_debug_marker_table.cpp vk_layer_table.cpp)
add_vk_layer(Threading threading.cpp vk_layer_table.cpp)
add_vk_layer(ScreenShot screenshot.cpp vk_layer_table.cpp)
