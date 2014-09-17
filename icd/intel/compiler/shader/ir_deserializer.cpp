/* -*- c++ -*- */
/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "ir_deserializer.h"

/**
 * Searches for ir_function with matching signature from exec_list.
 */
static ir_function *
search_func(struct _mesa_glsl_parse_state *state, struct exec_list *list,
            const char *name, struct exec_list *parameters)
{
   foreach_list_safe(node, list) {
      ir_function *func = ((ir_instruction *) node)->as_function();
      if (func && strcmp(name, func->name) == 0 &&
         func->matching_signature(state, parameters))
         return func;
   }
   return NULL;
}


/**
 * Helper function to read a list of instructions.
 */
bool
ir_deserializer::deserialize_list(exec_list *list)
{
   uint32_t list_len = map->read_uint32_t();
   for (unsigned k = 0; k < list_len; k++)
      if (!read_instruction(list))
         return false;
   return true;
}


ir_instruction *
ir_deserializer::read_ir_variable()
{
   const glsl_type *type = deserialize_glsl_type(map, state, type_ht);

   /* TODO: Understand how this can happen and fix */
   if (type == glsl_type::error_type)
      return NULL;

   char *name = map->read_string();
   int64_t unique_id = map->read_int64_t();
   uint8_t mode = map->read_uint8_t();

   ir_variable *var =
      new(mem_ctx) ir_variable(type, name, (ir_variable_mode) mode);

   if (!var)
      return NULL;

   map->read(&var->data, sizeof(var->data));

   var->num_state_slots = map->read_uint32_t();
   uint8_t has_constant_value = map->read_uint8_t();
   uint8_t has_constant_initializer = map->read_uint8_t();

   var->state_slots = NULL;

   if (var->num_state_slots > 0) {

      /* Validate num_state_slots against defined maximum. */
      if (var->num_state_slots > MAX_NUM_STATE_SLOTS)
         return NULL;

      var->state_slots = ralloc_array(var, ir_state_slot, var->num_state_slots);

      for (unsigned i = 0; i < var->num_state_slots; i++) {
         var->state_slots[i].swizzle = map->read_int32_t();
         for (int j = 0; j < 5; j++) {
            var->state_slots[i].tokens[j] = map->read_int32_t();
         }
      }
   }

   if (has_constant_value)
      var->constant_value = read_ir_constant();

   if (has_constant_initializer)
      var->constant_initializer = read_ir_constant();

   uint8_t has_interface_type = map->read_uint8_t();

   if (has_interface_type && (var->get_interface_type() == NULL))
      var->init_interface_type(deserialize_glsl_type(map, state, type_ht));
   /**
    * Store address to this variable so that variable
    * dereference readers can find it later.
    */
   _mesa_hash_table_insert(var_ht, hash_value,
                           (void*) (intptr_t) unique_id, var);
   return var;
}


ir_instruction *
ir_deserializer::read_ir_function(bool prototypes_only)
{
   char *name = map->read_string();
   uint32_t num_signatures = map->read_uint32_t();

   ir_function *f = new(mem_ctx) ir_function(name);
   ir_function_signature *sig = NULL;
   uint32_t next_signature = 0;

   /* Add all signatures to the function. */
   for (unsigned j = 0; j < num_signatures; j++) {

      if (prototypes_only && next_signature) {
         /* We're on our >1 iteration of the loop so
          * we have more than one signature for this function
          * and must skip ahead to the next signature.
          */
         map->jump(next_signature);
         continue;
      }

      /* Type equals ir_function_signature. */
      uint8_t ir_type = map->read_uint8_t();
      uint32_t len = map->read_uint32_t();

      /* The next sig, if there is one, follows the entire function, not
       * just the parameters.
       */
      if (prototypes_only)
         next_signature = map->position() + len;

      /* Used for debugging. */
      (void) ir_type;

      assert(ir_type == ir_type_function_signature);

      uint8_t is_builtin = map->read_uint8_t();

      uint8_t is_defined = map->read_uint8_t();

      const glsl_type *return_type = deserialize_glsl_type(map, state, type_ht);
      if (!return_type)
         return NULL;

      sig = new(mem_ctx) ir_function_signature(return_type);

      /* is_defined should be true if original was, even if func is empty */
      sig->is_defined = is_defined;

      /* Fill function signature parameters. */
      if (!deserialize_list(&sig->parameters))
         return NULL;

      /* Fill instructions for the function body. */
      if (!prototypes_only) {
         uint32_t body_count = map->read_uint32_t();
         for (unsigned k = 0; k < body_count; k++)
            if (!read_instruction(&sig->body, is_builtin ? true : false))
               return NULL;
      }

      if (!is_builtin) {
         f->add_signature(sig);
      } else {
         ir_function_signature *builtin_sig =
            _mesa_glsl_find_builtin_function(state, name, &sig->parameters);

         if (!builtin_sig)
            return NULL;

         f->add_signature(sig);
      }

      /* Break out of the loop if read errors occured. */
      if (map->errors())
         return NULL;

   } /* For each function signature. */

   return f;
}


