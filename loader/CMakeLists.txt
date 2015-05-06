include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

# DEBUG enables runtime loader ICD verification
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if (WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -DXCB_NVIDIA")

    add_library(vulkan SHARED loader.c loader.h loader_platform.h dirent_on_windows.c trampoline.c table_ops.h gpa_helper.h vulkan.def)
    set_target_properties(vulkan PROPERTIES LINK_FLAGS "/DEF:${PROJECT_SOURCE_DIR}/loader/vulkan.def")
    add_library(VKstatic STATIC loader.c loader.h dirent_on_windows.c dispatch.c table_ops.h gpa_helper.h)
    set_target_properties(VKstatic PROPERTIES OUTPUT_NAME VKstatic)
    target_link_libraries(vulkan)
endif()
if (NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -Wpointer-arith")

    add_library(vulkan SHARED loader.c trampoline.c loader.h loader_platform.h table_ops.h gpa_helper.h)
    set_target_properties(vulkan PROPERTIES SOVERSION 0)
    target_link_libraries(vulkan -ldl -lpthread)
endif()
