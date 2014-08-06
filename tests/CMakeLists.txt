SET(COMMON
    common.c
    )

add_executable(xglinfo xglinfo.c ${COMMON})
add_executable(xglAllocMemory xglAllocMemory.c ${COMMON})
target_link_libraries(xglinfo XGL)
target_link_libraries(xglAllocMemory XGL)
