# Create the i965 compiler library

# Mesa required defines
add_definitions(-D_GNU_SOURCE -DHAVE_PTHREAD -D__NOT_HAVE_DRM_H)
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
    -L${GLSLANG_BUILD}/install/lib
    -L${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/lib
    -L${LUNARGLASS_PREFIX}/build/Core
    -L${LUNARGLASS_PREFIX}/build/Frontends/glslang
    -L${LUNARGLASS_PREFIX}/build/Frontends/Bil
    -L${LUNARGLASS_PREFIX}/build/Core/Passes/Transforms
    -L${LUNARGLASS_PREFIX}/build/Core/Passes/Immutable
    -L${LUNARGLASS_PREFIX}/build/Core/Passes/Analysis
    -L${LUNARGLASS_PREFIX}/build/Core/Passes/Util
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

# Ensure we re-link if our external libraries change
# Every entry of COMPILER_LIBS should be included here
string(REPLACE "-l" "" LLVM_LIBS ${LLVM_LIBS_ALL})
separate_arguments(LLVM_LIBS)
foreach(LIB ${LLVM_LIBS})
    #message(STATUS "adding the following lib")
    #message(STATUS ${LIB})
    add_library(${LIB} STATIC IMPORTED)
    #message(STATUS "adding the following path")
    #message(STATUS ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/lib/lib${LIB}.a)
    set_target_properties(${LIB} PROPERTIES IMPORTED_LOCATION ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/lib/lib${LIB}.a)
endforeach(LIB)
add_library(glslangFrontend STATIC IMPORTED)
add_library(BilFrontend     STATIC IMPORTED)
add_library(core            STATIC IMPORTED)
add_library(LLVMipo         STATIC IMPORTED)
add_library(glslang         STATIC IMPORTED)
add_library(BIL             STATIC IMPORTED)
add_library(OGLCompiler     STATIC IMPORTED)
set_target_properties(glslangFrontend PROPERTIES IMPORTED_LOCATION ${LUNARGLASS_PREFIX}/build/Frontends/glslang/libglslangFrontend.a)
set_target_properties(BilFrontend     PROPERTIES IMPORTED_LOCATION ${LUNARGLASS_PREFIX}/build/Frontends/Bil/libBilFrontend.a)
set_target_properties(core            PROPERTIES IMPORTED_LOCATION ${LUNARGLASS_PREFIX}/build/Core/libcore.a)
set_target_properties(LLVMipo         PROPERTIES IMPORTED_LOCATION ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/lib/libLLVMipo.a)
set_target_properties(glslang         PROPERTIES IMPORTED_LOCATION ${GLSLANG_BUILD}/install/lib/libglslang.a)
set_target_properties(BIL             PROPERTIES IMPORTED_LOCATION ${GLSLANG_BUILD}/install/lib/libBIL.a)
set_target_properties(OGLCompiler     PROPERTIES IMPORTED_LOCATION ${GLSLANG_BUILD}/install/lib/libOGLCompiler.a)

SET(COMPILER_INCLUDE_DIRS
    ${GLSLANG_PREFIX}
    ${LUNARGLASS_PREFIX}/Core/LLVM/llvm-3.4/build/install/usr/local/include
    ${LUNARGLASS_PREFIX}
    shader
    mesa-utils/include
    mesa-utils/src
    mesa-utils/src/glsl
    mesa-utils/src/mesa
    mesa-utils/src/mesa/program
    mesa-utils/src/mapi
)

SET(COMPILER_SOURCES
    shader/ast_array_index.cpp
    shader/ast_expr.cpp
    shader/ast_function.cpp
    shader/ast_to_hir.cpp
    shader/ast_type.cpp
    shader/builtin_functions.cpp
    shader/builtin_types.cpp
    shader/builtin_variables.cpp
    shader/ir.cpp
    shader/ir_basic_block.cpp
    shader/ir_builder.cpp
    shader/ir_clone.cpp
    shader/ir_constant_expression.cpp
    shader/ir_deserializer.cpp
    shader/ir_equals.cpp
    shader/ir_expression_flattening.cpp
    shader/ir_function_can_inline.cpp
    shader/ir_function.cpp
    shader/ir_function_detect_recursion.cpp
    shader/ir_hierarchical_visitor.cpp
    shader/ir_hv_accept.cpp
    shader/ir_import_prototypes.cpp
    shader/ir_print_visitor.cpp
    shader/ir_reader.cpp
    shader/ir_rvalue_visitor.cpp
    shader/ir_serialize.cpp
    shader/ir_set_program_inouts.cpp
    shader/ir_validate.cpp
    shader/ir_variable_refcount.cpp
    shader/link_atomics.cpp
    shader/linker.cpp
    shader/link_functions.cpp
    shader/link_interface_blocks.cpp
    shader/link_uniform_block_active_visitor.cpp
    shader/link_uniform_blocks.cpp
    shader/link_uniform_initializers.cpp
    shader/link_uniforms.cpp
    shader/link_varyings.cpp
    shader/loop_analysis.cpp
    shader/loop_controls.cpp
    shader/loop_unroll.cpp
    shader/lower_clip_distance.cpp
    shader/lower_discard.cpp
    shader/lower_discard_flow.cpp
    shader/lower_if_to_cond_assign.cpp
    shader/lower_instructions.cpp
    shader/lower_jumps.cpp
    shader/lower_mat_op_to_vec.cpp
    shader/lower_named_interface_blocks.cpp
    shader/lower_noise.cpp
    shader/lower_offset_array.cpp
    shader/lower_output_reads.cpp
    shader/lower_packed_varyings.cpp
    shader/lower_packing_builtins.cpp
    shader/lower_texture_projection.cpp
    shader/lower_ubo_reference.cpp
    shader/lower_variable_index_to_cond_assign.cpp
    shader/lower_vec_index_to_cond_assign.cpp
    shader/lower_vec_index_to_swizzle.cpp
    shader/lower_vector.cpp
    shader/lower_vector_insert.cpp
    shader/opt_algebraic.cpp
    shader/opt_array_splitting.cpp
    shader/opt_constant_folding.cpp
    shader/opt_constant_propagation.cpp
    shader/opt_constant_variable.cpp
    shader/opt_copy_propagation.cpp
    shader/opt_copy_propagation_elements.cpp
    shader/opt_cse.cpp
    shader/opt_dead_builtin_varyings.cpp
    shader/opt_dead_code.cpp
    shader/opt_dead_code_local.cpp
    shader/opt_dead_functions.cpp
    shader/opt_flatten_nested_if_blocks.cpp
    shader/opt_flip_matrices.cpp
    shader/opt_function_inlining.cpp
    shader/opt_if_simplification.cpp
    shader/opt_noop_swizzle.cpp
    shader/opt_redundant_jumps.cpp
    shader/opt_structure_splitting.cpp
    shader/opt_swizzle_swizzle.cpp
    shader/opt_tree_grafting.cpp
    shader/opt_vectorize.cpp
    shader/s_expression.cpp
#    shader/shader_deserialize.cpp
#    shader/shader_serialize.cpp
#    shader/standalone_scaffolding.cpp
    shader/strtod.cpp

    mesa-utils/src/glsl/ralloc.c
    mesa-utils/src/mesa/program/program.c
#    mesa-utils/src/mesa/program/prog_execute.c
    # mesa-utils/src/mesa/program/prog_noise.c
    mesa-utils/src/mesa/program/prog_statevars.c
    # mesa-utils/src/mesa/program/prog_opt_constant_fold.c
    mesa-utils/src/mesa/program/symbol_table.c
#    mesa-utils/src/mesa/program/prog_cache.c
    mesa-utils/src/mesa/program/prog_instruction.c
    # mesa-utils/src/mesa/program/prog_optimize.c
    # mesa-utils/src/mesa/program/arbprogparse.c
    mesa-utils/src/mesa/program/prog_hash_table.c
    mesa-utils/src/mesa/program/prog_parameter.c
    # mesa-utils/src/mesa/program/prog_diskcache.c
    # mesa-utils/src/mesa/program/program_parse.tab.c
    # mesa-utils/src/mesa/program/programopt.c
    # mesa-utils/src/mesa/program/prog_print.c
    # mesa-utils/src/mesa/program/program_parse_extra.c
    # mesa-utils/src/mesa/program/prog_parameter_layout.c
     mesa-utils/src/mesa/program/register_allocate.c
    # mesa-utils/src/mesa/math/m_matrix.c
    # mesa-utils/src/mesa/main/enums.c
    # mesa-utils/src/mesa/main/imports.c
    mesa-utils/src/mesa/main/hash.c
    mesa-utils/src/mesa/main/hash_table.c
    # mesa-utils/src/mesa/main/errors.c
    # mesa-utils/src/mesa/main/formats.c

    mesa-utils/src/mesa/main/errors.c
    # mesa-utils/src/mesa/main/context.c
    mesa-utils/src/mesa/main/enums.c
    mesa-utils/src/mesa/main/imports.c
    mesa-utils/src/mesa/main/version.c
    mesa-utils/src/mesa/main/uniforms.c

    #mesa-utils/src/mesa/main/shaderobj.c

    mesa-utils/src/mesa/program/sampler.cpp

    shader/glsl_glass_manager.cpp
    shader/glsl_glass_backend_translator.cpp
    shader/glsl_glass_backend.cpp

    shader/glsl_parser_extras.cpp
    shader/standalone_scaffolding.cpp
    shader/glsl_types.cpp
    shader/glsl_symbol_table.cpp
    shader/hir_field_selection.cpp

    shader/compiler_interface.cpp

    # File required for backend compiler
    pipeline/pipeline_compiler_interface.cpp
    pipeline/pipeline_compiler_interface_meta.cpp
    pipeline/brw_blorp_blit_eu.cpp
    pipeline/brw_shader.cpp
    pipeline/brw_fs.cpp
    pipeline/brw_fs_visitor.cpp
    pipeline/brw_fs_live_variables.cpp
    pipeline/brw_cfg.cpp
    pipeline/brw_fs_cse.cpp
    pipeline/brw_fs_copy_propagation.cpp
    pipeline/brw_fs_peephole_predicated_break.cpp
    pipeline/brw_fs_dead_code_eliminate.cpp
    pipeline/brw_fs_sel_peephole.cpp
    pipeline/brw_dead_control_flow.cpp
    pipeline/brw_fs_saturate_propagation.cpp
    pipeline/brw_fs_register_coalesce.cpp
    pipeline/brw_schedule_instructions.cpp
    pipeline/brw_fs_reg_allocate.cpp
    pipeline/brw_fs_generator.cpp
    pipeline/brw_lower_texture_gradients.cpp
    pipeline/brw_cubemap_normalize.cpp
    pipeline/brw_lower_unnormalized_offset.cpp
    pipeline/brw_fs_channel_expressions.cpp
    pipeline/brw_fs_vector_splitting.cpp

    pipeline/brw_disasm.c
    pipeline/brw_device_info.c
    pipeline/brw_eu.c
    pipeline/brw_program.c
    pipeline/brw_wm.c
    pipeline/brw_eu_emit.c
    pipeline/brw_eu_compact.c
    pipeline/intel_debug.c

    pipeline/brw_vs.c
    pipeline/brw_vec4.cpp
    pipeline/brw_vec4_visitor.cpp
    pipeline/brw_vec4_vs_visitor.cpp
    pipeline/brw_vec4_live_variables.cpp
    pipeline/brw_vec4_copy_propagation.cpp
    pipeline/brw_vec4_reg_allocate.cpp
    pipeline/brw_vec4_generator.cpp
    # pipeline/gen8_vec4_generator.cpp
    )

set_source_files_properties(
	shader/glsl_glass_manager.cpp
	shader/glsl_glass_backend_translator.cpp
	shader/glsl_glass_backend.cpp
	shader/glsl_parser_extras.cpp
	PROPERTIES COMPILE_FLAGS "-Wno-unknown-pragmas -Wno-ignored-qualifiers")

SET(STANDALONE_COMPILER_SOURCES
    shader/main.cpp
    ${COMPILER_SOURCES}
)

# needed by libglslang.a
add_library(intelcompiler-os STATIC shader/ossource.cpp)
target_include_directories(intelcompiler-os PRIVATE ${COMPILER_INCLUDE_DIRS})
set_target_properties(intelcompiler-os PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(intelcompiler STATIC ${COMPILER_SOURCES})

target_include_directories(intelcompiler
    PRIVATE ${COMPILER_INCLUDE_DIRS}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(intelcompiler
    icd
    ${COMPILER_LINK_DIRS}
    ${COMPILER_LIBS}
    intelcompiler-os
    m
    pthread
    dl
)

set_target_properties(intelcompiler PROPERTIES POSITION_INDEPENDENT_CODE ON)

#add_executable(xglCompiler
#   ${STANDALONE_COMPILER_SOURCES}
#)
