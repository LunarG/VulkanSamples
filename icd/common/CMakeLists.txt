# the use of CMAKE_CURRENT_SOURCE_DIR pollutes the source tree
add_custom_command(OUTPUT icd-dispatch-table.h
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-dispatch-table > ${CMAKE_CURRENT_SOURCE_DIR}/icd-dispatch-table.h
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

add_custom_command(OUTPUT icd-dispatch-entrypoints.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-dispatch-entrypoints icd-dispatch-table.h > icd-dispatch-entrypoints.c
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

add_library(icd OBJECT icd.c icd-dispatch-entrypoints.c icd-dispatch-table.h icd-format.c icd-utils.c)
set_target_properties(icd PROPERTIES POSITION_INDEPENDENT_CODE ON)
