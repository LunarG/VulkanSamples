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
#ifndef SHADER_CACHE_H
#define SHADER_CACHE_H

#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "main/macros.h"
#include "program/hash_table.h"
#include "ir.h"

#ifdef __cplusplus
const uint32_t cache_validation_data[] = {
   ir_type_max,
   GLSL_TYPE_ERROR,
   sizeof(long),
   sizeof(gl_shader),
   sizeof(gl_program),
   sizeof(gl_shader_program),
   sizeof(gl_uniform_storage),
   sizeof(gl_program_parameter_list),
   sizeof(gl_program_parameter),
   sizeof(ir_variable),
   sizeof(ir_assignment),
   sizeof(ir_call),
   sizeof(ir_constant),
   sizeof(ir_dereference_array),
   sizeof(ir_dereference_record),
   sizeof(ir_dereference_variable),
   sizeof(ir_discard),
   sizeof(ir_expression),
   sizeof(ir_function),
   sizeof(ir_function_signature),
   sizeof(ir_if),
   sizeof(ir_loop),
   sizeof(ir_loop_jump),
   sizeof(ir_return),
   sizeof(ir_swizzle),
   sizeof(ir_texture),
   sizeof(ir_emit_vertex),
   sizeof(ir_end_primitive)
};
#define num_cache_validation_data_items\
   sizeof(cache_validation_data)/sizeof(uint32_t)
#endif

/* C API for the cache */
#ifdef __cplusplus
extern "C" {
#endif

enum {
   MESA_SHADER_DESERIALIZE_READ_ERROR = -1,
   MESA_SHADER_DESERIALIZE_VERSION_ERROR = -2,
};

const char *
mesa_get_shader_cache_magic();

char *
mesa_shader_serialize(struct gl_context *ctx, struct gl_shader *shader,
                      size_t *size, bool shader_only);

char *
mesa_program_serialize(struct gl_context *ctx, struct gl_shader_program *prog,
                       size_t *size);

int
mesa_program_deserialize(struct gl_context *ctx, struct gl_shader_program *prog,
                         const GLvoid *data, size_t size);

int
mesa_program_load(struct gl_context *ctx, struct gl_shader_program *prog,
                  const char *path);

struct gl_shader *
read_single_shader(struct gl_context *ctx, struct gl_shader *shader,
                   const char* path);

int
mesa_shader_load(struct gl_context *ctx, struct gl_shader *shader,
                 const char *path);

struct gl_shader *
mesa_shader_deserialize(struct gl_context *ctx, struct gl_shader *shader,
                        const char* path);

/**
 * Features not currently supported by the caches.
 */
bool
supported_by_program_cache(struct gl_shader_program *prog, bool is_write);

bool
supported_by_shader_cache(struct gl_shader *shader, bool is_write);



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SHADER_CACHE_H */
