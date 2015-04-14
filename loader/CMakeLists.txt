add_custom_command(OUTPUT dispatch.c
	COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py loader-entrypoints > dispatch.c
	DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py
                ${PROJECT_SOURCE_DIR}/include/vkIcd.h)

add_custom_command(OUTPUT table_ops.h
	COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py dispatch-table-ops loader > table_ops.h
	DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)

add_custom_command(OUTPUT vulkan.def
	COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py win-def-file vulkan all > vulkan.def
	DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

# DEBUG enables runtime loader ICD verification
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if (WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -DXCB_NVIDIA")

    add_library(vulkan SHARED loader.c loader.h dirent_on_windows.c dispatch.c table_ops.h vulkan.def)
    set_target_properties(vulkan PROPERTIES LINK_FLAGS "/DEF:${PROJECT_SOURCE_DIR}/loader/vulkan.def")
    add_library(VKstatic STATIC loader.c loader.h dirent_on_windows.c dispatch.c table_ops.h)
    set_target_properties(VKstatic PROPERTIES OUTPUT_NAME VKstatic)
    target_link_libraries(vulkan)
endif()
if (NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -Wpointer-arith")

    add_library(vulkan SHARED loader.c dispatch.c table_ops.h)
    set_target_properties(vulkan PROPERTIES SOVERSION 0)
    target_link_libraries(vulkan -ldl -lpthread)
endif()
