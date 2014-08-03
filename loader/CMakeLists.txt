add_custom_command(OUTPUT dispatch.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py loader > dispatch.c
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

add_library(XGL SHARED loader.c dispatch.c)
set_target_properties(XGL PROPERTIES SOVERSION 0)
target_link_libraries(XGL -ldl -lpthread)
