project(mhook)
cmake_minimum_required(VERSION 2.8)

include("${SRC_DIR}/build_options.cmake")

include_directories(
    ${SRC_DIR}/thirdparty/mhook/mhook-lib
    ${SRC_DIR}/thirdparty/mhook/disasm-lib
    ${SRC_DIR}/glvcommon
)

set(SRC_LIST
    mhook.c
    mhook.h
)

set_source_files_properties( ${SRC_LIST} PROPERTIES LANGUAGE C)

if (NOT MSVC)
    add_compiler_flag("-fPIC")
endif()


add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} 
    disasm
)

build_options_finalize()

set_target_properties(mhook PROPERTIES LINKER_LANGUAGE C)
