if(NOT WIN32)
    find_package(XCB REQUIRED)
    find_package(X11 REQUIRED)
endif()

file(GLOB TEXTURES
  "${PROJECT_SOURCE_DIR}/demos/*.ppm"
  )
file(COPY ${TEXTURES} DESTINATION ${CMAKE_BINARY_DIR}/demos)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(WIN32)
  set (LIBRARIES "vulkan-${MAJOR}")
elseif(UNIX)
  set (LIBRARIES "vulkan")
else()
endif()

if(WIN32)
    # For Windows, since 32-bit and 64-bit items can co-exist, we build each in its own build directory.
    # 32-bit target data goes in build32, and 64-bit target data goes into build.  So, include/link the
    # appropriate data at build time.
    if (CMAKE_CL_64)
        set (BUILDTGT_DIR build)
    else ()
        set (BUILDTGT_DIR build32)
    endif()

    # Use static MSVCRT libraries
    foreach(configuration in CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
                             CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${configuration} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${configuration} "${${configuration}}")
        endif()
    endforeach()

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-vert.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V ${PROJECT_SOURCE_DIR}/demos/tri.vert
       COMMAND move vert.spv ${CMAKE_BINARY_DIR}/demos/tri-vert.spv
       DEPENDS tri.vert ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V ${PROJECT_SOURCE_DIR}/demos/tri.frag
       COMMAND move frag.spv ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       DEPENDS tri.frag ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V ${PROJECT_SOURCE_DIR}/demos/cube.vert
       COMMAND move vert.spv ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       DEPENDS cube.vert ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V ${PROJECT_SOURCE_DIR}/demos/cube.frag
       COMMAND move frag.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       DEPENDS cube.frag ${GLSLANG_VALIDATOR}
       )
   file(COPY cube.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
   file(COPY tri.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
   file(COPY vulkaninfo.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
else()
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-vert.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o tri-vert.spv ${PROJECT_SOURCE_DIR}/demos/tri.vert 
       DEPENDS tri.vert ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o tri-frag.spv ${PROJECT_SOURCE_DIR}/demos/tri.frag
       DEPENDS tri.frag ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o cube-vert.spv ${PROJECT_SOURCE_DIR}/demos/cube.vert
       DEPENDS cube.vert ${GLSLANG_VALIDATOR}
       )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o cube-frag.spv ${PROJECT_SOURCE_DIR}/demos/cube.frag
       DEPENDS cube.frag ${GLSLANG_VALIDATOR}
       )
endif()

if(NOT WIN32)
    include_directories (
       ${XCB_INCLUDE_DIRS}
       ${X11_INCLUDE_DIRS}
       "${PROJECT_SOURCE_DIR}/icd/common"
       )
    link_libraries(${XCB_LIBRARIES} ${X11_LIBRARIES} vulkan m)
endif()
if(WIN32)
    include_directories (
       "${PROJECT_SOURCE_DIR}/icd/common"
       )

    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
endif()

add_executable(vulkaninfo vulkaninfo.c)
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

    add_executable(cube WIN32 cube.c ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv)
    target_link_libraries(cube ${LIBRARIES} )
endif()

add_subdirectory(smoke)

