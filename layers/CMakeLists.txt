cmake_minimum_required (VERSION 2.8.11)

add_custom_command(OUTPUT generic_layer.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py generic-layer > generic_layer.c
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I ../include")

add_library (XGLLayerBasic SHARED basic_plugin.c)
add_library (XGLLayerGeneric SHARED generic_layer.c)
#add_library (XGLLayerObjectTrack SHARED object_track.c)

