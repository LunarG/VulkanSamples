cmake_minimum_required (VERSION 2.8.11)

add_custom_command(OUTPUT generic_layer.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py generic-layer > generic_layer.c
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

# TODO : With standard settings below we get warnings as error due to unused functions.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I ../include")
set (CMAKE_C_FLAGS "-std=c99 -Wextra -Wno-sign-compare -Wno-unused-parameter -I ../include")

add_library (XGLLayerBasic SHARED basic_plugin.c)
add_library (XGLLayerGeneric SHARED generic_layer.c)
add_library (XGLLayerObjectTrack SHARED object_track.c)
add_library (XGLLayerAPIDump SHARED api_dump.c)
add_library (XGLLayerDrawState SHARED draw_state.c)

