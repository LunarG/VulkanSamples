# Create the nulldrv XGL DRI library

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")

add_custom_command(OUTPUT nulldrv_gpa.c
    COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/xgl-generate.py icd-get-proc-addr > nulldrv_gpa.c
    DEPENDS ${PROJECT_SOURCE_DIR}/xgl-generate.py ${PROJECT_SOURCE_DIR}/xgl.py)

set(sources
    nulldrv.c
    nulldrv_gpa.c
    )

set(definitions "")

set(libraries
    icd)

if (WIN32)
    list(APPEND definitions -DENABLE_WSI_WINDOWS)
endif()

add_library(XGL_nulldrv SHARED ${sources})
target_compile_definitions(XGL_nulldrv PRIVATE ${definitions})
target_include_directories(XGL_nulldrv PRIVATE ${include_dirs})
target_link_libraries(XGL_nulldrv ${libraries})

if (WIN32)
    # Add in the DLL "map" file for xglGetProcAddr()
    set_target_properties(XGL_nulldrv PROPERTIES
        LINK_FLAGS "/DEF:${PROJECT_SOURCE_DIR}/icd/nulldrv/XGL_nulldrv.def")
endif()

