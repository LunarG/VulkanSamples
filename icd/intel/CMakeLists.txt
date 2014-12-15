# Create the i965 XGL DRI library

# Mesa required defines
add_definitions(-D_GNU_SOURCE -DHAVE_PTHREAD)
# LLVM required defines
add_definitions(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)

# DEBUG and NDEBUG flags are important for proper mesa behavior
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

# LunarG TODO:  Get the llvm-config flags hooked up correctly and remove extra definitions from above

execute_process(COMMAND ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/bin/llvm-config --libs engine bitwriter
  OUTPUT_VARIABLE LLVM_LIBS_ALL_1
  RESULT_VARIABLE LLVM_LIBS_RESULT)

string(REPLACE "\n" "" LLVM_LIBS_ALL ${LLVM_LIBS_ALL_1})
message(STATUS "llvm-config lib results")
message(STATUS ${LLVM_LIBS_ALL})

if(NOT "${LLVM_LIBS_RESULT}" EQUAL "0")
  message(FATAL_ERROR "llvm-config failed: " ${LLVM_LIBS_RESULT})
endif()

# Expect libraries to be in either the build (release build) or dbuild (debug) directories
if(EXISTS ${GLSLANG_PREFIX}/build/install/lib)
    set(GLSLANG_BUILD ${GLSLANG_PREFIX}/build)
elseif(EXISTS ${GLSLANG_PREFIX}/dbuild/install/lib)
    set(GLSLANG_BUILD ${GLSLANG_PREFIX}/dbuild)
else()
    message(FATAL_ERROR "Necessary glslang libraries cannot be found: " ${GLSLANG_PREFIX})
endif()

if(EXISTS ${LUNARGLASS_PREFIX}/build/install/lib)
    set(LUNARGLASS_BUILD ${LUNARGLASS_PREFIX}/build)
elseif(EXISTS ${LUNARGLASS_PREFIX}/dbuild/install/lib)
    set(LUNARGLASS_BUILD ${LUNARGLASS_PREFIX}/dbuild)
else()
    message(FATAL_ERROR "Necessary LunarGLASS libraries cannot be found: " ${LUNARGLASS_PREFIX})
endif()

execute_process(COMMAND ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/bin/llvm-config --cxxflags
  OUTPUT_VARIABLE LLVM_CXX_CONFIG_ALL_1
  RESULT_VARIABLE LLVM_CXX_CONFIG_RESULT)

string(REPLACE "\n" "" LLVM_CXX_CONFIG_ALL ${LLVM_CXX_CONFIG_ALL_1})
string(REPLACE "-Woverloaded-virtual" "" LLVM_CXX_CONFIG_1 ${LLVM_CXX_CONFIG_ALL})
string(REPLACE "-fvisibility-inlines-hidden" "" LLVM_CXX_CONFIG ${LLVM_CXX_CONFIG_1})
message(STATUS "llvm-config cxxflags results")
message(STATUS ${LLVM_CXX_CONFIG})

# if(NOT "${LLVM_CXX_CONFIG_RESULT}" EQUAL "0")
#   message(FATAL_ERROR "llvm-config failed: " ${LLVM_CXX_CONFIG_RESULT})
# endif()


set_target_properties(icd
      PROPERTIES
      COMPILE_FLAGS "${LLVM_CXX_CONFIG}")

SET(COMPILER_LINK_DIRS
    ${GLSLANG_BUILD}/install/lib
    ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/lib
    ${LUNARGLASS_PREFIX}/build/Core
    ${LUNARGLASS_PREFIX}/build/Frontends/glslang
    ${LUNARGLASS_PREFIX}/build/Frontends/Bil
    ${LUNARGLASS_PREFIX}/build/Core/Passes/Transforms
    ${LUNARGLASS_PREFIX}/build/Core/Passes/Immutable
    ${LUNARGLASS_PREFIX}/build/Core/Passes/Analysis
    ${LUNARGLASS_PREFIX}/build/Core/Passes/Util
)

pkg_check_modules(DRM REQUIRED libdrm libdrm_intel)

link_directories (
    ${ICD_LIBRARY_DIRS}
    ${DRM_LIBRARY_DIRS}
    ${COMPILER_LINK_DIRS}
)

