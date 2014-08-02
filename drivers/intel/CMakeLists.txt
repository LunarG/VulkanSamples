# Create the i965 XGL DRI library

include_directories("${PROJECT_SOURCE_DIR}/loader")

link_directories (
)

link_libraries (
   drm
   udev
)

SET(SOURCES 
    init_driver.c
    gpuinfo.c
    )

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.c.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.c" @ONLY)

list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.c" GitSHA1.h)

add_library(XGL_i965 SHARED
   ${SOURCES}
   )

IF(UNIX)
    IF(CMAKE_COMPILER_IS_GNUCC)
         SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

# set_target_properties(i915_xgl PROPERTIES PREFIX "")
