cmake_minimum_required (VERSION 2.8.11)

macro(run_vk_helper subcmd)
    add_custom_command(OUTPUT ${ARGN}
        COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk_helper.py --${subcmd} ${PROJECT_SOURCE_DIR}/include/vulkan/vulkan.h --abs_out_dir ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${PROJECT_SOURCE_DIR}/vk_helper.py ${PROJECT_SOURCE_DIR}/include/vulkan/vulkan.h
    )
endmacro()

macro(run_vk_layer_generate subcmd output)
    add_custom_command(OUTPUT ${output}
        COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-layer-generate.py ${DisplayServer} ${subcmd} ${PROJECT_SOURCE_DIR}/include/vulkan/vulkan.h > ${output}
        DEPENDS ${PROJECT_SOURCE_DIR}/vk-layer-generate.py ${PROJECT_SOURCE_DIR}/include/vulkan/vulkan.h ${PROJECT_SOURCE_DIR}/vulkan.py
    )
endmacro()

macro(run_vk_layer_xml_generate subcmd output)
    add_custom_command(OUTPUT ${output}
        COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/genvk.py -registry ${PROJECT_SOURCE_DIR}/vk.xml ${output}
        DEPENDS ${PROJECT_SOURCE_DIR}/vk.xml ${PROJECT_SOURCE_DIR}/generator.py ${PROJECT_SOURCE_DIR}/genvk.py ${PROJECT_SOURCE_DIR}/reg.py
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
    VkLayer_device_limits
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
            endforeach(config_file)
        else()
            foreach (config_file ${LAYER_JSON_FILES})
                FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/windows/${config_file}.json src_json)
                FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${config_file}.json dst_json)
                add_custom_target(${config_file}-json ALL
                    COMMAND copy ${src_json} ${dst_json}
                    VERBATIM
                    )
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
        endforeach(config_file)
    endif()
endif()

if (WIN32)
    macro(add_vk_layer target)
    add_custom_command(OUTPUT VkLayer_${target}.def
        COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py ${DisplayServer} win-def-file VkLayer_${target} layer > VkLayer_${target}.def
        DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py
    )
    add_library(VkLayer_${target} SHARED ${ARGN} VkLayer_${target}.def)
    target_link_Libraries(VkLayer_${target} VkLayer_utils)
    add_dependencies(VkLayer_${target} generate_vk_layer_helpers)
    set_target_properties(VkLayer_${target} PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_BINARY_DIR}/VkLayer_${target}.def")
    endmacro()
else()
    macro(add_vk_layer target)
    add_library(VkLayer_${target} SHARED ${ARGN})
    target_link_Libraries(VkLayer_${target} VkLayer_utils)
    add_dependencies(VkLayer_${target} generate_vk_layer_helpers)
    set_target_properties(VkLayer_${target} PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")
    install(TARGETS VkLayer_${target} DESTINATION ${PROJECT_BINARY_DIR}/install_staging)
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
    set (CMAKE_CXX_FLAGS "-std=c++11")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpointer-arith")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")
endif()

add_custom_command(OUTPUT vk_dispatch_table_helper.h
    COMMAND ${PYTHON_CMD} ${PROJECT_SOURCE_DIR}/vk-generate.py ${DisplayServer} dispatch-table-ops layer > vk_dispatch_table_helper.h
    DEPENDS ${PROJECT_SOURCE_DIR}/vk-generate.py ${PROJECT_SOURCE_DIR}/vulkan.py)

run_vk_helper(gen_enum_string_helper vk_enum_string_helper.h)
run_vk_helper(gen_struct_wrappers
    vk_struct_string_helper.h
    vk_struct_string_helper_cpp.h
    vk_struct_string_helper_no_addr.h
    vk_struct_string_helper_no_addr_cpp.h
    vk_struct_size_helper.h
    vk_struct_size_helper.c
    vk_struct_wrappers.h
    vk_struct_wrappers.cpp
    vk_safe_struct.h
    vk_safe_struct.cpp
)

add_custom_target(generate_vk_layer_helpers DEPENDS
    vk_dispatch_table_helper.h
    vk_enum_string_helper.h
    vk_struct_string_helper.h
    vk_struct_string_helper_no_addr.h
    vk_struct_string_helper_cpp.h
    vk_struct_string_helper_no_addr_cpp.h
    vk_struct_size_helper.h
    vk_struct_size_helper.c
    vk_struct_wrappers.h
    vk_struct_wrappers.cpp
    vk_safe_struct.h
    vk_safe_struct.cpp
)

run_vk_layer_generate(object_tracker object_tracker.cpp)
run_vk_layer_xml_generate(Threading thread_check.h)
run_vk_layer_generate(unique_objects unique_objects.cpp)
run_vk_layer_xml_generate(ParamChecker parameter_validation.h)

# Layer Utils Library
# For Windows, we use a static lib because the Windows loader has a fairly restrictive loader search
# path that can't be easily modified to point it to the same directory that contains the layers.
if (WIN32)
    add_library(VkLayer_utils STATIC vk_layer_config.cpp vk_layer_extension_utils.cpp vk_layer_utils.cpp)
else()
    add_library(VkLayer_utils SHARED vk_layer_config.cpp vk_layer_extension_utils.cpp vk_layer_utils.cpp)
    install(TARGETS VkLayer_utils DESTINATION ${PROJECT_BINARY_DIR}/install_staging)
endif()

add_vk_layer(core_validation core_validation.cpp vk_layer_table.cpp vk_safe_struct.cpp descriptor_sets.cpp)
add_vk_layer(device_limits device_limits.cpp vk_layer_table.cpp)
add_vk_layer(image image.cpp vk_layer_table.cpp)
add_vk_layer(swapchain swapchain.cpp vk_layer_table.cpp)
# generated
add_vk_layer(object_tracker object_tracker.cpp vk_layer_table.cpp)
add_vk_layer(threading threading.cpp thread_check.h vk_layer_table.cpp)
add_vk_layer(unique_objects unique_objects.cpp vk_layer_table.cpp vk_safe_struct.cpp)
add_vk_layer(parameter_validation parameter_validation.cpp parameter_validation.h vk_layer_table.cpp)

# Core validation has additional dependencies
target_include_directories(VkLayer_core_validation PRIVATE ${GLSLANG_SPIRV_INCLUDE_DIR})
target_include_directories(VkLayer_core_validation PRIVATE ${SPIRV_TOOLS_INCLUDE_DIR})
target_link_libraries(VkLayer_core_validation ${SPIRV_TOOLS_LIBRARIES})
