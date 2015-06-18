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

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

set (LIBRARIES "vulkan")

if(WIN32)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-vert.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/tri.vert
       COMMAND move vert.spv ${CMAKE_BINARY_DIR}/demos/tri-vert.spv
       DEPENDS tri.vert ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/tri.frag
       COMMAND move frag.spv ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       DEPENDS tri.frag ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/cube.vert
       COMMAND move vert.spv ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       DEPENDS cube.vert ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/cube.frag
       COMMAND move frag.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       DEPENDS cube.frag ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )
endif()
if(NOT WIN32)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-vert.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/tri.vert
       COMMAND mv vert.spv ${CMAKE_BINARY_DIR}/demos/tri-vert.spv
       DEPENDS tri.vert ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       COMMAND ${GLSLANG_PREFIX}/build/install/bin/glslangValidator -s -V ${PROJECT_SOURCE_DIR}/demos/tri.frag
       COMMAND mv frag.spv ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       DEPENDS tri.frag ${GLSLANG_PREFIX}/build/install/bin/glslangValidator
       )
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

    link_libraries(${XCB_LIBRARIES} vulkan png m)
endif()
if(WIN32)
    include_directories (
       "${PROJECT_SOURCE_DIR}/icd/common"
       "${PROJECT_SOURCE_DIR}/libs/libpng"
       "${PROJECT_SOURCE_DIR}/libs/zlib"
       )

    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
endif()

if(UNIX)
    add_executable(vulkaninfo vulkaninfo.c)
else()
    add_executable(vulkaninfo WIN32 vulkaninfo.c)
endif()
target_link_libraries(vulkaninfo ${LIBRARIES})

if(UNIX)
    add_executable(tri tri.c ${CMAKE_BINARY_DIR}/demos/tri-vert.spv ${CMAKE_BINARY_DIR}/demos/tri-frag.spv)
else()
    add_executable(tri WIN32 tri.c ${CMAKE_BINARY_DIR}/demos/tri-vert.spv ${CMAKE_BINARY_DIR}/demos/tri-frag.spv)
endif()
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
    add_executable(cube WIN32 cube.c ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv)
    target_link_libraries(cube ${LIBRARIES} libpngd zlibd)
endif()


if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(PATH_TO_WIN_VK ${CMAKE_CURRENT_BINARY_DIR}/../loader/${CMAKE_CFG_INTDIR})
    add_custom_command(TARGET tri
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PATH_TO_WIN_VK}/vulkan.dll" $<TARGET_FILE_DIR:tri>
    )
endif()
