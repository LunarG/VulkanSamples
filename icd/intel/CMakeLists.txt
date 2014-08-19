# Create the i965 XGL DRI library

link_directories (
    ${LIBDRM_LIBRARY_DIRS}
    ${LIBDRM_INTEL_LIBRARY_DIRS}
)

link_libraries (
   ${LIBDRM_LIBRARIES}
   ${LIBDRM_INTEL_LIBRARIES}
   udev
   m
)

include_directories (
    ${LIBDRM_INCLUDE_DIRS}
    ${LIBDRM_INTEL_INCLUDE_DIRS}
)

SET(SOURCES 
    cmd.c
    dev.c
    dispatch.c
    dset.c
    init_driver.c
    event.c
    fence.c
    format.c
    gpu.c
    img.c
    layout.c
    mem.c
    obj.c
    query.c
    queue.c
    sampler.c
    state.c
    view.c
    kmd/winsys_drm.c
    )

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.c.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.c" @ONLY)

list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.c" GitSHA1.h)

add_library(XGL_i965 SHARED
   ${SOURCES}
   $<TARGET_OBJECTS:icd>
   )

# set_target_properties(i915_xgl PROPERTIES PREFIX "")
