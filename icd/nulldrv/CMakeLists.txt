# Create the nulldrv Vulkan DRI library

if (WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()

add_custom_command(OUTPUT nulldrv_gpa.c
    COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py icd-get-proc-addr > nulldrv_gpa.c
    DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)

set(sources
    nulldrv.c
    nulldrv_gpa.c
    )

set(definitions "")

set(libraries
    icd)

add_library(VK_nulldrv SHARED ${sources})
target_compile_definitions(VK_nulldrv PRIVATE ${definitions})
target_include_directories(VK_nulldrv PRIVATE ${include_dirs})
target_link_libraries(VK_nulldrv ${libraries})

if (WIN32)
    # Add in the DLL "map" file for vkGetProcAddr()
    set_target_properties(VK_nulldrv PROPERTIES
        LINK_FLAGS "/DEF:${PROJECT_SOURCE_DIR}/icd/nulldrv/VK_nulldrv.def")
else()
    set_target_properties(VK_nulldrv PROPERTIES
        LINK_FLAGS "-Wl,-Bsymbolic")
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        add_custom_target(nulldrv_icd-json ALL
        COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/nulldrv_icd.json
            VERBATIM
            )
    endif()
endif()
