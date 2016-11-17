project(Teapots)
cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")  #for find_package
#-------------------------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
endif()
if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
endif()
if(ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
endif()
#-------------------------------------------------------------------------------------------------
include_directories("./generated")

set(sources
    Game.h
    Helpers.h
    #generated/HelpersDispatchTable.cpp  //TODO: Use this dispatch table
    #generated/HelpersDispatchTable.h
    generated/Hologram.frag.h
    generated/Hologram.vert.h
    generated/Hologram.push_constant.vert.h
    Hologram.cpp
    Hologram.h
    main.cpp
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
    #PRIVATE -DVK_NO_PROTOTYPES
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

if(WIN32)
    list(APPEND definitions PRIVATE -DVK_USE_PLATFORM_WIN32_KHR)
    list(APPEND definitions PRIVATE -DWIN32_LEAN_AND_MEAN)
else()
    list(APPEND libraries PRIVATE -ldl)
    find_package(XCB REQUIRED)
    list(APPEND definitions PRIVATE -DVK_USE_PLATFORM_XCB_KHR)
    list(APPEND includes PRIVATE ${XCB_INCLUDES})
    list(APPEND libraries PRIVATE ${XCB_LIBRARIES})
endif()

find_package (Threads)

add_executable(${PROJECT_NAME} ${sources})

target_compile_definitions(${PROJECT_NAME} ${definitions})
target_include_directories(${PROJECT_NAME} ${includes})
target_link_libraries     (${PROJECT_NAME} ${libraries})
target_link_libraries     (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
#-------------------------------------------------------------------------------------------------

#-----------------WSIWindow-----------------
add_subdirectory(WSIWindow)
target_link_libraries(${PROJECT_NAME} Teapots_WSIWindow)
#-------------------------------------------
