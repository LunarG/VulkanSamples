link_libraries(XGL)

pkg_check_modules(XCB xcb)

if (XCB_FOUND)
	include_directories (${XCB_INCLUDE_DIRS})
	link_directories(${XCB_LIBRARY_DIRS})
	link_libraries(${XCB_LIBRARIES})

	add_executable(tri tri.c)
endif()
