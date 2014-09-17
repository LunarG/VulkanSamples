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

#include "ir_serialize.h"


/**
 * Wraps serialization of an ir instruction, writes ir_type
 * and length of each instruction package as a header for it
 */
void
ir_instruction::serialize(memory_writer &mem)
{
   uint32_t data_len = 0;
   uint8_t ir_type = this->ir_type;
   mem.write_uint8_t(ir_type);

   unsigned start_pos = mem.position();
   mem.write_uint32_t(data_len);

   this->serialize_data(mem);

   data_len = mem.position() - start_pos - sizeof(data_len);
   mem.overwrite(&data_len, sizeof(data_len), start_pos);
}


/**
 * Wraps rvalue serialization, rvalue has its type or
 * ir_type_unset written before it to indicate if value is NULL
 */
static void
serialize_rvalue(ir_rvalue *val, memory_writer &mem)
{
   uint8_t ir_type = val ? val->ir_type : ir_type_unset;
   mem.write_uint8_t(ir_type);

   if (val)
      val->serialize(mem);
}


/**
 * Serialization of exec_list, writes length of the list +
 * calls serialize_data for each instruction
 */
void
serialize_list(exec_list *list, memory_writer &mem)
{
   uint32_t list_len = 0;

   unsigned start_pos = mem.position();
   mem.write_uint32_t(list_len);

   foreach_list(node, list) {
      ((ir_instruction *)node)->serialize(mem);
      list_len++;
   }

   mem.overwrite(&list_len, sizeof(list_len), start_pos);
}


void
ir_variable::serialize_data(memory_writer &mem)
{
   /* name can be NULL, see ir_print_visitor for explanation */
   const char *non_null_name = name ? name : "parameter";
   int64_t unique_id = (int64_t) (intptr_t) this;
   uint8_t mode = data.mode;
   uint8_t has_constant_value = constant_value ? 1 : 0;
   uint8_t has_constant_initializer = constant_initializer ? 1 : 0;

   type->serialize(mem);

   mem.write_string(non_null_name);
   mem.write_int64_t(unique_id);
   mem.write_uint8_t(mode);

   mem.write(&data, sizeof(data));

   mem.write_uint32_t(num_state_slots);
   mem.write_uint8_t(has_constant_value);
   mem.write_uint8_t(has_constant_initializer);

   for (unsigned i = 0; i < num_state_slots; i++) {
      mem.write_int32_t(state_slots[i].swizzle);
      for (unsigned j = 0; j < 5; j++) {
         mem.write_int32_t(state_slots[i].tokens[j]);
      }
   }

   if (constant_value)
      constant_value->serialize(mem);

   if (constant_initializer)
      constant_initializer->serialize(mem);

   uint8_t has_interface_type = get_interface_type() ? 1 : 0;

   mem.write_uint8_t(has_interface_type);
   if (has_interface_type)
      get_interface_type()->serialize(mem);
}


void
ir_assignment::serialize_data(memory_writer &mem)
{
   uint8_t assignment_mask = write_mask;
   mem.write_uint8_t(assignment_mask);

   serialize_rvalue(lhs, mem);
   serialize_rvalue(condition, mem);
   serialize_rvalue(rhs, mem);
}


void
ir_call::serialize_data(memory_writer &mem)
{
   mem.write_string(callee_name());

   uint8_t list_len = 0;
   uint8_t uses_builtin = use_builtin;

   serialize_rvalue(return_deref, mem);

   unsigned start_pos = mem.position();
   mem.write_uint8_t(list_len);

   foreach_list(node, &this->actual_parameters) {
      serialize_rvalue((ir_rvalue*)node, mem);
      list_len++;
   }

   mem.overwrite(&list_len, sizeof(list_len), start_pos);
   mem.write_uint8_t(uses_builtin);
}


void
ir_constant::serialize_data(memory_writer &mem)
{
   type->serialize(mem);

   mem.write(&value, sizeof(ir_constant_data));

   if (array_elements) {
      for (unsigned i = 0; i < type->length; i++)
         array_elements[i]->serialize(mem);
   }

   /* struct constant, dump components exec_list */
   if (!components.is_empty())
      serialize_list(&components, mem);
}


void
ir_dereference_array::serialize_data(memory_writer &mem)
{
   serialize_rvalue(array, mem);
   serialize_rvalue(array_index, mem);
}


void
ir_dereference_record::serialize_data(memory_writer &mem)
{
   mem.write_string(field);
   serialize_rvalue(record, mem);
}



/**
 * address of the variable is used as unique identifier for it
 */
void
ir_dereference_variable::serialize_data(memory_writer &mem)
{
   mem.write_int64_t((int64_t) (intptr_t) var);
}


void
ir_discard::serialize_data(memory_writer &mem)
{
   serialize_rvalue(condition, mem);
}


void
ir_expression::serialize_data(memory_writer &mem)
{
   uint32_t num_operands = get_num_operands();
   uint32_t exp_operation = operation;

   type->serialize(mem);

   mem.write_uint32_t(exp_operation);
   mem.write_uint32_t(num_operands);

   for (unsigned i = 0; i < num_operands; i++)
      serialize_rvalue(operands[i], mem);
}


void
ir_function::serialize_data(memory_writer &mem)
{
   mem.write_string(name);
   serialize_list(&signatures, mem);
}


void
ir_function_signature::serialize_data(memory_writer &mem)
{
   uint8_t builtin_func = is_builtin();
   mem.write_uint8_t(builtin_func);

   uint8_t is_defined = this->is_defined;
   mem.write_uint8_t(is_defined);

   /* dump the return type of function */
   return_type->serialize(mem);

   /* function parameters */
   serialize_list(&parameters, mem);

   /* function body */
   serialize_list(&body, mem);
}


void
ir_if::serialize_data(memory_writer &mem)
{
   serialize_rvalue(condition, mem);
   serialize_list(&then_instructions, mem);
   serialize_list(&else_instructions, mem);
}


void
ir_loop::serialize_data(memory_writer &mem)
{
   serialize_list(&body_instructions, mem);
}


void
ir_loop_jump::serialize_data(memory_writer &mem)
{
   uint8_t jump_mode = mode;
   mem.write_uint8_t(jump_mode);
}


void
ir_return::serialize_data(memory_writer &mem)
{
   serialize_rvalue(value, mem);
}


void
ir_swizzle::serialize_data(memory_writer &mem)
{
   mem.write(&mask, sizeof(ir_swizzle_mask));
   serialize_rvalue(val, mem);
}


void
ir_texture::serialize_data(memory_writer &mem)
{
   mem.write_int32_t((int32_t)op);

   type->serialize(mem);

   /* sampler */
   mem.write_uint8_t((uint8_t)sampler->ir_type);

   sampler->serialize(mem);

   serialize_rvalue(coordinate, mem);
   serialize_rvalue(projector, mem);
   serialize_rvalue(shadow_comparitor, mem);
   serialize_rvalue(offset, mem);

   /* lod_info structure */
   serialize_rvalue(lod_info.lod, mem);
   serialize_rvalue(lod_info.bias, mem);
   serialize_rvalue(lod_info.sample_index, mem);
   serialize_rvalue(lod_info.component, mem);
   serialize_rvalue(lod_info.grad.dPdx, mem);
   serialize_rvalue(lod_info.grad.dPdy, mem);
}


void
ir_emit_vertex::serialize_data(memory_writer &mem)
{
   /* no data */
}


void
ir_end_primitive::serialize_data(memory_writer &mem)
{
   /* no data */
}


void
ir_rvalue::serialize_data(memory_writer &mem)
{
   assert(0 && "unreachable");
}
