SET(COMMON
    common.c
    )

SET(COMMON_CPP
   xglgpu.cpp
   )

include_directories("${PROJECT_SOURCE_DIR}/tests/gtest-1.7.0/include")


add_executable(xglinfo xglinfo.c ${COMMON})
add_executable(xglbase init.cpp ${COMMON_CPP})
set_target_properties(xglbase
   PROPERTIES
   COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(xglbase XGL gtest gtest_main)

target_link_libraries(xglinfo XGL)

add_subdirectory(gtest-1.7.0)
