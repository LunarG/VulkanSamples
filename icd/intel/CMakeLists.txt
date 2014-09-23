# Create the i965 XGL DRI library

pkg_check_modules(DRM REQUIRED libdrm libdrm_intel)

include_directories (
	${ICD_INCLUDE_DIRS}
	${DRM_INCLUDE_DIRS}
)

link_directories (
	${ICD_LIBRARY_DIRS}
	${DRM_LIBRARY_DIRS}
)

link_libraries (
	${ICD_LIBRARIES}
	${DRM_LIBRARIES}
	m
)

SET(SOURCES 
    cmd.c
    cmd_decode.c
    cmd_mi.c
    cmd_prepare.c
    cmd_pipeline.c
    dev.c
    dispatch.c
    dset.c
    intel.c
    event.c
    fence.c
    format.c
    gpu.c
    img.c
    layout.c
    mem.c
    obj.c
    pipeline.c
    pipeline_shader.c
    query.c
    queue.c
    sampler.c
    shader.c
    state.c
    view.c
    kmd/winsys_drm.c
    )

pkg_check_modules(DRI3 xcb xcb-dri3 xcb-present)
if (DRI3_FOUND)
	add_definitions(-DENABLE_WSI_X11)
	include_directories(${DRI3_INCLUDE_DIRS})
	link_directories(${DRI3_LIBRARY_DIRS})
	link_libraries(${DRI3_LIBRARIES})
	set(SOURCES ${SOURCES} wsi_x11.c)
endif()

add_library(XGL_i965 SHARED
   ${SOURCES}
   $<TARGET_OBJECTS:icd>
   )
