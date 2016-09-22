if(NOT WIN32)
    if (BUILD_WSI_XCB_SUPPORT)
        find_package(XCB REQUIRED)
    endif()
    if (BUILD_WSI_XLIB_SUPPORT)
        find_package(X11 REQUIRED)
    endif()
    if (BUILD_WSI_WAYLAND_SUPPORT)
        find_package(Wayland REQUIRED)
    endif()
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
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o ${CMAKE_BINARY_DIR}/demos/tri-vert.spv ${PROJECT_SOURCE_DIR}/demos/tri.vert
       DEPENDS tri.vert ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/tri-frag.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o ${CMAKE_BINARY_DIR}/demos/tri-frag.spv ${PROJECT_SOURCE_DIR}/demos/tri.frag
       DEPENDS tri.frag ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${PROJECT_SOURCE_DIR}/demos/cube.vert
       DEPENDS cube.vert ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o ${CMAKE_BINARY_DIR}/demos/cube-frag.spv ${PROJECT_SOURCE_DIR}/demos/cube.frag
       DEPENDS cube.frag ${GLSLANG_VALIDATOR}
       )
   file(COPY cube.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
   file(COPY tri.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
   file(COPY vulkaninfo.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
else()
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
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
endif()

if(NOT WIN32)
    if(BUILD_WSI_XCB_SUPPORT)
        include_directories(${XCB_INCLUDE_DIRS})
        link_libraries(${XCB_LIBRARIES})
    endif()
    if(BUILD_WSI_XLIB_SUPPORT)
        include_directories(${X11_INCLUDE_DIRS})
        link_libraries(${X11_LIBRARIES})
    endif()
    if(BUILD_WSI_WAYLAND_SUPPORT)
        include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
        link_libraries(${WAYLAND_CLIENT_LIBRARIES})
    endif()

    include_directories ("${PROJECT_SOURCE_DIR}/icd/common")
    link_libraries(vulkan m)
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
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        add_executable(tri tri.c ${CMAKE_BINARY_DIR}/demos/tri-vert.spv ${CMAKE_BINARY_DIR}/demos/tri-frag.spv)
    endif()
else()
    add_executable(tri WIN32 tri.c ${CMAKE_BINARY_DIR}/demos/tri-vert.spv ${CMAKE_BINARY_DIR}/demos/tri-frag.spv)
endif()
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
    target_link_libraries(tri ${LIBRARIES})
endif()

if(NOT WIN32)
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        add_executable(cube cube.c ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv)
        target_link_libraries(cube ${LIBRARIES})
    endif()
else()
    if (CMAKE_CL_64)
        set (LIB_DIR "Win64")
    else()
        set (LIB_DIR "Win32")
    endif()

    add_executable(cube WIN32 cube.c ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv)
    target_link_libraries(cube ${LIBRARIES} )
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
    add_subdirectory(smoke)
endif()
