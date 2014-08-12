SET(COMMON
    common.c
    )

SET(COMMON_CPP
   xglgpu.cpp
   )

include_directories("${PROJECT_SOURCE_DIR}/tests/gtest-1.7.0/include")


add_executable(xglinfo xglinfo.c ${COMMON})
add_executable(xglAllocMemory xglAllocMemory.c ${COMMON})
add_executable(xglFence xglFence.c ${COMMON})
add_executable(xglQueue xglQueue.c ${COMMON})
add_executable(xglEvent xglEvent.c ${COMMON})
add_executable(xglQuery xglQuery.c ${COMMON})
add_executable(xglinit init.cpp ${COMMON_CPP})
set_target_properties(xglinit
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xglinit XGL gtest gtest_main)

target_link_libraries(xglinfo XGL)
target_link_libraries(xglAllocMemory XGL)
target_link_libraries(xglFence XGL)
target_link_libraries(xglQueue XGL)
target_link_libraries(xglEvent XGL)
target_link_libraries(xglQuery XGL)

add_subdirectory(gtest-1.7.0)
