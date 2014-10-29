pkg_check_modules(XCB REQUIRED xcb)
if (NOT XCB_FOUND)
	message(FATAL_ERROR "xcb not found")
endif()
if(NOT EXISTS /usr/include/glm/glm.hpp)
    message(FATAL_ERROR "Necessary libglm-dev headers cannot be found: sudo apt-get install libglm-dev")
endif()
if(NOT EXISTS /usr/include/png.h)
    message(FATAL_ERROR "Necessary png12-dev headers cannot be found: sudo apt-get install png12-dev")
endif()


include_directories (
   ${XCB_INCLUDE_DIRS}
   "${PROJECT_SOURCE_DIR}/icd/common"
   )

link_directories(${XCB_LIBRARY_DIRS})
link_libraries(${XCB_LIBRARIES} XGL png m)

add_executable(tri tri.c)
target_link_libraries(tri)

add_executable(cube cube.c)
target_link_libraries(cube)
