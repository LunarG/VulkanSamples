cmake_minimum_required(VERSION 2.8.11)
set(SAMPLES_DATA_DIR ${SAMPLES_DATA_DIR} "${PROJECT_SOURCE_DIR}/data")
set(SHADER_FILES ${SHADER_FILES} "")
include_directories( ${SAMPLES_DATA_DIR} ${GLSLANG_PREFIX} ${GLMINC_PREFIX})

# function to build a single-source-file sample that uses glsl shaders;
#   - assumes VULKAN_VERSION set (i.e. vk0.9)
#   - assumes SAMPLE_GLSL_VERT_SHADERS and SAMPLE_GLSL_FRAG_SHADERS is set to
#     the list of shader files of that type, with .vert and .frag omitted
#   - uses glslangValidator to convert glsl to spirv
function(sampleExtGLSLShaders SNAME)
    set(SAMPLE_NAME ${VULKAN_VERSION}-${SNAME})
    set(SHADER_DIR ${SAMPLES_DATA_DIR}/${VULKAN_VERSION})
    foreach(SFILE ${SAMPLE_GLSL_VERT_SHADERS})
        add_custom_command(OUTPUT ${SHADER_DIR}/${SFILE}-vert.spv
            COMMAND ${GLSLANG_VALIDATOR} -s -V ${SHADER_DIR}/${SFILE}.vert
            COMMAND ${MOVE_CMD} vert.spv ${SHADER_DIR}/${SFILE}-vert.spv
            DEPENDS ${SHADER_DIR}/${SFILE}.vert ${GLSLANG_VALIDATOR}
        )
        set(SHADER_FILES ${SHADER_FILES} ${SHADER_DIR}/${SFILE}-vert.spv)
    endforeach(SFILE)
    foreach(SFILE ${SAMPLE_GLSL_FRAG_SHADERS})
        add_custom_command(OUTPUT ${SHADER_DIR}/${SFILE}-frag.spv
            COMMAND ${GLSLANG_VALIDATOR} -s -V ${SHADER_DIR}/${SFILE}.frag
            COMMAND ${MOVE_CMD} frag.spv ${SHADER_DIR}/${SFILE}-frag.spv
            DEPENDS ${SHADER_DIR}/${SFILE}.frag ${GLSLANG_VALIDATOR}
        )
        set(SHADER_FILES ${SHADER_FILES} ${SHADER_DIR}/${SFILE}-frag.spv)
    endforeach(SFILE)
    add_executable(${SAMPLE_NAME} ${SAMPLE_NAME}.cpp ${SHADER_FILES})
    target_link_libraries(${SAMPLE_NAME} ${VULKAN_LOADER} ${UTILS_NAME} ${GLSLANG_LIBRARIES} ${PTHREAD})
endfunction(sampleExtGLSLShaders)

# function to build a single-source-file sample that uses spirv shaders
#   - assumes VULKAN_VERSION set (i.e. vk0.9)
#   - assumes SAMPLE_SPIRV_SHADERS is set to list to list of spirv shsders
#     used by the sample, with the .spv omitted
function(sampleExtSPIRVShaders SNAME)
    set(SAMPLE_NAME ${VULKAN_VERSION}-${SNAME})
    set(SHADER_DIR ${SAMPLES_DATA_DIR}/${VULKAN_VERSION})
    foreach(SFILE ${SAMPLE_SPIRV_SHADERS})
        set(SHADER_FILES ${SHADER_FILES} ${SHADER_DIR}/${SFILE}.spv)
    endforeach(SFILE)
    add_executable(${SAMPLE_NAME} ${SAMPLE_NAME}.cpp ${SHADER_FILES})
    target_link_libraries(${SAMPLE_NAME} ${VULKAN_LOADER} ${UTILS_NAME} ${GLSLANG_LIBRARIES} ${PTHREAD})
endfunction(sampleExtSPIRVShaders)

