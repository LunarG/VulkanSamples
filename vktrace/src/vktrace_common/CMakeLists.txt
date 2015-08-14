project(vktrace_common)
cmake_minimum_required(VERSION 2.8)

include("${SRC_DIR}/build_options.cmake")

include_directories(
    ${SRC_DIR}/vktrace_common
    ${SRC_DIR}/thirdparty
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    require_pthreads()
endif()

set(SRC_LIST
    ${SRC_LIST}
    vktrace_filelike.c
    vktrace_interconnect.c
    vktrace_platform.c
    vktrace_process.c
    vktrace_settings.c
    vktrace_tracelog.c
    vktrace_trace_packet_utils.c
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

file( GLOB_RECURSE HDRS *.[h|inl] )

if (NOT MSVC)
    add_compiler_flag("-fPIC")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_LIST} ${HDRS})


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

set_target_properties(vktrace_common PROPERTIES LINKER_LANGUAGE C)
