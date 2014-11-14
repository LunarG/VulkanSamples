project(glvcommon)
cmake_minimum_required(VERSION 2.8)

include("${SRC_DIR}/build_options.cmake")

include_directories(
    ${SRC_DIR}/glvcommon
    ${SRC_DIR}/thirdparty
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    require_pthreads()
endif()

set(SRC_LIST
    ${SRC_LIST}
    glv_filelike.c
    glv_interconnect.c
    glv_platform.c
    glv_process.c
    glv_settings.c
    glv_tracelog.c
    glv_trace_packet_utils.c
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

file( GLOB_RECURSE HDRS *.[h|inl] )

if (NOT MSVC)
    add_compiler_flag("-fPIC")
endif()

add_library(${PROJECT_NAME} ${SRC_LIST} ${HDRS})


if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_link_libraries(${PROJECT_NAME}
    Rpcrt4.lib
)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_Libraries(${PROJECT_NAME}
    dl
    pthread
)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

build_options_finalize()

set_target_properties(glvcommon PROPERTIES LINKER_LANGUAGE C)