# function to build a simple single-source-file sample
#   - assumes VULKAN_VERSION set (i.e. vk0.9)
#   - assumes S_TARGETS set to list of build targets
function(sampleWithSingleFile)
    foreach(TARG ${S_TARGETS})
        set(SAMPLE_NAME ${VULKAN_VERSION}-${TARG})
        if(UNIX)
            add_executable(${SAMPLE_NAME} ${SAMPLE_NAME}.cpp)
            target_link_libraries(${SAMPLE_NAME} ${UTILS_NAME} ${GLSLANG_LIBRARIES} ${XCB_LIBRARIES} ${VULKAN_LOADER} ${PTHREAD})
        else()
            add_executable(${SAMPLE_NAME} WIN32 ${SAMPLE_NAME}.cpp)
            target_link_libraries(${SAMPLE_NAME} ${UTILS_NAME} ${GLSLANG_LIBRARIES} ${VULKAN_LOADER} ${WINLIBS})
        endif()
    endforeach(TARG)
endfunction(sampleWithSingleFile)

# function to build a simple single-source-file sample
#   - assumes that code is standalone and doesn't depend on utilitites
#   - assumes VULKAN_VERSION set (i.e. vk0.9)
#   - assumes S_TARGETS set to list of build targets
function(sampleInlineGLSLShaders)
    foreach(TARG ${S_TARGETS})
        set(SAMPLE_NAME ${VULKAN_VERSION}-${TARG})
        if(UNIX)
            add_executable(${SAMPLE_NAME} ${SAMPLE_NAME}.cpp)
            target_link_libraries(${SAMPLE_NAME} ${GLSLANG_LIBRARIES} ${XCB_LIBRARIES} ${VULKAN_LOADER} ${PTHREAD})
        else()
            add_executable(${SAMPLE_NAME} WIN32 ${SAMPLE_NAME}.cpp)
            target_link_libraries(${SAMPLE_NAME} ${GLSLANG_LIBRARIES} ${VULKAN_LOADER} ${WINLIBS})
        endif()
    endforeach(TARG)
endfunction(sampleInlineGLSLShaders)

if(NOT WIN32)
    find_package(XCB REQUIRED)
endif()

find_library(VULKAN_LOADER NAMES vulkan-0 vulkan
             HINTS "${CMAKE_SOURCE_DIR}/../LoaderAndTools/build/loader"
                   "${CMAKE_SOURCE_DIR}/../LoaderAndTools/build/loader/Release"
                   "${PROJECT_SOURCE_DIR}\\..\\Bin\\Release" )

find_program(GLSLANG_VALIDATOR NAMES glslangValidator
             HINTS "${CMAKE_SOURCE_DIR}/../glslang/build/install/bin"
                   "${PROJECT_SOURCE_DIR}\\..\\Bin" )

if(WIN32)
    set (MOVE_CMD "move")
else()
    set (MOVE_CMD "mv")
    set (PTHREAD "pthread")
endif()

set (LIBGLM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs)

if(NOT WIN32)
    include_directories(
        ${XCB_INCLUDE_DIRS}
    )
    link_libraries(${XCB_LIBRARIES} ${VULKAN_LOADER} m )
endif()
if(WIN32)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES /wd4267")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES /wd4267")
else()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -Wno-sign-compare")
endif()

if (NOT WIN32)
    # extra setup for out-of-tree builds
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        add_custom_target(binary-dir-symlinks ALL
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_samples.sh
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/get-short-descripts.sh
            VERBATIM
            )
    endif()
else()
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lunarg.ppm LUNARG)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/run_all_samples.ps1 RUN_ALL)
        add_custom_target(binary-dir-symlinks ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUN_ALL} run_all_samples.ps1
            VERBATIM
            )
    endif()
endif()


if (WIN32)
    set (GLSLANG_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../glslang/build/glslang/Release"
                             "${CMAKE_SOURCE_DIR}/../glslang/build/glslang/OSDependent/Windows/Release"
                             "${CMAKE_SOURCE_DIR}/../glslang/build/OGLCompilersDLL/Release"
                             "${CMAKE_SOURCE_DIR}/../glslang/build/SPIRV/Release" )
