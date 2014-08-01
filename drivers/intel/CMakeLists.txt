# Create the i965 XGL DRI library

include_directories("${PROJECT_SOURCE_DIR}/loader")

link_directories (
)

link_libraries (
   drm
   udev
)


add_library(XGL_i965 SHARED
   init_driver.c
   gpuinfo.c
   )

IF(UNIX)
    IF(CMAKE_COMPILER_IS_GNUCC)
         SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

# set_target_properties(i915_xgl PROPERTIES PREFIX "")
