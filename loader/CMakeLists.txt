add_custom_command(OUTPUT dispatch.c ${PROJECT_SOURCE_DIR}/icd/common/icd-dispatch-table.h
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py loader > dispatch.c
        COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-dispatch-table > ${PROJECT_SOURCE_DIR}/icd/common/icd-dispatch-table.h
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

add_library(XGL SHARED loader.c dispatch.c)
set_target_properties(XGL PROPERTIES SOVERSION 0)
target_link_libraries(XGL -ldl -lpthread)
