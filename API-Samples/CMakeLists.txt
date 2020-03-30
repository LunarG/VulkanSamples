cmake_minimum_required(VERSION 3.10.2)

macro(glsl_to_spirv src basename)
    add_custom_command(OUTPUT ${src}.h
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${CMAKE_CURRENT_BINARY_DIR}/ShaderHeaders/${src}.h ${GLSLANG_VALIDATOR} false
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${GLSLANG_VALIDATOR}
        )
endmacro()

macro(assembly_to_spirv src basename)
    add_custom_command(OUTPUT ${src}.h
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${CMAKE_CURRENT_BINARY_DIR}/ShaderHeaders/${src}.h ${SPIRV_TOOLS_ASSEMBLER} true
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${SPIRV_TOOLS_ASSEMBLER}
        )
endmacro()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ShaderHeaders)

# If ANDROID is ON, turn on cross-compiling for it
if(ANDROID)
    set(CMAKE_SYSTEM_NAME "Android")
    set(CMAKE_SYSTEM_VERSION "7")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    set(DisplayServer Win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (SAMPLES_WSI_SELECTION STREQUAL "XCB")
        if (NOT BUILD_WSI_XCB_SUPPORT)
            message( FATAL_ERROR "Selected XCB for samples build but not building Xcb support" )
        endif()
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    elseif (SAMPLES_WSI_SELECTION STREQUAL "XLIB")
        if (NOT BUILD_WSI_XLIB_SUPPORT)
            message( FATAL_ERROR "Selected XLIB for samples build but not building Xlib support" )
        endif()
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    elseif (SAMPLES_WSI_SELECTION STREQUAL "WAYLAND")
        if (NOT BUILD_WSI_WAYLAND_SUPPORT)
            message( FATAL_ERROR "Selected Wayland for samples build but not building Wayland support" )
        endif()
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

set(SAMPLES_DATA_DIR ${SAMPLES_DATA_DIR} "${CMAKE_SOURCE_DIR}/API-Samples/data")
set(SHADER_FILES ${SHADER_FILES} "")
include_directories( ${SAMPLES_DATA_DIR} ${GLMINC_PREFIX} ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/ShaderHeaders)

# Additional includes for spirv-tools
include_directories(${SPIRV_TOOLS_INCLUDE_DIR})

# function to build a simple single-source-file sample
#   - assumes S_TARGETS set to list of build targets
function(sampleWithSingleFile)
    foreach(TARG ${S_TARGETS})
        set(SAMPLE_NAME ${TARG})
        if(ANDROID)
            # Generate AndroidStudio project files.
            MESSAGE("Generating AndroidStudio project:" ${SAMPLE_NAME})
            # Replace "-" to "_", which is invalid in a package name.
            STRING(REPLACE "-" "_" PACKAGE_NAME ${SAMPLE_NAME})
            STRING(REGEX MATCH "^[0-9]" LEADING_DIGIT "${PACKAGE_NAME}")
            if(NOT LEADING_DIGIT STREQUAL "")
                set(PACKAGE_NAME "T${PACKAGE_NAME}")
            endif()
            set (PACKAGE_NAME vulkan.samples.${PACKAGE_NAME})
            set (SAMPLE_DESCRIPTION "Vulkan sample: ${SAMPLE_NAME}")
            file(COPY ./android/project_template/ DESTINATION ./android/${SAMPLE_NAME}/)

            # Clear flags.
            set (OTHER_FLAGS "")
            set (ASSET_DIR "")
            set (EXTRA_PERMISSIONS "")
            set (VALIDATIONLAYER_PACKING "")

            # Add extra setting for some samples.
            # Include resource files.
            set (SAMPLE_WITH_RESOURCES
             draw_textured_cube init_texture immutable_sampler memory_barriers multiple_sets pipeline_cache
             pipeline_derivative secondary_command_buffer separate_image_sampler spirv_assembly
             spirv_specialization template)
            if (";${SAMPLE_WITH_RESOURCES};" MATCHES ";${SAMPLE_NAME};")
                set (ASSET_DIR "sourceSets.main.assets.srcDirs '../../data'")
            endif()

            # Add external storage access permission.
            set (SAMPLE_WITH_EXTERNALSTORAGEACCESS pipeline_cache)
            if (";${SAMPLE_WITH_EXTERNALSTORAGEACCESS};" MATCHES ";${SAMPLE_NAME};")
                set (EXTRA_PERMISSIONS "<uses-permission android:name=\"android.permission.WRITE_EXTERNAL_STORAGE\"/>")
            endif()

            # Add validation layer settings.
            set (SAMPLE_WITH_TOOLS create_debug_report_callback enable_validation_with_callback)
            if (";${SAMPLE_WITH_TOOLS};" MATCHES ";${SAMPLE_NAME};")
                set (VALIDATIONLAYER_PACKING "sourceSets.main.jniLibs.srcDirs file(ndkDir).absolutePath +\n\t\t\t\t'/sources/third_party/vulkan/src/build-android/jniLibs'")
            endif()

            # Cook template files.
            set (TEMPLATE_FILES
                android/${SAMPLE_NAME}/src/main/res/values/strings.xml
                android/${SAMPLE_NAME}/src/main/AndroidManifest.xml
                android/${SAMPLE_NAME}/build.gradle
                android/${SAMPLE_NAME}/README.md
            )
            foreach(TEMPLATE_FILE ${TEMPLATE_FILES})
                configure_file(${TEMPLATE_FILE} ${TEMPLATE_FILE} @ONLY)
            endforeach(TEMPLATE_FILE)
            SET(SETTINGS_GRADLE "${SETTINGS_GRADLE}include ':${SAMPLE_NAME}'\n")
        elseif(UNIX)
            if (${SAMPLE_NAME} MATCHES spirv_assembly)
                assembly_to_spirv(${SAMPLE_NAME}.vert ${SAMPLE_NAME})
                assembly_to_spirv(${SAMPLE_NAME}.frag ${SAMPLE_NAME})
                set (sources ${SAMPLE_NAME}/${SAMPLE_NAME}.cpp)
                set (sources ${sources} ${SAMPLE_NAME}.vert.h)
                set (sources ${sources} ${SAMPLE_NAME}.frag.h)
            elseif (${SAMPLE_NAME} MATCHES spirv_specialization)
                set (sources ${SAMPLE_NAME}/${SAMPLE_NAME}.cpp)
                glsl_to_spirv(${SAMPLE_NAME}.vert ${SAMPLE_NAME})
                set (sources ${sources} ${SAMPLE_NAME}.vert.h)
                glsl_to_spirv(${SAMPLE_NAME}.frag ${SAMPLE_NAME})
                set (sources ${sources} ${SAMPLE_NAME}.frag.h)
                assembly_to_spirv(specialized.frag ${SAMPLE_NAME})
                set (sources ${sources} specialized.frag.h)
            else()
                set(sources ${SAMPLE_NAME}/${SAMPLE_NAME}.cpp)
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}.vert)
                    glsl_to_spirv(${SAMPLE_NAME}.vert ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}.vert.h)
                endif()
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}.full.vert)
                    glsl_to_spirv(${SAMPLE_NAME}.full.vert ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}.full.vert.h)
                endif()
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}.frag)
                    glsl_to_spirv(${SAMPLE_NAME}.frag ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}.frag.h)
                endif()
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}2.frag)
                    glsl_to_spirv(${SAMPLE_NAME}2.frag ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}2.frag.h)
                endif()
            endif()
            add_executable(${SAMPLE_NAME} ${sources})
            target_link_libraries(${SAMPLE_NAME} ${UTILS_NAME} ${XCB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${VULKAN_LOADER} ${PTHREAD})
        else()
            if (${SAMPLE_NAME} MATCHES spirv_assembly)
                assembly_to_spirv(${SAMPLE_NAME}.vert ${SAMPLE_NAME})
                assembly_to_spirv(${SAMPLE_NAME}.frag ${SAMPLE_NAME})
                set (sources ${SAMPLE_NAME}/${SAMPLE_NAME}.cpp)
                set (sources ${sources} ${SAMPLE_NAME}.vert.h)
                set (sources ${sources} ${SAMPLE_NAME}.frag.h)
            elseif (${SAMPLE_NAME} MATCHES spirv_specialization)
                set (sources ${SAMPLE_NAME}/${SAMPLE_NAME}.cpp)
                glsl_to_spirv(${SAMPLE_NAME}.vert ${SAMPLE_NAME})
                set (sources ${sources} ${SAMPLE_NAME}.vert.h)
                glsl_to_spirv(${SAMPLE_NAME}.frag ${SAMPLE_NAME})
                set (sources ${sources} ${SAMPLE_NAME}.frag.h)
                assembly_to_spirv(specialized.frag ${SAMPLE_NAME})
                set (sources ${sources} specialized.frag.h)
            else()
                set(sources ${SAMPLE_NAME}/${SAMPLE_NAME}.cpp)
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}.vert)
                    glsl_to_spirv(${SAMPLE_NAME}.vert ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}.vert.h)
                endif()
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}.full.vert)
                    glsl_to_spirv(${SAMPLE_NAME}.full.vert ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}.full.vert.h)
                endif()
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}.frag)
                    glsl_to_spirv(${SAMPLE_NAME}.frag ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}.frag.h)
                endif()
                if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}/${SAMPLE_NAME}2.frag)
                    glsl_to_spirv(${SAMPLE_NAME}2.frag ${SAMPLE_NAME})
                    set (sources ${sources} ${SAMPLE_NAME}2.frag.h)
                endif()
            endif()
            add_executable(${SAMPLE_NAME} WIN32 ${sources})
            target_link_libraries(${SAMPLE_NAME} ${UTILS_NAME} ${VULKAN_LOADER} ${WINLIBS})
        endif()
    endforeach(TARG)

    if(ANDROID)
        #generate Settings.gradle
        configure_file(android/settings.gradle.in android/settings.gradle @ONLY)
    endif()