else()
    set (GLSLANG_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../glslang/build/install/lib" "${CMAKE_SOURCE_DIR}/../x86_64/lib/glslang" )
endif()

find_library(GLSLANG_LIB NAMES glslang
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(OGLCompiler_LIB NAMES OGLCompiler
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(OSDependent_LIB NAMES OSDependent
             HINTS ${GLSLANG_SEARCH_PATH} )

find_library(SPIRV_LIB NAMES SPIRV
             HINTS ${GLSLANG_SEARCH_PATH} )

# On Windows, we must pair Debug and Release appropriately
if (WIN32)
    set (GLSLANG_DEBUG_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../glslang/build/glslang/Debug"
                                   "${CMAKE_SOURCE_DIR}/../glslang/build/glslang/OSDependent/Windows/Debug"
                                   "${CMAKE_SOURCE_DIR}/../glslang/build/OGLCompilersDLL/Debug"
                                   "${CMAKE_SOURCE_DIR}/../glslang/build/SPIRV/Debug")
    add_library(glslang     STATIC IMPORTED)
    add_library(OGLCompiler STATIC IMPORTED)
    add_library(OSDependent STATIC IMPORTED)
    add_library(SPIRV       STATIC IMPORTED)
    add_library(Loader      STATIC IMPORTED)

    find_library(GLSLANG_DLIB NAMES glslang
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(OGLCompiler_DLIB NAMES OGLCompiler
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(OSDependent_DLIB NAMES OSDependent
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(SPIRV_DLIB NAMES SPIRV
                 HINTS ${GLSLANG_DEBUG_SEARCH_PATH} )
    find_library(LOADER_DLIB NAMES vulkan-0
                 HINTS "${CMAKE_SOURCE_DIR}/../LoaderAndTools/build/loader/Debug"
                       "${PROJECT_SOURCE_DIR}\\..\\Bin\\Debug" )

    set_target_properties(glslang PROPERTIES
                         IMPORTED_LOCATION       "${GLSLANG_LIB}"
                         IMPORTED_LOCATION_DEBUG "${GLSLANG_DLIB}")
    set_target_properties(OGLCompiler PROPERTIES
                         IMPORTED_LOCATION       "${OGLCompiler_LIB}"
                         IMPORTED_LOCATION_DEBUG "${OGLCompiler_DLIB}")
    set_target_properties(OSDependent PROPERTIES
                         IMPORTED_LOCATION       "${OSDependent_LIB}"
                         IMPORTED_LOCATION_DEBUG "${OSDependent_DLIB}")
    set_target_properties(SPIRV PROPERTIES
                         IMPORTED_LOCATION       "${SPIRV_LIB}"
                         IMPORTED_LOCATION_DEBUG "${SPIRV_DLIB}")
    set_target_properties(Loader PROPERTIES
                         IMPORTED_LOCATION       "${VULKAN_LOADER}"
                         IMPORTED_LOCATION_DEBUG "${LOADER_DLIB}")

    set (GLSLANG_LIBRARIES glslang OGLCompiler OSDependent SPIRV Loader)
else ()
    set (GLSLANG_LIBRARIES ${GLSLANG_LIB} ${OGLCompiler_LIB} ${OSDependent_LIB} ${SPIRV_LIB})
endif()

#
# START BUILDING SAMPLES HERE
#

# TARGETS FOR VULKAN VERSION 0.9
set(VULKAN_VERSION vk0.9)

# simple one file sample targets, no additional files
set (S_TARGETS instance device enumerate enumerate-adv
    depthbuffer initwsi inittexture uniformbuffer
    descriptor_pipeline_layouts initcommandbuffer
    initrenderpass initframebuffers vertexbuffer
    allocdescriptorsets initshaders initpipeline
    drawcube drawtexturedcube
    instance_layer_properties instance_extension_properties
    instance_layer_extension_properties dbgcreatemsgcallback
    enable_validation_with_callback)
sampleWithSingleFile()

# simple one file sample targets, no additional files
set (S_TARGETS drawcubewithutilities)
sampleInlineGLSLShaders()

# targets that use spv shaders

# usespirvshader
set(SAMPLE_SPIRV_SHADERS spirvshader-vert spirvshader-frag)
sampleExtSPIRVShaders(usespirvshader)

# targets that require glsl to spv shader conversion

# useglslshader
set(SAMPLE_GLSL_FRAG_SHADERS glslshader)
set(SAMPLE_GLSL_VERT_SHADERS glslshader)
sampleExtGLSLShaders(useglslshader)

