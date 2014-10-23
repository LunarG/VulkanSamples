pkg_check_modules(XCB REQUIRED xcb)
if (NOT XCB_FOUND)
	message(FATAL_ERROR "xcb not found")
endif()

include_directories (
   ${XCB_INCLUDE_DIRS}
   "${PROJECT_SOURCE_DIR}/icd/common"
   )

link_directories(${XCB_LIBRARY_DIRS})
link_libraries(${XCB_LIBRARIES} XGL)

add_executable(tri tri.c)
target_link_libraries(tri)

add_executable(cube cube.c)
target_link_libraries(cube)