endfunction(sampleWithSingleFile)

if(WIN32)
    set (MOVE_CMD "move")
    set (VULKAN_LOADER_NAME "${API_LOWERCASE}-${MAJOR}")
else()
    set (MOVE_CMD "mv")
    set (PTHREAD "pthread")
    set (VULKAN_LOADER_NAME "${API_LOWERCASE}")
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/${V_LVL_RELATIVE_LOCATION}/loader")
    set (VULKAN_LOADER ${VULKAN_LOADER_NAME})
else()
    find_library(VULKAN_LOADER NAMES ${VULKAN_LOADER_NAME}
    HINTS "${V_LVL_RELATIVE_LOCATION}/${LIBSOURCE_DIR}" "${V_LVL_RELATIVE_LOCATION}/x86_64/lib" )
endif()

set (LIBGLM_INCLUDE_DIR ${V_LVL_RELATIVE_LOCATION}/libs)

if(NOT WIN32 AND NOT ANDROID)
    if(SDK_INCLUDE_PATH)
        include_directories(${SDK_INCLUDE_PATH})
    endif()
    if (BUILD_WSI_XCB_SUPPORT)
        include_directories(${XCB_INCLUDE_DIRS})
        link_libraries(${XCB_LIBRARIES} m )
    endif()
    if (BUILD_WSI_WAYLAND_SUPPORT)
        include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
        link_libraries(${WAYLAND_CLIENT_LIBRARIES})
    endif()
    link_libraries(${VULKAN_LOADER} m ) 
