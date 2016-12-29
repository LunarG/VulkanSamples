cmake_minimum_required (VERSION 2.8.11)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    set(DisplayServer Win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
       add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
       add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif()

    if (BUILD_WSI_MIR_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_MIR_KHR)
        include_directories(${MIR_INCLUDE_DIR})
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

macro(run_vk_helper subcmd)
    add_custom_command(OUTPUT ${ARGN}
        COMMAND ${PYTHON_CMD} ${SCRIPTS_DIR}/vk_helper.py --${subcmd} ${PROJECT_SOURCE_DIR}/include/vulkan/vulkan.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${SCRIPTS_DIR}/vk_helper.py ${PROJECT_SOURCE_DIR}/include/vulkan/vulkan.h
    )
endmacro()

macro(run_vk_xml_generate dependency output)
    add_custom_command(OUTPUT ${output}
        COMMAND ${PYTHON_CMD} ${SCRIPTS_DIR}/lvl_genvk.py -registry ${SCRIPTS_DIR}/vk.xml ${output}
        DEPENDS ${SCRIPTS_DIR}/vk.xml ${SCRIPTS_DIR}/generator.py ${SCRIPTS_DIR}/${dependency} ${SCRIPTS_DIR}/lvl_genvk.py ${SCRIPTS_DIR}/reg.py
    )
endmacro()

set(LAYER_JSON_FILES
    VkLayer_core_validation
    VkLayer_image
    VkLayer_object_tracker
    VkLayer_unique_objects
    VkLayer_parameter_validation
    VkLayer_swapchain
    VkLayer_threading
    )

if (WIN32)
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        if (CMAKE_GENERATOR MATCHES "^Visual Studio.*")
            foreach (config_file ${LAYER_JSON_FILES})
                FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/windows/${config_file}.json src_json)
                FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${config_file}.json dst_json)
                add_custom_target(${config_file}-json ALL
                    COMMAND copy ${src_json} ${dst_json}
                    VERBATIM
                    )
                add_dependencies(${config_file}-json ${config_file})
            endforeach(config_file)
        else()
            foreach (config_file ${LAYER_JSON_FILES})
                FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/windows/${config_file}.json src_json)
                FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${config_file}.json dst_json)
                add_custom_target(${config_file}-json ALL
                    COMMAND copy ${src_json} ${dst_json}
                    VERBATIM
                    )
                add_dependencies(${config_file}-json ${config_file})
            endforeach(config_file)
        endif()
    endif()
else()
    # extra setup for out-of-tree builds
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        foreach (config_file ${LAYER_JSON_FILES})
            add_custom_target(${config_file}-json ALL
                COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/linux/${config_file}.json
                VERBATIM
                )
                add_dependencies(${config_file}-json ${config_file})
        endforeach(config_file)
    endif()
endif()

# Add targets for JSON file install on Linux.
# Need to remove the "./" from the library path before installing to /etc.
if(UNIX)
    foreach (config_file ${LAYER_JSON_FILES})
        add_custom_target(${config_file}-staging-json ALL
            COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/staging-json
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/linux/${config_file}.json ${CMAKE_CURRENT_BINARY_DIR}/staging-json
            COMMAND sed -i -e "/\"library_path\":/s$./libVkLayer$libVkLayer$" ${CMAKE_CURRENT_BINARY_DIR}/staging-json/${config_file}.json
            VERBATIM
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linux/${config_file}.json
            )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/staging-json/${config_file}.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/vulkan/explicit_layer.d)
    endforeach(config_file)
endif()

if (WIN32)
    macro(add_vk_layer target)
    FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_${target}.def DEF_FILE)
    add_custom_target(copy-${target}-def-file ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEF_FILE} VkLayer_${target}.def
        VERBATIM
    )
    add_library(VkLayer_${target} SHARED ${ARGN} VkLayer_${target}.def)
    target_link_Libraries(VkLayer_${target} VkLayer_utils)
    add_dependencies(VkLayer_${target} generate_dispatch_table_helper generate_vk_layer_helpers generate_enum_string_helper VkLayer_utils)
    endmacro()
