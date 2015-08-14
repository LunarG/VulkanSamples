project(disasm)
cmake_minimum_required(VERSION 2.8)

include("${SRC_DIR}/build_options.cmake")

include_directories(
    ${SRC_DIR}/thirdparty/mhook/disasm-lib
    ${SRC_DIR}/vktrace_common
)

set(SRC_LIST
    cpu.c
    cpu.h
    disasm.c
    disasm.h
    disasm_x86.c
    disasm_x86.h
    disasm_x86_tables.h
    misc.c
    misc.h
)

if (NOT MSVC)
    add_compiler_flag("-fPIC")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

build_options_finalize()

