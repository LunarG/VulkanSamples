cmake_minimum_required (VERSION 2.8.11)

add_custom_command(OUTPUT generic_layer.c xgl_enum_string_helper.h xgl_struct_string_helper.h xgl_struct_graphviz_helper.h xgl_struct_string_helper_no_addr.h api_dump.c api_dump_file.c api_dump_no_addr.c object_track.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py Generic > generic_layer.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-helper.py --gen_enum_string_helper ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-helper.py --gen_struct_wrappers ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-helper.py --gen_graphviz ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py ApiDump > api_dump.c
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py ApiDumpFile > api_dump_file.c
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py ApiDumpNoAddr > api_dump_no_addr.c
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py ObjectTracker > object_track.c
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl-helper.py
                           ${PROJECT_SOURCE_DIR}/xgl.py)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

# TODO : With standard settings below we get warnings as error due to unused functions.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I ../include")
set (CMAKE_C_FLAGS "-std=c99 -Wextra -Wno-sign-compare -Wno-unused-parameter -I ../include")
set (CMAKE_CXX_FLAGS "-std=c++11")

add_library (XGLLayerBasic SHARED basic.cpp)
add_library (XGLLayerGeneric SHARED generic_layer.c)
add_library (XGLLayerObjectTracker SHARED object_track.c object_track.h)
add_library (XGLLayerAPIDump SHARED api_dump.c xgl_enum_string_helper.h xgl_struct_string_helper.h)
add_library (XGLLayerAPIDumpFile SHARED api_dump_file.c xgl_enum_string_helper.h xgl_struct_string_helper.h)
add_library (XGLLayerAPIDumpNoAddr SHARED api_dump_no_addr.c xgl_enum_string_helper.h xgl_struct_string_helper_no_addr.h)
add_library (XGLLayerDrawState SHARED draw_state.c xgl_enum_string_helper.h xgl_struct_string_helper.h xgl_struct_string_helper.h)
add_library (XGLLayerMemTracker SHARED mem_tracker.c xgl_enum_string_helper.h xgl_struct_string_helper.h)