SET(COMPILER_LIBS
   glslangFrontend
   BilFrontend
   core
   LLVMipo
   glslang
   BIL
   OGLCompiler
   ${LLVM_LIBS_ALL}
)

link_libraries (
    ${ICD_LIBRARIES}
    ${DRM_LIBRARIES}
    ${COMPILER_LIBS}
    m
    pthread
    dl
)

SET(COMPILER_INCLUDE_DIRS
    ${GLSLANG_PREFIX}
    ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/include
    ${LUNARGLASS_PREFIX}
    ${CMAKE_CURRENT_SOURCE_DIR}
    compiler/shader
    compiler/mesa-utils/include
    compiler/mesa-utils/src
    compiler/mesa-utils/src/glsl
    compiler/mesa-utils/src/mesa
    compiler/mesa-utils/src/mesa/program
    compiler/mesa-utils/src/mapi
)

include_directories (
    ${ICD_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${COMPILER_INCLUDE_DIRS}
)


SET(COMPILER_SOURCES
    compiler/shader/ast_array_index.cpp
    compiler/shader/ast_expr.cpp
    compiler/shader/ast_function.cpp
    compiler/shader/ast_to_hir.cpp
    compiler/shader/ast_type.cpp
    compiler/shader/builtin_functions.cpp
    compiler/shader/builtin_types.cpp
    compiler/shader/builtin_variables.cpp
    compiler/shader/ir.cpp
    compiler/shader/ir_basic_block.cpp
    compiler/shader/ir_builder.cpp
    compiler/shader/ir_clone.cpp
    compiler/shader/ir_constant_expression.cpp
    compiler/shader/ir_deserializer.cpp
    compiler/shader/ir_equals.cpp
    compiler/shader/ir_expression_flattening.cpp
    compiler/shader/ir_function_can_inline.cpp
    compiler/shader/ir_function.cpp
    compiler/shader/ir_function_detect_recursion.cpp
    compiler/shader/ir_hierarchical_visitor.cpp
    compiler/shader/ir_hv_accept.cpp
    compiler/shader/ir_import_prototypes.cpp
    compiler/shader/ir_print_visitor.cpp
    compiler/shader/ir_reader.cpp
    compiler/shader/ir_rvalue_visitor.cpp
    compiler/shader/ir_serialize.cpp
    compiler/shader/ir_set_program_inouts.cpp
    compiler/shader/ir_validate.cpp
    compiler/shader/ir_variable_refcount.cpp
    compiler/shader/link_atomics.cpp
    compiler/shader/linker.cpp
    compiler/shader/link_functions.cpp
    compiler/shader/link_interface_blocks.cpp
    compiler/shader/link_uniform_block_active_visitor.cpp
    compiler/shader/link_uniform_blocks.cpp
    compiler/shader/link_uniform_initializers.cpp
    compiler/shader/link_uniforms.cpp
    compiler/shader/link_varyings.cpp
    compiler/shader/loop_analysis.cpp
    compiler/shader/loop_controls.cpp
    compiler/shader/loop_unroll.cpp
    compiler/shader/lower_clip_distance.cpp
    compiler/shader/lower_discard.cpp
    compiler/shader/lower_discard_flow.cpp
    compiler/shader/lower_if_to_cond_assign.cpp
    compiler/shader/lower_instructions.cpp
    compiler/shader/lower_jumps.cpp
    compiler/shader/lower_mat_op_to_vec.cpp
    compiler/shader/lower_named_interface_blocks.cpp
    compiler/shader/lower_noise.cpp
    compiler/shader/lower_offset_array.cpp
    compiler/shader/lower_output_reads.cpp
    compiler/shader/lower_packed_varyings.cpp
    compiler/shader/lower_packing_builtins.cpp
    compiler/shader/lower_texture_projection.cpp
    compiler/shader/lower_ubo_reference.cpp
    compiler/shader/lower_variable_index_to_cond_assign.cpp
    compiler/shader/lower_vec_index_to_cond_assign.cpp
    compiler/shader/lower_vec_index_to_swizzle.cpp
    compiler/shader/lower_vector.cpp
    compiler/shader/lower_vector_insert.cpp
    compiler/shader/opt_algebraic.cpp
    compiler/shader/opt_array_splitting.cpp
    compiler/shader/opt_constant_folding.cpp
    compiler/shader/opt_constant_propagation.cpp
    compiler/shader/opt_constant_variable.cpp
    compiler/shader/opt_copy_propagation.cpp
    compiler/shader/opt_copy_propagation_elements.cpp
    compiler/shader/opt_cse.cpp
    compiler/shader/opt_dead_builtin_varyings.cpp
    compiler/shader/opt_dead_code.cpp
    compiler/shader/opt_dead_code_local.cpp
    compiler/shader/opt_dead_functions.cpp
    compiler/shader/opt_flatten_nested_if_blocks.cpp
    compiler/shader/opt_flip_matrices.cpp
    compiler/shader/opt_function_inlining.cpp
    compiler/shader/opt_if_simplification.cpp
    compiler/shader/opt_noop_swizzle.cpp
    compiler/shader/opt_redundant_jumps.cpp
    compiler/shader/opt_structure_splitting.cpp
    compiler/shader/opt_swizzle_swizzle.cpp
    compiler/shader/opt_tree_grafting.cpp
    compiler/shader/opt_vectorize.cpp
    compiler/shader/s_expression.cpp
#    compiler/shader/shader_deserialize.cpp
#    compiler/shader/shader_serialize.cpp
#    compiler/shader/standalone_scaffolding.cpp
    compiler/shader/strtod.cpp

    compiler/mesa-utils/src/glsl/ralloc.c
    compiler/mesa-utils/src/mesa/program/program.c
#    compiler/mesa-utils/src/mesa/program/prog_execute.c
    # compiler/mesa-utils/src/mesa/program/prog_noise.c
    compiler/mesa-utils/src/mesa/program/prog_statevars.c
    # compiler/mesa-utils/src/mesa/program/prog_opt_constant_fold.c
    compiler/mesa-utils/src/mesa/program/symbol_table.c
#    compiler/mesa-utils/src/mesa/program/prog_cache.c
    compiler/mesa-utils/src/mesa/program/prog_instruction.c
    # compiler/mesa-utils/src/mesa/program/prog_optimize.c
    # compiler/mesa-utils/src/mesa/program/arbprogparse.c
    compiler/mesa-utils/src/mesa/program/prog_hash_table.c
    compiler/mesa-utils/src/mesa/program/prog_parameter.c
    # compiler/mesa-utils/src/mesa/program/prog_diskcache.c
    # compiler/mesa-utils/src/mesa/program/program_parse.tab.c
    # compiler/mesa-utils/src/mesa/program/programopt.c
    # compiler/mesa-utils/src/mesa/program/prog_print.c
    # compiler/mesa-utils/src/mesa/program/program_parse_extra.c
    # compiler/mesa-utils/src/mesa/program/prog_parameter_layout.c
     compiler/mesa-utils/src/mesa/program/register_allocate.c
    # compiler/mesa-utils/src/mesa/math/m_matrix.c
    # compiler/mesa-utils/src/mesa/main/enums.c
    # compiler/mesa-utils/src/mesa/main/imports.c
    compiler/mesa-utils/src/mesa/main/hash.c
    compiler/mesa-utils/src/mesa/main/hash_table.c
    # compiler/mesa-utils/src/mesa/main/errors.c
    # compiler/mesa-utils/src/mesa/main/formats.c

    compiler/mesa-utils/src/mesa/main/errors.c
    # compiler/mesa-utils/src/mesa/main/context.c
    compiler/mesa-utils/src/mesa/main/enums.c
    compiler/mesa-utils/src/mesa/main/imports.c
    compiler/mesa-utils/src/mesa/main/version.c
    compiler/mesa-utils/src/mesa/main/uniforms.c

    #compiler/mesa-utils/src/mesa/main/shaderobj.c

    compiler/mesa-utils/src/mesa/program/sampler.cpp

    compiler/shader/glsl_glass_manager.cpp
    compiler/shader/glsl_glass_backend_translator.cpp
    compiler/shader/glsl_glass_backend.cpp

    compiler/shader/glsl_parser_extras.cpp
    compiler/shader/ossource.cpp
    compiler/shader/standalone_scaffolding.cpp
    compiler/shader/glsl_types.cpp
    compiler/shader/glsl_symbol_table.cpp
    compiler/shader/hir_field_selection.cpp

    compiler/shader/compiler_interface.cpp

    # File required for backend compiler
    compiler/pipeline/pipeline_compiler_interface.cpp
    compiler/pipeline/pipeline_compiler_interface_meta.cpp
    compiler/pipeline/brw_blorp_blit_eu.cpp
    compiler/pipeline/brw_shader.cpp
    compiler/pipeline/brw_fs.cpp
    compiler/pipeline/brw_fs_visitor.cpp
    compiler/pipeline/brw_fs_live_variables.cpp
    compiler/pipeline/brw_cfg.cpp
    compiler/pipeline/brw_fs_cse.cpp
    compiler/pipeline/brw_fs_copy_propagation.cpp
    compiler/pipeline/brw_fs_peephole_predicated_break.cpp
    compiler/pipeline/brw_fs_dead_code_eliminate.cpp
    compiler/pipeline/brw_fs_sel_peephole.cpp
    compiler/pipeline/brw_dead_control_flow.cpp
    compiler/pipeline/brw_fs_saturate_propagation.cpp
    compiler/pipeline/brw_fs_register_coalesce.cpp
    compiler/pipeline/brw_schedule_instructions.cpp
    compiler/pipeline/brw_fs_reg_allocate.cpp
    compiler/pipeline/brw_fs_generator.cpp
    compiler/pipeline/brw_lower_texture_gradients.cpp
    compiler/pipeline/brw_cubemap_normalize.cpp
    compiler/pipeline/brw_lower_unnormalized_offset.cpp
    compiler/pipeline/brw_fs_channel_expressions.cpp
    compiler/pipeline/brw_fs_vector_splitting.cpp

    compiler/pipeline/brw_disasm.c
    compiler/pipeline/brw_device_info.c
    compiler/pipeline/brw_eu.c
    compiler/pipeline/brw_program.c
    compiler/pipeline/brw_wm.c
    compiler/pipeline/brw_eu_emit.c
    compiler/pipeline/brw_eu_compact.c
    compiler/pipeline/intel_debug.c

    compiler/pipeline/brw_vs.c
    compiler/pipeline/brw_vec4.cpp
    compiler/pipeline/brw_vec4_visitor.cpp
    compiler/pipeline/brw_vec4_vs_visitor.cpp
    compiler/pipeline/brw_vec4_live_variables.cpp
    compiler/pipeline/brw_vec4_copy_propagation.cpp
    compiler/pipeline/brw_vec4_reg_allocate.cpp
    compiler/pipeline/brw_vec4_generator.cpp
    compiler/pipeline/gen8_vec4_generator.cpp
    )

set_source_files_properties(
	compiler/shader/glsl_glass_manager.cpp
	compiler/shader/glsl_glass_backend_translator.cpp
	compiler/shader/glsl_glass_backend.cpp
	compiler/shader/glsl_parser_extras.cpp
	PROPERTIES COMPILE_FLAGS "-Wno-unknown-pragmas -Wno-ignored-qualifiers")

SET(STANDALONE_COMPILER_SOURCES
    compiler/shader/main.cpp
    ${COMPILER_SOURCES}
)

SET(SOURCES 
    cmd.c
    cmd_decode.c
    cmd_meta.c
    cmd_mi.c
    cmd_prepare.c
    cmd_pipeline.c
    dev.c
    dispatch.c
    dset.c
    intel.c
    event.c
    fence.c
    format.c
    gpu.c
    img.c
    layout.c
    mem.c
    obj.c
    pipeline.c
    pipeline_shader.c
    query.c
    queue.c
    sampler.c
    shader.c
    state.c
    view.c
    kmd/winsys_drm.c
    ${COMPILER_SOURCES}
    )

pkg_check_modules(DRI3 xcb xcb-dri3 xcb-present)
if (DRI3_FOUND)
	add_definitions(-DENABLE_WSI_X11)
	include_directories(${DRI3_INCLUDE_DIRS})
	link_directories(${DRI3_LIBRARY_DIRS})
	link_libraries(${DRI3_LIBRARIES})
	set(SOURCES ${SOURCES} wsi_x11.c)
endif()

add_library(XGL_i965 SHARED
   ${SOURCES}
   $<TARGET_OBJECTS:icd>
   )

#add_executable(xglCompiler
#   ${STANDALONE_COMPILER_SOURCES}
#)
