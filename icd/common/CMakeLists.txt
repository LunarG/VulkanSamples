set(ICD_SOURCES
	icd-alloc.c
	icd-format.c
	icd-log.c
	icd-utils.c)

set(ICD_REQUIRED_MODULES)

if (UNIX)
	set(ICD_REQUIRED_MODULES ${ICD_REQUIRED_MODULES} libudev)
	set(ICD_SOURCES ${ICD_SOURCES} icd-enumerate-drm.c)
endif()

pkg_check_modules(ICD REQUIRED ${ICD_REQUIRED_MODULES})

include_directories(
	${ICD_INCLUDE_DIRS}
)

add_library(icd STATIC ${ICD_SOURCES})
set_target_properties(icd PROPERTIES POSITION_INDEPENDENT_CODE ON)