/* Reads in type and package length, compares type to expected type. */
#define VALIDATE_RVALUE(node_type)\
   uint8_t ir_type = map->read_uint8_t();\
   uint32_t len = map->read_uint32_t();\
   (void) len;\
   if (ir_type != node_type)\
      return NULL;\


ir_dereference_array *
ir_deserializer::read_ir_dereference_array()
{
   VALIDATE_RVALUE(ir_type_dereference_array);

   ir_rvalue *array_rval = read_ir_rvalue();
   ir_rvalue *index_rval = read_ir_rvalue();

   if (array_rval && index_rval)
      return new(mem_ctx) ir_dereference_array(array_rval, index_rval);

   return NULL;
}


ir_dereference_record *
ir_deserializer::read_ir_dereference_record()
{
   VALIDATE_RVALUE(ir_type_dereference_record);

   char *name = map->read_string();

   ir_rvalue *rval = read_ir_rvalue();

   if (rval)
      return new(mem_ctx) ir_dereference_record(rval, name);

   return NULL;
}


/**
 * Reads in a variable deref, seeks variable address
 * from a map with it's unique_id.
 */
ir_dereference_variable *
ir_deserializer::read_ir_dereference_variable()
{
   VALIDATE_RVALUE(ir_type_dereference_variable);

   int64_t unique_id = map->read_int64_t();

   hash_entry *entry = _mesa_hash_table_search(var_ht, hash_value,
                                               (void*) (intptr_t) unique_id);
   if (!entry)
      return NULL;

   return new(mem_ctx) ir_dereference_variable((ir_variable*) entry->data);
}


ir_constant *
ir_deserializer::read_ir_constant()
{
   VALIDATE_RVALUE(ir_type_constant);

   const glsl_type *constant_type = deserialize_glsl_type(map, state, type_ht);

   /* TODO: Understand how this can happen and fix */
   if (constant_type == glsl_type::error_type)
      return NULL;

   ir_constant_data data;
   map->read(&data, sizeof(data));

   /* Array of constants. */
   if (constant_type->base_type == GLSL_TYPE_ARRAY) {

      struct exec_list elements;
      for (unsigned i = 0; i < constant_type->length; i++) {

         ir_constant *con = read_ir_constant();

         /* TODO: Understand how this can happen and fix */
         if (!con)
            return NULL;

         /* Break out of the loop if read errors occured. */
         if (map->errors())
            return NULL;

         elements.push_tail(con);
      }

      return new(mem_ctx) ir_constant(constant_type, &elements);

   } else if (constant_type->base_type == GLSL_TYPE_STRUCT) {
      struct exec_list elements;
      if (!deserialize_list(&elements))
         return NULL;

      return new(mem_ctx) ir_constant(constant_type, &elements);
   }

   return new(mem_ctx) ir_constant(constant_type, &data);
}


ir_swizzle *
ir_deserializer::read_ir_swizzle()
{
   VALIDATE_RVALUE(ir_type_swizzle);

   struct ir_swizzle_mask mask;
   map->read(&mask, sizeof(ir_swizzle_mask));

   ir_rvalue *rval = read_ir_rvalue();
   if (rval)
      return new(mem_ctx) ir_swizzle(rval, mask.x, mask.y, mask.z, mask.w,
                                     mask.num_components);
   return NULL;
}


