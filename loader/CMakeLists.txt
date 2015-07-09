include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

if (WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vulkan.${MAJOR}.def
	    COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/loader/vk-loader-generate.py win-def-file vulkan.${MAJOR}.dll all > ${CMAKE_CURRENT_BINARY_DIR}/vulkan.${MAJOR}.def
	    DEPENDS ${PROJECT_SOURCE_DIR}/loader/vk-loader-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)
else()
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vulkan.def
	    COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/loader/vk-loader-generate.py win-def-file vulkan.dll all > ${CMAKE_CURRENT_BINARY_DIR}/vulkan.def
	    DEPENDS ${PROJECT_SOURCE_DIR}/loader/vk-loader-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)
endif()

# DEBUG enables runtime loader ICD verification
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(LOADER_SRCS
    loader.c
    loader.h
    vk_loader_platform.h
    trampoline.c
    wsi_lunarg.c
    wsi_lunarg.h
    debug_report.c
    debug_report.h
    table_ops.h
    gpa_helper.h
    cJSON.c
    cJSON.h
)

if (WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS")

    add_library(vulkan.${MAJOR} SHARED ${LOADER_SRCS} dirent_on_windows.c ${CMAKE_CURRENT_BINARY_DIR}/vulkan.${MAJOR}.def)
    set_target_properties(vulkan.${MAJOR} PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_BINARY_DIR}/vulkan.${MAJOR}.def")
    add_library(VKstatic.${MAJOR} STATIC ${LOADER_SRCS} dirent_on_windows.c)
    set_target_properties(VKstatic.${MAJOR} PROPERTIES OUTPUT_NAME VKstatic.${MAJOR})
    target_link_libraries(vulkan.${MAJOR} shlwapi)
endif()
if (NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -Wpointer-arith")

    add_library(vulkan SHARED ${LOADER_SRCS})
    set_target_properties(vulkan PROPERTIES SOVERSION 0)
    target_link_libraries(vulkan -ldl -lpthread -lm)
endif()
