# Create the i965 XGL DRI library

add_subdirectory(compiler)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES")

find_package(DRM REQUIRED COMPONENTS libdrm libdrm_intel)

add_custom_command(OUTPUT intel_gpa.c
    COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-get-proc-addr > intel_gpa.c
    DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

set(sources
    buf.c
    cmd.c
    cmd_decode.c
    cmd_meta.c
    cmd_mi.c
    cmd_prepare.c
    cmd_pipeline.c
    desc.c
    dev.c
    intel.c
    intel_gpa.c
    event.c
    fb.c
    fence.c
    format.c
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
    kmd/winsys_drm.c
    )

set(definitions "")
set(include_dirs ${DRM_INCLUDE_DIRS})

set(libraries
    ${DRM_LIBRARIES}
    m
    icd
    intelcompiler)

find_package(XCB COMPONENTS xcb xcb-dri3 xcb-present)
if(XCB_FOUND)
    list(APPEND definitions -DENABLE_WSI_X11)
    list(APPEND include_dirs ${XCB_INCLUDE_DIRS})
    list(APPEND libraries ${XCB_LIBRARIES})
    list(APPEND sources wsi_x11.c)
endif()

add_library(XGL_i965 SHARED ${sources})
target_compile_definitions(XGL_i965 PRIVATE ${definitions})
target_include_directories(XGL_i965 PRIVATE ${include_dirs})
target_link_libraries(XGL_i965 ${libraries})

# set -Bsymbolic for xglGetProcAddr()
set_target_properties(XGL_i965 PROPERTIES
    COMPILE_FLAGS "-Wmissing-declarations"
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,-no-undefined -Wl,--exclude-libs,ALL")