ir_texture *
ir_deserializer::read_ir_texture()
{
   VALIDATE_RVALUE(ir_type_texture);

   int32_t op = map->read_int32_t();

   ir_texture *new_tex = new(mem_ctx) ir_texture((ir_texture_opcode) op);

   if (!new_tex)
      return NULL;

   const glsl_type *type = deserialize_glsl_type(map, state, type_ht);

   ir_dereference *sampler = (ir_dereference *) read_ir_rvalue();

   if (!sampler || !type)
      return NULL;

   new_tex->set_sampler(sampler, type);

   new_tex->coordinate = read_ir_rvalue();
   new_tex->projector = read_ir_rvalue();
   new_tex->shadow_comparitor = read_ir_rvalue();
   new_tex->offset = read_ir_rvalue();

   memset(&new_tex->lod_info, 0, sizeof(ir_texture::lod_info));

   new_tex->lod_info.lod = read_ir_rvalue();
   new_tex->lod_info.bias = read_ir_rvalue();
   new_tex->lod_info.sample_index = read_ir_rvalue();
   new_tex->lod_info.component = read_ir_rvalue();
   new_tex->lod_info.grad.dPdx = read_ir_rvalue();
   new_tex->lod_info.grad.dPdy = read_ir_rvalue();

   return new_tex;
}


ir_expression *
ir_deserializer::read_ir_expression()
{
   VALIDATE_RVALUE(ir_type_expression);

   /* Type resulting from the operation. */
   const glsl_type *rval_type = deserialize_glsl_type(map, state, type_ht);

   /* Read operation type + all operands for creating ir_expression. */
   uint32_t operation = map->read_uint32_t();
   uint32_t operands = map->read_int32_t();

   ir_rvalue *ir_rvalue_table[4] = { NULL };
   for (unsigned k = 0; k < operands; k++) {
      ir_rvalue_table[k] = read_ir_rvalue();
      if (!ir_rvalue_table[k])
         return NULL;
   }

   return new(mem_ctx) ir_expression(operation,
      rval_type,
      ir_rvalue_table[0],
      ir_rvalue_table[1],
      ir_rvalue_table[2],
      ir_rvalue_table[3]);
}


ir_rvalue *
ir_deserializer::read_ir_rvalue()
{
   uint8_t ir_type = map->read_uint8_t();

   switch(ir_type) {
   case ir_type_constant:
      return read_ir_constant();
   case ir_type_dereference_variable:
      return read_ir_dereference_variable();
   case ir_type_dereference_record:
      return read_ir_dereference_record();
   case ir_type_dereference_array:
      return read_ir_dereference_array();
   case ir_type_expression:
      return read_ir_expression();
   case ir_type_swizzle:
      return read_ir_swizzle();
   case ir_type_texture:
      return read_ir_texture();
   /* type is ir_type_unset ir rvalue is set to NULL */
   case ir_type_unset:
   default:
      return NULL;
   }
}


ir_instruction *
ir_deserializer::read_ir_assignment()
{
   uint32_t write_mask = map->read_uint8_t();

   ir_dereference *lhs_deref = (ir_dereference *) read_ir_rvalue();

   if (!lhs_deref)
      return NULL;

   ir_rvalue *cond = read_ir_rvalue();
   ir_rvalue *rval = read_ir_rvalue();

   if (rval)
      return new(mem_ctx) ir_assignment(lhs_deref, rval, cond, write_mask);

   return NULL;
}


ir_instruction *
ir_deserializer::read_ir_if()
{
   ir_rvalue *cond = read_ir_rvalue();
   /* TODO: Understand how this can happen and fix */
   if (!cond)
      return NULL;

   ir_if *irif = new(mem_ctx) ir_if(cond);
   if (!irif)
      return NULL;

   if (!deserialize_list(&irif->then_instructions))
      return NULL;
   if (!deserialize_list(&irif->else_instructions))
      return NULL;

   return irif;
}


ir_instruction *
ir_deserializer::read_ir_return()
{
   return new(mem_ctx) ir_return(read_ir_rvalue());
}


