# Create the i965 Vulkan DRI library

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")

add_subdirectory(kmd)
add_subdirectory(compiler)

add_custom_command(OUTPUT gpa.c
    COMMAND ${PROJECT_SOURCE_DIR}/vk-generate.py icd-get-proc-addr > gpa.c
    DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)

set(sources
    buf.c
    cmd.c
    cmd_decode.c
    cmd_meta.c
    cmd_mi.c
    cmd_barrier.c
    cmd_pipeline.c
    desc.c
    dev.c
    instance.c
    event.c
    extension_utils.c
    fb.c
    fence.c
    format.c
    gpa.c
    gpu.c
    img.c
    layout.c
    mem.c
    obj.c
    pipeline.c
    query.c
    queue.c
    sampler.c
    shader.c
    state.c
    view.c
    )

set(definitions "")
set(include_dirs "")

set(libraries
    m
    icd
    intelkmd
    intelcompiler)

find_package(XCB COMPONENTS xcb xcb-dri3 xcb-present)
if(XCB_FOUND)
    list(APPEND include_dirs ${XCB_INCLUDE_DIRS})
    list(APPEND libraries ${XCB_LIBRARIES})

    if (DisplayServer MATCHES "Xcb")
        find_package(XCB COMPONENTS xcb)
        if(XCB_FOUND)
            # No include directories by purpose.
            list(APPEND libraries ${XCB_LIBRARIES})
        endif()
    endif()

    if (DisplayServer MATCHES "Xlib")
        find_package(X11_XCB COMPONENTS X11-xcb)
        if(X11_XCB_FOUND)
            # No include directories by purpose.
            list(APPEND libraries ${X11_XCB_LIBRARIES})
        endif()
    endif()

    list(APPEND sources wsi_x11.c)

    set_source_files_properties(wsi_x11.c PROPERTIES COMPILE_FLAGS "-I${PROJECT_SOURCE_DIR}/icd/intel/kmd/libdrm/include/drm")
else()
    list(APPEND sources wsi_null.c)
endif()

if (NOT WIN32)
    # extra setup for out-of-tree builds
    # intel ICD not built on Windows
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
            add_custom_target(intel_icd-json ALL
                COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/intel_icd.json
                VERBATIM
                )
    endif()
endif()

add_library(VK_i965 SHARED ${sources})
target_compile_definitions(VK_i965 PRIVATE ${definitions})
target_include_directories(VK_i965 PRIVATE ${include_dirs})
target_link_libraries(VK_i965 ${libraries})

# set -Bsymbolic for vkGetProcAddr()
set_target_properties(VK_i965 PROPERTIES
    COMPILE_FLAGS "-Wmissing-declarations"
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,-no-undefined -Wl,--exclude-libs,ALL")
