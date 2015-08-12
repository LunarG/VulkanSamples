cmake_minimum_required(VERSION 2.8.11)
set(SAMPLES_DATA_DIR ${SAMPLES_DATA_DIR} "${PROJECT_SOURCE_DIR}/data")
set(SHADER_FILES ${SHADER_FILES} "")

# function to build a single-source-file sample that uses glsl shaders;
#   - assumes VULKAN_VERSION set (i.e. vk0.2)
#   - assumes SAMPLE_GLSL_VERT_SHADERS and SAMPLE_GLSL_FRAG_SHADERS is set to
#     the list of shader files of that type, with .vert and .frag omitted
#   - uses glslangValidator to convert glsl to spirv
function(sampleWithGLSLShaders SNAME)
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
    target_link_libraries(${SAMPLE_NAME} ${VULKAN_LOADER} ${UTILS_NAME})
endfunction(sampleWithGLSLShaders)

# function to build a single-source-file sample that uses spirv shaders
#   - assumes VULKAN_VERSION set (i.e. vk0.2)
#   - assumes SAMPLE_SPIRV_SHADERS is set to list to list of spirv shsders
#     used by the sample, with the .spv omitted
function(sampleWithSPIRVShaders SNAME)
    set(SAMPLE_NAME ${VULKAN_VERSION}-${SNAME})
    set(SHADER_DIR ${SAMPLES_DATA_DIR}/${VULKAN_VERSION})
    foreach(SFILE ${SAMPLE_SPIRV_SHADERS})
        set(SHADER_FILES ${SHADER_FILES} ${SHADER_DIR}/${SFILE}.spv)
    endforeach(SFILE)
    add_executable(${SAMPLE_NAME} ${SAMPLE_NAME}.cpp ${SHADER_FILES})
    target_link_libraries(${SAMPLE_NAME} ${VULKAN_LOADER} ${UTILS_NAME})
endfunction(sampleWithSPIRVShaders)

# function to build a simple single-source-file sample
#   - assumes VULKAN_VERSION set (i.e. vk0.2)
#   - assumes S_TARGETS set to list of build targets
function(sampleWithSingleFile)
    foreach(TARG ${S_TARGETS})
        set(SAMPLE_NAME ${VULKAN_VERSION}-${TARG})
        if(UNIX)
            add_executable(${SAMPLE_NAME} ${SAMPLE_NAME}.cpp)
            target_link_libraries(${SAMPLE_NAME} ${UTILS_NAME} ${XCB_LIBRARIES} ${VULKAN_LOADER})
        else()
            add_executable(${SAMPLE_NAME} WIN32 ${SAMPLE_NAME}.cpp)
            target_link_libraries(${SAMPLE_NAME} ${UTILS_NAME} ${VULKAN_LOADER} ${WINLIBS})
        endif()
    endforeach(TARG)
endfunction(sampleWithSingleFile)

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

if(WIN32)
    set (VULKAN_LOADER
        "C:\\VulkanSDK\\0.0.2.3\\Bin\\vulkan.0.lib"
        )
    set (GLSLANG_VALIDATOR
        "C:\\VulkanSDK\\0.0.2.3\\Bin\\glslangValidator.exe"
        )
    set (MOVE_CMD "move")
else()
    set (VULKAN_LOADER
        "vulkan"
        )
    set (GLSLANG_VALIDATOR
        "/usr/bin/glslangValidator"
        )
    set (MOVE_CMD "mv")
endif()

set (LIBGLM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs)

if(NOT WIN32)
    include_directories(
        ${XCB_INCLUDE_DIRS}
    )
    link_libraries(${XCB_LIBRARIES} ${VULKAN_LOADER} png m )
endif()
if(WIN32)
    include_directories (
       "${PROJECT_SOURCE_DIR}/libs/libpng"
       "${PROJECT_SOURCE_DIR}/libs/zlib"
       )
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES /wd4267")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES /wd4267")
else()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -Wno-sign-compare")
endif()

#
# START BUILDING SAMPLES HERE
#

# TARGETS FOR VULKAN VERSION 0.2
set(VULKAN_VERSION vk0.2)

# simple one file sample targets, no additional files
set (S_TARGETS instance device enumerate depthbuffer initwsi)
sampleWithSingleFile()

# targets that use spv shaders

# usespirvshader
set(SAMPLE_SPIRV_SHADERS spirvshader-vert spirvshader-frag)
sampleWithSPIRVShaders(usespirvshader)

# targets that require glsl to spv shader conversion

# useglslshader
set(SAMPLE_GLSL_FRAG_SHADERS glslshader)
set(SAMPLE_GLSL_VERT_SHADERS glslshader)
sampleWithGLSLShaders(useglslshader)

# simple one file sample targets, no additional files
set (S_TARGETS instance_layer_properties)
sampleWithSingleFile()
