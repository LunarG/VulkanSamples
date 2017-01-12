project(Hologram)
cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)
#-------------------------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
endif()
if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    add_definitions(-D-DWIN32_LEAN_AND_MEAN)
endif()
if(ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
endif()
#-------------------------------------------------------------------------------------------------
#------------------------------------------GLSL to SPIRV------------------------------------------
if(NOT EXISTS ${GLSLANG_VALIDATOR})
    message("WARNING : GLSLANG_VALIDATOR not found. Shaders will not be rebuilt.")
else()
    Message("GLSLANG_VALIDATOR=${GLSLANG_VALIDATOR}")
    find_package(PythonInterp 3 REQUIRED)
    macro(glsl_to_spirv src)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/generated/${src}.h
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/glsl-to-spirv ${CMAKE_CURRENT_SOURCE_DIR}/${src} ${CMAKE_CURRENT_SOURCE_DIR}/generated/${src}.h ${GLSLANG_VALIDATOR}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/glsl-to-spirv ${CMAKE_CURRENT_SOURCE_DIR}/${src} ${GLSLANG_VALIDATOR}
            )
    endmacro()
    glsl_to_spirv(Hologram.frag)
    glsl_to_spirv(Hologram.vert)
    glsl_to_spirv(Hologram.push_constant.vert)
endif()
#-------------------------------------------------------------------------------------------------

include_directories("./generated")

set(sources
    Game.h
    Helpers.h
    generated/Hologram.frag.h
    generated/Hologram.vert.h
    generated/Hologram.push_constant.vert.h
    Hologram.cpp
    Hologram.h
    Main.cpp
    Meshes.cpp
    Meshes.h
    Meshes.teapot.h
    Simulation.cpp
    Simulation.h
    Shell.h
    ShellWSI.cpp
    ShellWSI.h
    )

set(definitions
    PRIVATE -DGLM_FORCE_RADIANS)

if(SDK_INCLUDE_PATH)
    set(includes
        PRIVATE ${GLMINC_PREFIX}
        PRIVATE ${SDK_INCLUDE_PATH}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
else()
    set(includes
        PRIVATE ${GLMINC_PREFIX}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

set(libraries PRIVATE ${CMAKE_THREAD_LIBS_INIT})

if(TARGET vulkan)
    list(APPEND definitions PRIVATE -DUNINSTALLED_LOADER="$<TARGET_FILE:vulkan>")
endif()

list(APPEND includes PRIVATE "../../libs") #Path to GLM

find_package (Threads REQUIRED)

add_executable(${PROJECT_NAME} ${sources})

target_compile_definitions(${PROJECT_NAME} ${definitions})
target_include_directories(${PROJECT_NAME} ${includes})
target_link_libraries     (${PROJECT_NAME} ${libraries})
target_link_libraries     (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
#-------------------------------------------------------------------------------------------------

#---------------Find WSIWindow--------------
if(NOT TARGET WSIWindow)  # If WSIWindow is not already built...
    add_subdirectory(../../Utilities/WSIWindow ${CMAKE_BINARY_DIR}/WSIWindow)
endif()
target_link_libraries(${PROJECT_NAME} WSIWindow)
#-------------------------------------------
