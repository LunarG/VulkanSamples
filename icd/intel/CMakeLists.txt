# Create the i965 XGL DRI library

link_directories (
    ${LIBDRM_LIBRARY_DIRS}
)

link_libraries (
   ${LIBDRM_LIBRARIES}
   udev
)

include_directories (
    ${LIBDRM_INCLUDE_DIRS}
)

SET(SOURCES 
    dispatch_tables.c
    init_driver.c
    gpu.c
    gpuinfo.c
    obj.c
    )

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.c.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.c" @ONLY)

list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.c" GitSHA1.h)

add_library(XGL_i965 SHARED
   ${SOURCES}
   $<TARGET_OBJECTS:icd>
   )

# set_target_properties(i915_xgl PROPERTIES PREFIX "")
