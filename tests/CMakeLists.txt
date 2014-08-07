SET(COMMON
    common.c
    )

add_executable(xglinfo xglinfo.c ${COMMON})
add_executable(xglAllocMemory xglAllocMemory.c ${COMMON})
add_executable(xglFence xglFence.c ${COMMON})
add_executable(xglQueue xglQueue.c ${COMMON})
target_link_libraries(xglinfo XGL)
target_link_libraries(xglAllocMemory XGL)
target_link_libraries(xglFence XGL)
target_link_libraries(xglQueue XGL)