/**
 * Read a call to ir_function, finds the correct function
 * signature from prototypes list and creates the call.
 */
ir_instruction *
ir_deserializer::read_ir_call()
{
   struct exec_list parameters;

   char *name = map->read_string();

   ir_dereference_variable *return_deref = (ir_dereference_variable *) read_ir_rvalue();

   uint8_t list_len = map->read_uint8_t();

   /* read call parameters */
   for (unsigned k = 0; k < list_len; k++) {
      ir_rvalue *rval = read_ir_rvalue();

      if (!rval)
         return NULL;

      parameters.push_tail(rval);
   }

   uint8_t use_builtin = map->read_uint8_t();

   if (use_builtin) {
      ir_function_signature *builtin_sig =
         _mesa_glsl_find_builtin_function(state, name, &parameters);

      if (!builtin_sig)
         return NULL;

      ir_call *call = new(mem_ctx) ir_call(builtin_sig, return_deref,
                                           &parameters);
      call->use_builtin = true;
      return call;
   }

   /* Find the function from the prototypes. */
   ir_function *func = search_func(state, prototypes, name, &parameters);
   if (func)
      return new(mem_ctx) ir_call(func->matching_signature(state, &parameters),
                                  return_deref, &parameters);

   return NULL;
}


ir_instruction *
ir_deserializer::read_ir_discard()
{
   return new(mem_ctx) ir_discard(read_ir_rvalue());
}


ir_instruction *
ir_deserializer::read_ir_loop()
{
   ir_loop *loop = new(mem_ctx) ir_loop;

   if (!deserialize_list(&loop->body_instructions))
      return NULL;

   return loop;
}


ir_instruction *
ir_deserializer::read_ir_loop_jump()
{
   uint8_t mode = map->read_uint8_t();
   return  new(mem_ctx) ir_loop_jump((ir_loop_jump::jump_mode) mode);
}


ir_instruction *
ir_deserializer::read_emit_vertex()
{
   return new(mem_ctx) ir_emit_vertex;
}


ir_instruction *
ir_deserializer::read_end_primitive()
{
   return new(mem_ctx) ir_end_primitive;
}


bool
ir_deserializer::read_instruction(struct exec_list *list, bool ignore)
{
   uint8_t ir_type = map->read_uint8_t();
   uint32_t inst_dumpsize = map->read_uint32_t();

   /* Reader wants to jump over this instruction. */
   if (ignore) {
      map->ffwd(inst_dumpsize);
      return true;
   }

   ir_instruction *ir;

   switch(ir_type) {
   case ir_type_variable:
      ir = read_ir_variable();
      break;
   case ir_type_assignment:
      ir = read_ir_assignment();
      break;
   case ir_type_constant:
      ir = read_ir_constant();
      break;
   case ir_type_function:
      ir = read_ir_function();
      break;
   case ir_type_if:
      ir = read_ir_if();
      break;
   case ir_type_return:
      ir = read_ir_return();
      break;
   case ir_type_call:
      ir = read_ir_call();
      break;
   case ir_type_discard:
      ir = read_ir_discard();
      break;
   case ir_type_loop:
      ir = read_ir_loop();
      break;
   case ir_type_loop_jump:
      ir = read_ir_loop_jump();
      break;
   case ir_type_emit_vertex:
      ir = read_emit_vertex();
      break;
   case ir_type_end_primitive:
      ir = read_end_primitive();
      break;
   default:
      if (ir_type <= ir_type_unset || ir_type >= ir_type_max)
         assert(!"read invalid ir_type during IR deserialization\n");

      return false;
   }

   if (!ir || map->errors())
      return false;

   list->push_tail(ir);
   return true;
}


/**
 * Go through the blob and read prototypes for the functions.
 */
