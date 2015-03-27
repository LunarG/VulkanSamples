if(NOT WIN32)
    find_package(XCB REQUIRED)
endif()

if(NOT WIN32)
    if(NOT EXISTS /usr/include/glm/glm.hpp)
        message(FATAL_ERROR "Necessary libglm-dev headers cannot be found: sudo apt-get install libglm-dev")
    endif()
    if(NOT EXISTS /usr/include/png.h)
        message(FATAL_ERROR "Necessary png12-dev headers cannot be found: sudo apt-get install libpng12-dev")
    endif()
endif()

file(GLOB TEXTURES
  "${PROJECT_SOURCE_DIR}/demos/*.png"
  )
file(COPY ${TEXTURES} DESTINATION ${CMAKE_BINARY_DIR}/demos)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXGL_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXGL_PROTOTYPES")

if(WIN32)
    set (LIBRARIES "XGL")
endif()
if(NOT WIN32)
    set (LIBRARIES "XGL")
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/cube.vert
       COMMAND mv vert.spv ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       DEPENDS cube.vert ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/cube.frag
       COMMAND mv frag.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       DEPENDS cube.frag ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )
endif()

if(NOT WIN32)
    include_directories (
       ${XCB_INCLUDE_DIRS}
       "${PROJECT_SOURCE_DIR}/icd/common"
       )

    link_libraries(${XCB_LIBRARIES} XGL png m)
endif()
if(WIN32)
    include_directories (
       "${PROJECT_SOURCE_DIR}/icd/common"
       "${PROJECT_SOURCE_DIR}/libs/libpng"
       "${PROJECT_SOURCE_DIR}/libs/zlib"
       )

    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXCB_NVIDIA -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXCB_NVIDIA -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set (LIBRARIES ${LIBRARIES} xcb_nvidia)
else()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEXTERNAL_SPV")
endif()

add_executable(xglinfo xglinfo.c)
target_link_libraries(xglinfo ${LIBRARIES})

add_executable(tri tri.c)
target_link_libraries(tri ${LIBRARIES})

if(NOT WIN32)
    add_executable(cube cube.c ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv)
    target_link_libraries(cube ${LIBRARIES})
else()
    if (CMAKE_CL_64)
        set (LIB_DIR "Win64")
    else()
        set (LIB_DIR "Win32")
    endif()
    add_library(libpngd STATIC IMPORTED)
    set_target_properties(libpngd PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${LIB_DIR}/libpngd.lib)
    add_library(zlibd STATIC IMPORTED)
    set_target_properties(zlibd PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${LIB_DIR}/zlibd.lib)
    add_executable(cube cube.c)
    target_link_libraries(cube ${LIBRARIES} libpngd zlibd)
endif()


if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(PATH_TO_WIN_XGL ${CMAKE_CURRENT_BINARY_DIR}/../loader/${CMAKE_CFG_INTDIR})
    add_custom_command(TARGET tri
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PATH_TO_WIN_XGL}/XGL.dll" $<TARGET_FILE_DIR:tri>
    )
endif()