endif()
if(WIN32)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")

    # If MSVC, disable some signed/unsigned mismatch warnings.
    if (MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4267")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267")
    endif()

else()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

if (NOT WIN32)
    # extra setup for out-of-tree builds
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        add_custom_target(samples-binary-dir-symlinks ALL
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_samples.sh
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run_all_samples.py
            COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/get-short-descripts.sh
            VERBATIM
            )
    endif()
else()
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lunarg.ppm LUNARG)
        FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/run_all_samples.py RUN_ALL)
        add_custom_target(samples-binary-dir-symlinks ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUN_ALL} run_all_samples.py
            VERBATIM
            )
    endif()
endif()


#
# START BUILDING SAMPLES HERE
#

# Generate Android project.
option(ANDROID OFF)

# simple one file sample targets, no additional files
set (S_TARGETS
    01-init_instance 02-enumerate_devices 03-init_device 04-init_command_buffer
    05-init_swapchain 06-init_depth_buffer 07-init_uniform_buffer
    08-init_pipeline_layout 09-init_descriptor_set 10-init_render_pass
    11-init_shaders 12-init_frame_buffers 13-init_vertex_buffer 14-init_pipeline 15-draw_cube
    16-vulkan_1_1
    enumerate_devices_adv init_texture multithreaded_command_buffers
    draw_textured_cube dynamic_uniform texel_buffer events
    instance_layer_properties instance_extension_properties
    instance_layer_extension_properties create_debug_report_callback
    enable_validation_with_callback multiple_sets
    copy_blit_image template separate_image_sampler input_attachment
    occlusion_query pipeline_cache pipeline_derivative push_descriptors
    immutable_sampler push_constants draw_subpasses secondary_command_buffer
    memory_barriers spirv_assembly spirv_specialization validation_cache vulkan_1_1_flexible)
sampleWithSingleFile()

if (NOT ANDROID)
foreach (sample ${S_TARGETS})
    install(TARGETS ${sample} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach(sample)

    add_subdirectory(utils)
endif()
