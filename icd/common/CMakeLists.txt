add_custom_command(OUTPUT icd-dispatch-table.h
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-dispatch-table > icd-dispatch-table.h
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

add_custom_command(OUTPUT icd-dispatch-entrypoints.c
	COMMAND ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-dispatch-entrypoints icd-dispatch-table.h > icd-dispatch-entrypoints.c
	           DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py
	                   ${PROJECT_SOURCE_DIR}/xgl.py)

set(ICD_SOURCES
	icd.c
	icd-dispatch-entrypoints.c
	icd-dispatch-table.h
	icd-format.c
	icd-utils.c)

set(ICD_REQUIRED_MODULES)

if (UNIX)
	set(ICD_REQUIRED_MODULES ${ICD_REQUIRED_MODULES} libudev)
	set(ICD_SOURCES ${ICD_SOURCES} icd-enumerate-drm.c)
endif()

pkg_check_modules(ICD REQUIRED ${ICD_REQUIRED_MODULES})

# icd-dispatch-table.h was generated in ${CMAKE_CURRENT_BINARY_DIR}
set(ICD_INCLUDE_DIRS
	${ICD_INCLUDE_DIRS}
	${CMAKE_CURRENT_BINARY_DIR}
	CACHE INTERNAL "")

include_directories(
	${ICD_INCLUDE_DIRS}
)

add_library(icd OBJECT ${ICD_SOURCES})
set_target_properties(icd PROPERTIES POSITION_INDEPENDENT_CODE ON)
