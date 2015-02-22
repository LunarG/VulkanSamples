if (WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
endif()

set(sources
    icd-format.c
    icd-instance.c
    icd-log.c
    icd-utils.c)

set(include_dirs "")
set(libraries "")

if(UNIX)
    find_package(UDev REQUIRED)
    list(APPEND include_dirs ${UDEV_INCLUDE_DIRS})
    list(APPEND libraries ${UDEV_LIBRARIES})
    list(APPEND sources icd-enumerate-drm.c)
endif()

add_library(icd STATIC ${sources})
target_include_directories(icd
    PRIVATE ${include_dirs}
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(icd ${libraries})
set_target_properties(icd PROPERTIES POSITION_INDEPENDENT_CODE ON)
