add_custom_command(OUTPUT dispatch.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py loader-entrypoints > dispatch.c
	DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

add_custom_command(OUTPUT table_ops.h
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py dispatch-table-ops loader > table_ops.h
	DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES")

if (WIN32)
    add_library(XGL SHARED loader.c dirent_on_windows.c dispatch.c table_ops.h)
endif()
if (NOT WIN32)
    add_library(XGL SHARED loader.c dispatch.c table_ops.h)
endif()
set_target_properties(XGL PROPERTIES SOVERSION 0)
target_link_libraries(XGL -ldl -lpthread)
