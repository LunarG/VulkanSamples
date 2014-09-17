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

#pragma once
#ifndef IR_CACHE_DESERIALIZER_H
#define IR_CACHE_DESERIALIZER_H

#include "program/program.h"
#include "glsl_parser_extras.h"
#include "main/shaderobj.h"
#include "linker.h"
#include "memory_map.h"

#ifdef __cplusplus

/**
 * Class to deserialize gl_shader IR from a binary data blob
 *
 * Deserialization is done with a help of memory_map class that takes care
 * of actual data reading. Class fills existing gl_shader's ir exec_list
 * with IR instructions from the binary blob.
 */
class ir_deserializer
{
public:
   ir_deserializer() :
      state(NULL),
      mem_ctx(NULL),
      map(NULL)
   {
      var_ht = _mesa_hash_table_create(0, int_equal);
      type_ht = _mesa_hash_table_create(0, int_equal);
      hash_value = _mesa_hash_data(this, sizeof(ir_deserializer));
   }

   ~ir_deserializer()
   {
      _mesa_hash_table_destroy(var_ht, NULL);
      _mesa_hash_table_destroy(type_ht, NULL);
   }

   /* deserialize IR to gl_shader from mapped memory */
   bool deserialize(struct gl_context *ctx, void *mem_ctx, gl_shader *shader, memory_map *map);

private:

   struct _mesa_glsl_parse_state *state;
   void *mem_ctx;
   memory_map *map;

   /* pointer to list which contains prototypes of functions */
   struct exec_list *prototypes;

   bool read_prototypes(unsigned list_len);
   bool read_instruction(struct exec_list *list, bool ignore = false);
   bool deserialize_list(struct exec_list *list);

   ir_instruction *read_ir_variable();
   ir_instruction *read_ir_assignment();
   ir_instruction *read_ir_function(bool prototypes_only = false);
   ir_instruction *read_ir_if();
   ir_instruction *read_ir_return();
   ir_instruction *read_ir_call();
   ir_instruction *read_ir_discard();
   ir_instruction *read_ir_loop();
   ir_instruction *read_ir_loop_jump();
   ir_instruction *read_emit_vertex();
   ir_instruction *read_end_primitive();

   /* rvalue readers */
   ir_rvalue *read_ir_rvalue();
   ir_constant *read_ir_constant();
   ir_swizzle *read_ir_swizzle();
   ir_texture *read_ir_texture();
   ir_expression *read_ir_expression();
   ir_dereference_array *read_ir_dereference_array();
   ir_dereference_record *read_ir_dereference_record();
   ir_dereference_variable *read_ir_dereference_variable();

   /**
    * var_ht is used to store created ir_variables with a unique_key for
    * each so that ir_dereference_variable creation can find the variable
    */
   struct hash_table *var_ht;
   uint32_t hash_value;

   /* to read user types only once */
   struct hash_table *type_ht;

   static bool int_equal(const void *a, const void *b)
   {
      return a == b;
   }

};


#endif /* ifdef __cplusplus */

#endif /* IR_CACHE_DESERIALIZER_H */
