include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXGL_PROTOTYPES")

add_library(xcb_nvidia STATIC xcb_nvidia.cpp)
target_link_libraries(xcb_nvidia)
