add_custom_command(OUTPUT dispatch.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py loader-entrypoints > dispatch.c
	DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py
                ${PROJECT_SOURCE_DIR}/include/xglIcd.h)

add_custom_command(OUTPUT table_ops.h
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py dispatch-table-ops loader > table_ops.h
	DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

# DEBUG enables runtime loader ICD verification
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if (WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS")

    add_library(XGL SHARED loader.c dirent_on_windows.c dispatch.c table_ops.h)
    add_library(XGLstatic STATIC loader.c dirent_on_windows.c dispatch.c table_ops.h)
    set_target_properties(XGLstatic PROPERTIES OUTPUT_NAME XGL)
    target_link_libraries(XGL)
endif()
if (NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES")

    add_library(XGL SHARED loader.c dispatch.c table_ops.h)
    set_target_properties(XGL PROPERTIES SOVERSION 0)
    target_link_libraries(XGL -ldl -lpthread)
endif()
