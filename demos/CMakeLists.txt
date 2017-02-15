if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    set(DisplayServer Win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (NOT DEMOS_WSI_SELECTION)
        set(DEMOS_WSI_SELECTION "XCB")
    endif()

    if (DEMOS_WSI_SELECTION STREQUAL "XCB")
        if (NOT BUILD_WSI_XCB_SUPPORT)
            message( FATAL_ERROR "Selected XCB for demos build but not building Xcb support" )
        endif()
        include_directories(${XCB_INCLUDE_DIRS})
        link_libraries(${XCB_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    elseif(DEMOS_WSI_SELECTION STREQUAL "XLIB")
        if (NOT BUILD_WSI_XLIB_SUPPORT)
            message( FATAL_ERROR "Selected XLIB for demos build but not building Xlib support" )
        endif()
        include_directories(${X11_INCLUDE_DIR})
        link_libraries(${X11_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    elseif(DEMOS_WSI_SELECTION STREQUAL "WAYLAND")
        if (NOT BUILD_WSI_WAYLAND_SUPPORT)
            message( FATAL_ERROR "Selected Wayland for demos build but not building Wayland support" )
        endif()
        include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
        link_libraries(${WAYLAND_CLIENT_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    elseif(DEMOS_WSI_SELECTION STREQUAL "MIR")
        if (NOT BUILD_WSI_MIR_SUPPORT)
            message( FATAL_ERROR "Selected MIR for demos build but not building Mir support" )
        endif()
        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
        include_directories(${MIR_INCLUDE_DIR})
        # TODO - Add Mir support
    elseif(DEMOS_WSI_SELECTION STREQUAL "DISPLAY")
        add_definitions(-DVK_USE_PLATFORM_DISPLAY_KHR)
    else()
        message( FATAL_ERROR "Unrecognized value for DEMOS_WSI_SELECTION: ${DEMOS_WSI_SELECTION}" )
     endif()

    include_directories ("${PROJECT_SOURCE_DIR}/icd/common")
    link_libraries(${API_LOWERCASE} m)
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

file(GLOB TEXTURES
  "${PROJECT_SOURCE_DIR}/demos/*.ppm"
  )
file(COPY ${TEXTURES} DESTINATION ${CMAKE_BINARY_DIR}/demos)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(WIN32)
  set (LIBRARIES "${API_LOWERCASE}-${MAJOR}")
elseif(UNIX)
  set (LIBRARIES "${API_LOWERCASE}")
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

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-vert.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${PROJECT_SOURCE_DIR}/demos/cube.vert
       DEPENDS cube.vert ${GLSLANG_VALIDATOR}
       )
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/demos/cube-frag.spv
       COMMAND ${GLSLANG_VALIDATOR} -s -V -o ${CMAKE_BINARY_DIR}/demos/cube-frag.spv ${PROJECT_SOURCE_DIR}/demos/cube.frag
       DEPENDS cube.frag ${GLSLANG_VALIDATOR}
       )
   file(COPY cube.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
   file(COPY vulkaninfo.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/demos)
else()
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
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

if(WIN32)
    include_directories (
       "${PROJECT_SOURCE_DIR}/icd/common"
       )

    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
endif()

add_executable(${API_LOWERCASE}info vulkaninfo.c)
target_link_libraries(${API_LOWERCASE}info ${LIBRARIES})

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
    target_link_libraries(cube ${LIBRARIES})
endif()

if(NOT WIN32)
    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        add_executable(cubepp cube.cpp ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv)
        target_link_libraries(cubepp ${LIBRARIES})
    endif()
else()
    if (CMAKE_CL_64)
        set (LIB_DIR "Win64")
    else()
        set (LIB_DIR "Win32")
    endif()

    add_executable(cubepp WIN32 cube.cpp ${CMAKE_BINARY_DIR}/demos/cube-vert.spv ${CMAKE_BINARY_DIR}/demos/cube-frag.spv)
    target_link_libraries(cubepp ${LIBRARIES})
endif()

if ((${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR}))
    if  ((DEMOS_WSI_SELECTION STREQUAL "XCB") OR (DEMOS_WSI_SELECTION STREQUAL "WAYLAND") OR WIN32 OR (CMAKE_SYSTEM_NAME STREQUAL "Android"))
        add_subdirectory(smoke)
    endif()
endif()

if(UNIX)
    install(TARGETS ${API_LOWERCASE}info DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
