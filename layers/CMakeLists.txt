cmake_minimum_required (VERSION 2.8.11)

macro(run_xgl_helper subcmd)
	add_custom_command(OUTPUT ${ARGN}
		COMMAND ${PROJECT_SOURCE_DIR}/xgl_helper.py --${subcmd} ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${PROJECT_SOURCE_DIR}/xgl_helper.py ${PROJECT_SOURCE_DIR}/include/xgl.h
	)
endmacro()

macro(run_xgl_layer_generate subcmd output)
	add_custom_command(OUTPUT ${output}
		COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py ${subcmd} ${PROJECT_SOURCE_DIR}/include/xgl.h > ${output}
		DEPENDS ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py ${PROJECT_SOURCE_DIR}/include/xgl.h ${PROJECT_SOURCE_DIR}/xgl.py
	)
endmacro()

macro(add_xgl_layer target)
	add_library(XGLLayer${target} SHARED ${ARGN})
	add_dependencies(XGLLayer${target} generate_xgl_layer_helpers)
endmacro()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

set (CMAKE_CXX_FLAGS "-std=c++11")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXGL_PROTOTYPES")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES")

add_custom_command(OUTPUT xgl_dispatch_table_helper.h
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py dispatch-table-ops layer > xgl_dispatch_table_helper.h
	DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

add_custom_command(OUTPUT xgl_generic_intercept_proc_helper.h
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py layer-intercept-proc > xgl_generic_intercept_proc_helper.h
	DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

run_xgl_helper(gen_enum_string_helper xgl_enum_string_helper.h)
run_xgl_helper(gen_struct_wrappers
	xgl_struct_string_helper.h
        xgl_struct_string_helper_cpp.h
	xgl_struct_string_helper_no_addr.h
	xgl_struct_string_helper_no_addr_cpp.h
	xgl_struct_wrappers.h
	xgl_struct_wrappers.cpp
)
run_xgl_helper(gen_graphviz xgl_struct_graphviz_helper.h)

add_custom_target(generate_xgl_layer_helpers DEPENDS
	xgl_dispatch_table_helper.h
	xgl_generic_intercept_proc_helper.h
	xgl_enum_string_helper.h
	xgl_struct_string_helper.h
	xgl_struct_string_helper_no_addr.h
        xgl_struct_string_helper_cpp.h
	xgl_struct_string_helper_no_addr_cpp.h
	xgl_struct_wrappers.h
	xgl_struct_wrappers.cpp
	xgl_struct_graphviz_helper.h
)

run_xgl_layer_generate(Generic generic_layer.c)
run_xgl_layer_generate(ApiDump api_dump.c)
run_xgl_layer_generate(ApiDumpFile api_dump_file.c)
run_xgl_layer_generate(ApiDumpNoAddr api_dump_no_addr.c)
run_xgl_layer_generate(ApiDumpCpp api_dump.cpp)
run_xgl_layer_generate(ApiDumpNoAddrCpp api_dump_no_addr.cpp)
run_xgl_layer_generate(ObjectTracker object_track.c)
run_xgl_layer_generate(ParamChecker param_checker.c)

add_xgl_layer(Basic basic.cpp)
add_xgl_layer(Multi multi.cpp)
add_xgl_layer(DrawState draw_state.c layers_config.cpp)
add_xgl_layer(MemTracker mem_tracker.c layers_config.cpp)
# generated
add_xgl_layer(Generic generic_layer.c)
add_xgl_layer(APIDump api_dump.c)
add_xgl_layer(APIDumpFile api_dump_file.c)
add_xgl_layer(APIDumpNoAddr api_dump_no_addr.c)
add_xgl_layer(APIDumpCpp api_dump.cpp)
add_xgl_layer(APIDumpNoAddrCpp api_dump_no_addr.cpp)
add_xgl_layer(ObjectTracker object_track.c)
add_xgl_layer(ParamChecker param_checker.c)
