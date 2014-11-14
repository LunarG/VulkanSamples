include_directories (${CMAKE_CURRENT_SOURCE_DIR})

include("${SRC_DIR}/build_options.cmake")

set(SRC_LIST
    getopt_long.c
    getopt.h
)
add_library (getopt_bundled STATIC EXCLUDE_FROM_ALL ${SRC_LIST})


build_options_finalize()
