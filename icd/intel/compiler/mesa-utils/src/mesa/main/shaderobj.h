/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2004-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef SHADEROBJ_H
#define SHADEROBJ_H


#include "main/compiler.h"
#include "main/glheader.h"
#include "main/mtypes.h"
#include "program/ir_to_mesa.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Internal functions
 */

extern void
_mesa_init_shader_state(struct gl_context * ctx);

extern void
_mesa_free_shader_state(struct gl_context *ctx);


extern void
_mesa_reference_shader(struct gl_context *ctx, struct gl_shader **ptr,
                       struct gl_shader *sh);

extern void
_mesa_wait_shaders(struct gl_context *ctx,
                   struct gl_shader **shaders,
                   int num_shaders);

extern struct gl_shader *
_mesa_lookup_shader_no_wait(struct gl_context *ctx, GLuint name);

extern struct gl_shader *
_mesa_lookup_shader_err_no_wait(struct gl_context *ctx, GLuint name,
                                const char *caller);

static inline struct gl_shader *
_mesa_lookup_shader(struct gl_context *ctx, GLuint name)
{
   struct gl_shader *sh = _mesa_lookup_shader_no_wait(ctx, name);
   if (sh)
      _mesa_wait_shaders(ctx, &sh, 1);
   return sh;
}

static inline struct gl_shader *
_mesa_lookup_shader_err(struct gl_context *ctx, GLuint name, const char *caller)
{
   struct gl_shader *sh = _mesa_lookup_shader_err_no_wait(ctx, name, caller);
   if (sh)
      _mesa_wait_shaders(ctx, &sh, 1);
   return sh;
}

extern void
_mesa_reference_shader_program(struct gl_context *ctx,
                               struct gl_shader_program **ptr,
                               struct gl_shader_program *shProg);
extern void
_mesa_init_shader(struct gl_context *ctx, struct gl_shader *shader);

extern struct gl_shader *
_mesa_new_shader(struct gl_context *ctx, GLuint name, GLenum type);

extern void
_mesa_init_shader_program(struct gl_context *ctx, struct gl_shader_program *prog);

extern void
_mesa_wait_shader_program(struct gl_context *ctx,
                          struct gl_shader_program *shProg);

extern struct gl_shader_program *
_mesa_lookup_shader_program_no_wait(struct gl_context *ctx, GLuint name);

extern struct gl_shader_program *
_mesa_lookup_shader_program_err_no_wait(struct gl_context *ctx, GLuint name,
                                        const char *caller);

static inline struct gl_shader_program *
_mesa_lookup_shader_program(struct gl_context *ctx, GLuint name)
{
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_no_wait(ctx, name);
   if (shProg)
      _mesa_wait_shader_program(ctx, shProg);
   return shProg;
}

static inline struct gl_shader_program *
_mesa_lookup_shader_program_err(struct gl_context *ctx, GLuint name,
                                const char *caller)
{
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err_no_wait(ctx, name, caller);
   if (shProg)
      _mesa_wait_shader_program(ctx, shProg);
   return shProg;
}

extern void
_mesa_clear_shader_program_data(struct gl_context *ctx,
                                struct gl_shader_program *shProg);

extern void
_mesa_free_shader_program_data(struct gl_context *ctx,
                               struct gl_shader_program *shProg);



extern void
_mesa_init_shader_object_functions(struct dd_function_table *driver);

extern void
_mesa_init_shader_state(struct gl_context *ctx);

extern void
_mesa_free_shader_state(struct gl_context *ctx);


static inline gl_shader_stage
_mesa_shader_enum_to_shader_stage(GLenum v)
{
   switch (v) {
   case GL_VERTEX_SHADER:
      return MESA_SHADER_VERTEX;
   case GL_FRAGMENT_SHADER:
      return MESA_SHADER_FRAGMENT;
   case GL_GEOMETRY_SHADER:
      return MESA_SHADER_GEOMETRY;
   case GL_COMPUTE_SHADER:
      return MESA_SHADER_COMPUTE;
   default:
      ASSERT(0 && "bad value in _mesa_shader_enum_to_shader_stage()");
      return MESA_SHADER_VERTEX;
   }
}


#ifdef __cplusplus
}
#endif

#endif /* SHADEROBJ_H */
