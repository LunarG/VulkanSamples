add_custom_command(OUTPUT dispatch.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py loader-entrypoints > dispatch.c
	DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(XGL SHARED loader.c dispatch.c)
set_target_properties(XGL PROPERTIES SOVERSION 0)
target_link_libraries(XGL -ldl -lpthread)