bool
ir_deserializer::read_prototypes(unsigned list_len)
{
   uint32_t ir_start = map->position();

   for (unsigned k = 0; k < list_len; k++) {
      uint8_t ir_type = map->read_uint8_t();
      uint32_t len = map->read_uint32_t();

      if (len > (map->size() - map->position())) {
         /* We've run off the end for some reason
          * hopefully we got the protos we need
          * move on!
          * TODO: Understand how this can happen and fix
          */
         break;
      }

      if (ir_type != ir_type_function) {
         map->ffwd(len);
         continue;
      }

      ir_instruction *func = read_ir_function(true);
      if (!func)
         return false;

      prototypes->push_tail(func);

      /* Break out of the loop if read errors occured. */
      if (map->errors())
         return false;
   }

   map->jump(ir_start);
   return true;
}


/**
 * Enable all glsl extensions for the parse state. Patch to pack
 * enable bits in a struct was not accepted by upstream so we need
 * to enable each individual bit like this.
 */
static void
enable_glsl_extensions(struct _mesa_glsl_parse_state *state)
{
   state->ARB_arrays_of_arrays_enable = true;
   state->ARB_compute_shader_enable = true;
   state->ARB_conservative_depth_enable = true;
   state->ARB_draw_buffers_enable = true;
   state->ARB_draw_instanced_enable = true;
   state->ARB_explicit_attrib_location_enable = true;
   state->ARB_fragment_coord_conventions_enable = true;
   state->ARB_gpu_shader5_enable = true;
   state->ARB_sample_shading_enable = true;
   state->ARB_separate_shader_objects_enable = true;
   state->ARB_shader_atomic_counters_enable = true;
   state->ARB_shader_bit_encoding_enable = true;
   state->ARB_shader_image_load_store_enable = true;
   state->ARB_shader_stencil_export_enable = true;
   state->ARB_shader_texture_lod_enable = true;
   state->ARB_shading_language_420pack_enable = true;
   state->ARB_shading_language_packing_enable = true;
   state->ARB_texture_cube_map_array_enable = true;
   state->ARB_texture_gather_enable = true;
   state->ARB_texture_multisample_enable = true;
   state->ARB_texture_query_levels_enable = true;
   state->ARB_texture_query_lod_enable = true;
   state->ARB_texture_rectangle_enable = true;
   state->ARB_uniform_buffer_object_enable = true;
   state->ARB_viewport_array_enable = true;

   state->OES_EGL_image_external_enable = true;
   state->OES_standard_derivatives_enable = true;
   state->OES_texture_3D_enable = true;

   state->AMD_conservative_depth_enable = true;
   state->AMD_shader_stencil_export_enable = true;
   state->AMD_shader_trinary_minmax_enable = true;
   state->AMD_vertex_shader_layer_enable = true;
   state->EXT_separate_shader_objects_enable = true;
   state->EXT_shader_integer_mix_enable = true;
   state->EXT_texture_array_enable = true;
}


bool
ir_deserializer::deserialize(struct gl_context *ctx, void *mem_ctx, gl_shader *shader, memory_map *map)
{
   assert(ctx);
   bool success = false;
   uint32_t exec_list_len;

   /* Allocations use passed ralloc ctx during reading. */
   this->mem_ctx = mem_ctx;
   this->map = map;

   /* Parse state is used to find builtin functions and existing types during
    * reading. We enable all the extensions to have all possible builtin types
    * and functions available.
    */
   state = new(mem_ctx)
      _mesa_glsl_parse_state(ctx,
                            _mesa_shader_enum_to_shader_stage(shader->Type),
                             shader);

   enable_glsl_extensions(state);

   state->language_version = shader->Version;
   state->uses_builtin_functions = true;
   _mesa_glsl_initialize_builtin_functions();
   _mesa_glsl_initialize_types(state);

   prototypes = new(mem_ctx) exec_list;
   shader->ir = new(shader) exec_list;

   exec_list_len = map->read_uint32_t();

   success = read_prototypes(exec_list_len);

   /* Top level exec_list read loop, constructs a new list. */
   while(exec_list_len && success == true) {
      success = read_instruction(shader->ir);
      exec_list_len--;
   }

   ralloc_free(prototypes);
   ralloc_free(state);

   if (!success)
      goto error_deserialize;

   shader->CompileStatus = GL_TRUE;

   /* Allocates glsl_symbol_table internally. */
   populate_symbol_table(shader);

   validate_ir_tree(shader->ir);

error_deserialize:
   return success;
}
