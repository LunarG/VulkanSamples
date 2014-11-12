cmake_minimum_required (VERSION 2.8.11)

# NOTE : Initially generating helper files to both source and BIN dir to support checked-in and generated layers, need a better way to do this.
add_custom_command(OUTPUT xgl_enum_string_helper.h xgl_struct_string_helper.h api_dump.c api_dump_file.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py generic-layer > generic_layer.c
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-helper.py --gen_enum_string_helper ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${CMAKE_BINARY_DIR}/layers
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-helper.py --gen_struct_wrappers ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${CMAKE_BINARY_DIR}/layers
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-helper.py --gen_enum_string_helper ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${PROJECT_SOURCE_DIR}/layers
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-helper.py --gen_struct_wrappers ${PROJECT_SOURCE_DIR}/include/xgl.h --abs_out_dir ${PROJECT_SOURCE_DIR}/layers
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py api-dump > api_dump.c
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py api-dump-file > api_dump_file.c
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-layer-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl-helper.py
                           ${PROJECT_SOURCE_DIR}/xgl.py)

# TODO : With standard settings below we get warnings as error due to unused functions.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I ../include")
set (CMAKE_C_FLAGS "-std=c99 -Wextra -Wno-sign-compare -Wno-unused-parameter -I ../include")

add_library (XGLLayerBasic SHARED basic_plugin.c)
add_library (XGLLayerGeneric SHARED generic_layer.c)
add_library (XGLLayerObjectTracker SHARED object_track.c)
add_library (XGLLayerAPIDump SHARED api_dump.c xgl_enum_string_helper.h xgl_struct_string_helper.h)
add_library (XGLLayerAPIDumpFile SHARED api_dump_file.c xgl_enum_string_helper.h xgl_struct_string_helper.h)
add_library (XGLLayerDrawState SHARED draw_state.c)
add_library (XGLLayerMemTracker SHARED mem_tracker.c)
