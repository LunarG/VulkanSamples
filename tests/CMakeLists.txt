SET(COMMON
    common.c
    )

SET(COMMON_CPP
   testobject.cpp
   )

include_directories("${PROJECT_SOURCE_DIR}/gtest-1.7.0/include")


add_executable(xglinfo xglinfo.c ${COMMON})
add_executable(xglAllocMemory xglAllocMemory.c ${COMMON})
add_executable(xglFence xglFence.c ${COMMON})
add_executable(xglQueue xglQueue.c ${COMMON})
add_executable(xglEvent xglEvent.c ${COMMON})
add_executable(xglQuery xglQuery.c ${COMMON})
target_link_libraries(xglinfo XGL)
target_link_libraries(xglAllocMemory XGL)
target_link_libraries(xglFence XGL)
target_link_libraries(xglQueue XGL)
target_link_libraries(xglEvent XGL)
target_link_libraries(xglQuery XGL)

add_subdirectory(gtest-1.7.0)