else()
    macro(add_vk_layer target)
    add_library(VkLayer_${target} SHARED ${ARGN})
    target_link_Libraries(VkLayer_${target} VkLayer_utils)
    add_dependencies(VkLayer_${target} generate_dispatch_table_helper generate_vk_layer_helpers generate_enum_string_helper VkLayer_utils)
    set_target_properties(VkLayer_${target} PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic,--exclude-libs,ALL")
    install(TARGETS VkLayer_${target} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endmacro()
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../loader
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/vulkan
    ${CMAKE_CURRENT_BINARY_DIR}
)

if (WIN32)
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS")
    set (CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS")
    set (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS /bigobj")
    set (CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS /bigobj")
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpointer-arith -Wno-unused-function -Wno-sign-compare")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith -Wno-unused-function -Wno-sign-compare")
endif()

run_vk_helper(gen_struct_wrappers
    vk_safe_struct.h
    vk_safe_struct.cpp
)

set_source_files_properties(
    vk_safe_struct.h
    vk_safe_struct.cpp
    PROPERTIES GENERATED TRUE)

add_custom_target(generate_enum_string_helper DEPENDS
    vk_enum_string_helper.h
    vk_struct_size_helper.h
    vk_struct_size_helper.c
)

add_custom_target(generate_dispatch_table_helper DEPENDS
    vk_dispatch_table_helper.h
)

add_custom_target(generate_vk_layer_helpers DEPENDS
    vk_safe_struct.h
    vk_safe_struct.cpp
)

run_vk_xml_generate(helper_file_generator.py vk_struct_size_helper.h)
run_vk_xml_generate(helper_file_generator.py vk_struct_size_helper.c)
run_vk_xml_generate(helper_file_generator.py vk_enum_string_helper.h)
run_vk_xml_generate(threading_generator.py thread_check.h)
run_vk_xml_generate(parameter_validation_generator.py parameter_validation.h)
run_vk_xml_generate(unique_objects_generator.py unique_objects_wrappers.h)
run_vk_xml_generate(dispatch_table_generator.py vk_dispatch_table_helper.h)

# Layer Utils Library
# For Windows, we use a static lib because the Windows loader has a fairly restrictive loader search
# path that can't be easily modified to point it to the same directory that contains the layers.
if (WIN32)
    add_library(VkLayer_utils STATIC vk_layer_config.cpp vk_layer_extension_utils.cpp vk_layer_utils.cpp)
else()
    add_library(VkLayer_utils SHARED vk_layer_config.cpp vk_layer_extension_utils.cpp vk_layer_utils.cpp)
    install(TARGETS VkLayer_utils DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

add_vk_layer(core_validation core_validation.cpp vk_layer_table.cpp descriptor_sets.cpp)
add_vk_layer(object_tracker object_tracker.cpp vk_layer_table.cpp)
add_vk_layer(image image.cpp vk_layer_table.cpp)
add_vk_layer(swapchain swapchain.cpp vk_layer_table.cpp)
# generated
add_vk_layer(threading threading.cpp thread_check.h vk_layer_table.cpp)
add_vk_layer(unique_objects unique_objects.cpp unique_objects_wrappers.h vk_layer_table.cpp)
add_vk_layer(parameter_validation parameter_validation.cpp parameter_validation.h vk_layer_table.cpp)

# Core validation has additional dependencies
target_include_directories(VkLayer_core_validation PRIVATE ${GLSLANG_SPIRV_INCLUDE_DIR})
target_include_directories(VkLayer_core_validation PRIVATE ${SPIRV_TOOLS_INCLUDE_DIR})
target_link_libraries(VkLayer_core_validation ${SPIRV_TOOLS_LIBRARIES})
