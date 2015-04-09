# Create the i965 XGL DRI library

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")

add_subdirectory(kmd)
add_subdirectory(compiler)

add_custom_command(OUTPUT gpa.c
    COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-get-proc-addr > gpa.c
    DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

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
    list(APPEND sources wsi_x11.c)

    set_source_files_properties(wsi_x11.c PROPERTIES COMPILE_FLAGS "-I${PROJECT_SOURCE_DIR}/icd/intel/kmd/libdrm/include/drm")
else()
    list(APPEND sources wsi_null.c)
endif()

add_library(VK_i965 SHARED ${sources})
target_compile_definitions(VK_i965 PRIVATE ${definitions})
target_include_directories(VK_i965 PRIVATE ${include_dirs})
target_link_libraries(VK_i965 ${libraries})

# set -Bsymbolic for vkGetProcAddr()
set_target_properties(VK_i965 PROPERTIES
    COMPILE_FLAGS "-Wmissing-declarations"
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,-no-undefined -Wl,--exclude-libs,ALL")
