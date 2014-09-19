/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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

/**
 * \file prog_statevars.c
 * Program state variable management.
 * \author Brian Paul
 */


#include "main/glheader.h"
#include "main/context.h"
//#include "main/blend.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
//#include "main/fbobject.h"
#include "prog_statevars.h"
#include "prog_parameter.h"
//#include "main/samplerobj.h"


/**
 * Use the list of tokens in the state[] array to find global GL state
 * and return it in <value>.  Usually, four values are returned in <value>
 * but matrix queries may return as many as 16 values.
 * This function is used for ARB vertex/fragment programs.
 * The program parser will produce the state[] values.
 */
static void
_mesa_fetch_state(struct gl_context *ctx, const gl_state_index state[],
                  GLfloat *value)
{
    _mesa_problem(ctx, "Invalid state in _mesa_fetch_state");
}


/**
 * Return a bitmask of the Mesa state flags (_NEW_* values) which would
 * indicate that the given context state may have changed.
 * The bitmask is used during validation to determine if we need to update
 * vertex/fragment program parameters (like "state.material.color") when
 * some GL state has changed.
 */
GLbitfield
_mesa_program_state_flags(const gl_state_index state[STATE_LENGTH])
{
   switch (state[0]) {
   case STATE_MATERIAL:
   case STATE_LIGHTPROD:
   case STATE_LIGHTMODEL_SCENECOLOR:
      /* these can be effected by glColor when colormaterial mode is used */
      return _NEW_LIGHT | _NEW_CURRENT_ATTRIB;

   case STATE_LIGHT:
   case STATE_LIGHTMODEL_AMBIENT:
      return _NEW_LIGHT;

   case STATE_TEXGEN:
      return _NEW_TEXTURE;
   case STATE_TEXENV_COLOR:
      return _NEW_TEXTURE | _NEW_BUFFERS | _NEW_FRAG_CLAMP;

   case STATE_FOG_COLOR:
      return _NEW_FOG | _NEW_BUFFERS | _NEW_FRAG_CLAMP;
   case STATE_FOG_PARAMS:
      return _NEW_FOG;

   case STATE_CLIPPLANE:
      return _NEW_TRANSFORM;

   case STATE_POINT_SIZE:
   case STATE_POINT_ATTENUATION:
      return _NEW_POINT;

   case STATE_MODELVIEW_MATRIX:
      return _NEW_MODELVIEW;
   case STATE_PROJECTION_MATRIX:
      return _NEW_PROJECTION;
   case STATE_MVP_MATRIX:
      return _NEW_MODELVIEW | _NEW_PROJECTION;
   case STATE_TEXTURE_MATRIX:
      return _NEW_TEXTURE_MATRIX;
   case STATE_PROGRAM_MATRIX:
      return _NEW_TRACK_MATRIX;

   case STATE_NUM_SAMPLES:
      return _NEW_BUFFERS;

   case STATE_DEPTH_RANGE:
      return _NEW_VIEWPORT;

   case STATE_FRAGMENT_PROGRAM:
   case STATE_VERTEX_PROGRAM:
      return _NEW_PROGRAM;

   case STATE_NORMAL_SCALE:
      return _NEW_MODELVIEW;

   case STATE_INTERNAL:
      switch (state[1]) {
      case STATE_CURRENT_ATTRIB:
         return _NEW_CURRENT_ATTRIB;
      case STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED:
         return _NEW_CURRENT_ATTRIB | _NEW_LIGHT | _NEW_BUFFERS;

      case STATE_NORMAL_SCALE:
         return _NEW_MODELVIEW;

      case STATE_TEXRECT_SCALE:
      case STATE_ROT_MATRIX_0:
      case STATE_ROT_MATRIX_1:
	 return _NEW_TEXTURE;
      case STATE_FOG_PARAMS_OPTIMIZED:
	 return _NEW_FOG;
      case STATE_POINT_SIZE_CLAMPED:
         return _NEW_POINT | _NEW_MULTISAMPLE;
      case STATE_LIGHT_SPOT_DIR_NORMALIZED:
      case STATE_LIGHT_POSITION:
      case STATE_LIGHT_POSITION_NORMALIZED:
      case STATE_LIGHT_HALF_VECTOR:
         return _NEW_LIGHT;

      case STATE_PT_SCALE:
      case STATE_PT_BIAS:
         return _NEW_PIXEL;

      case STATE_FB_SIZE:
      case STATE_FB_WPOS_Y_TRANSFORM:
         return _NEW_BUFFERS;

      default:
         /* unknown state indexes are silently ignored and
         *  no flag set, since it is handled by the driver.
         */
	 return 0;
      }

   default:
      _mesa_problem(NULL, "unexpected state[0] in make_state_flags()");
      return 0;
   }
}


static void
append(char *dst, const char *src)
{
   while (*dst)
      dst++;
   while (*src)
     *dst++ = *src++;
   *dst = 0;
}


/**
 * Convert token 'k' to a string, append it onto 'dst' string.
 */
static void
append_token(char *dst, gl_state_index k)
{
   switch (k) {
   case STATE_MATERIAL:
      append(dst, "material");
      break;
   case STATE_LIGHT:
      append(dst, "light");
      break;
   case STATE_LIGHTMODEL_AMBIENT:
      append(dst, "lightmodel.ambient");
      break;
   case STATE_LIGHTMODEL_SCENECOLOR:
      break;
   case STATE_LIGHTPROD:
      append(dst, "lightprod");
      break;
   case STATE_TEXGEN:
      append(dst, "texgen");
      break;
   case STATE_FOG_COLOR:
      append(dst, "fog.color");
      break;
   case STATE_FOG_PARAMS:
      append(dst, "fog.params");
      break;
   case STATE_CLIPPLANE:
      append(dst, "clip");
      break;
   case STATE_POINT_SIZE:
      append(dst, "point.size");
      break;
   case STATE_POINT_ATTENUATION:
      append(dst, "point.attenuation");
      break;
   case STATE_MODELVIEW_MATRIX:
      append(dst, "matrix.modelview");
      break;
   case STATE_PROJECTION_MATRIX:
      append(dst, "matrix.projection");
      break;
   case STATE_MVP_MATRIX:
      append(dst, "matrix.mvp");
      break;
   case STATE_TEXTURE_MATRIX:
      append(dst, "matrix.texture");
      break;
   case STATE_PROGRAM_MATRIX:
      append(dst, "matrix.program");
      break;
   case STATE_MATRIX_INVERSE:
      append(dst, ".inverse");
      break;
   case STATE_MATRIX_TRANSPOSE:
      append(dst, ".transpose");
      break;
   case STATE_MATRIX_INVTRANS:
      append(dst, ".invtrans");
      break;
   case STATE_AMBIENT:
      append(dst, ".ambient");
      break;
   case STATE_DIFFUSE:
      append(dst, ".diffuse");
      break;
   case STATE_SPECULAR:
      append(dst, ".specular");
      break;
   case STATE_EMISSION:
      append(dst, ".emission");
      break;
   case STATE_SHININESS:
      append(dst, "lshininess");
      break;
   case STATE_HALF_VECTOR:
      append(dst, ".half");
      break;
   case STATE_POSITION:
      append(dst, ".position");
      break;
   case STATE_ATTENUATION:
      append(dst, ".attenuation");
      break;
   case STATE_SPOT_DIRECTION:
      append(dst, ".spot.direction");
      break;
   case STATE_SPOT_CUTOFF:
      append(dst, ".spot.cutoff");
      break;
   case STATE_TEXGEN_EYE_S:
      append(dst, ".eye.s");
      break;
   case STATE_TEXGEN_EYE_T:
      append(dst, ".eye.t");
      break;
   case STATE_TEXGEN_EYE_R:
      append(dst, ".eye.r");
      break;
   case STATE_TEXGEN_EYE_Q:
      append(dst, ".eye.q");
      break;
   case STATE_TEXGEN_OBJECT_S:
      append(dst, ".object.s");
      break;
   case STATE_TEXGEN_OBJECT_T:
      append(dst, ".object.t");
      break;
   case STATE_TEXGEN_OBJECT_R:
      append(dst, ".object.r");
      break;
   case STATE_TEXGEN_OBJECT_Q:
      append(dst, ".object.q");
      break;
   case STATE_TEXENV_COLOR:
      append(dst, "texenv");
      break;
   case STATE_NUM_SAMPLES:
      append(dst, "numsamples");
      break;
   case STATE_DEPTH_RANGE:
      append(dst, "depth.range");
      break;
   case STATE_VERTEX_PROGRAM:
   case STATE_FRAGMENT_PROGRAM:
      break;
   case STATE_ENV:
      append(dst, "env");
      break;
   case STATE_LOCAL:
      append(dst, "local");
      break;
   /* BEGIN internal state vars */
   case STATE_INTERNAL:
      append(dst, ".internal.");
      break;
   case STATE_CURRENT_ATTRIB:
      append(dst, "current");
      break;
   case STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED:
      append(dst, "currentAttribMaybeVPClamped");
      break;
   case STATE_NORMAL_SCALE:
      append(dst, "normalScale");
      break;
   case STATE_TEXRECT_SCALE:
      append(dst, "texrectScale");
      break;
   case STATE_FOG_PARAMS_OPTIMIZED:
      append(dst, "fogParamsOptimized");
      break;
   case STATE_POINT_SIZE_CLAMPED:
      append(dst, "pointSizeClamped");
      break;
   case STATE_LIGHT_SPOT_DIR_NORMALIZED:
      append(dst, "lightSpotDirNormalized");
      break;
   case STATE_LIGHT_POSITION:
      append(dst, "lightPosition");
      break;
   case STATE_LIGHT_POSITION_NORMALIZED:
      append(dst, "light.position.normalized");
      break;
   case STATE_LIGHT_HALF_VECTOR:
      append(dst, "lightHalfVector");
      break;
   case STATE_PT_SCALE:
      append(dst, "PTscale");
      break;
   case STATE_PT_BIAS:
      append(dst, "PTbias");
      break;
   case STATE_FB_SIZE:
      append(dst, "FbSize");
      break;
   case STATE_FB_WPOS_Y_TRANSFORM:
      append(dst, "FbWposYTransform");
      break;
   case STATE_ROT_MATRIX_0:
      append(dst, "rotMatrixRow0");
      break;
   case STATE_ROT_MATRIX_1:
      append(dst, "rotMatrixRow1");
      break;
   default:
      /* probably STATE_INTERNAL_DRIVER+i (driver private state) */
      append(dst, "driverState");
   }
}

static void
append_face(char *dst, GLint face)
{
   if (face == 0)
      append(dst, "front.");
   else
      append(dst, "back.");
}

static void
append_index(char *dst, GLint index)
{
   char s[20];
   sprintf(s, "[%d]", index);
   append(dst, s);
}

/**
 * Make a string from the given state vector.
 * For example, return "state.matrix.texture[2].inverse".
 * Use free() to deallocate the string.
 */
char *
_mesa_program_state_string(const gl_state_index state[STATE_LENGTH])
{
   char str[1000] = "";
   char tmp[30];

   append(str, "state.");
   append_token(str, state[0]);

   switch (state[0]) {
   case STATE_MATERIAL:
      append_face(str, state[1]);
      append_token(str, state[2]);
      break;
   case STATE_LIGHT:
      append_index(str, state[1]); /* light number [i]. */
      append_token(str, state[2]); /* coefficients */
      break;
   case STATE_LIGHTMODEL_AMBIENT:
      append(str, "lightmodel.ambient");
      break;
   case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
         append(str, "lightmodel.front.scenecolor");
      }
      else {
         append(str, "lightmodel.back.scenecolor");
      }
      break;
   case STATE_LIGHTPROD:
      append_index(str, state[1]); /* light number [i]. */
      append_face(str, state[2]);
      append_token(str, state[3]);
      break;
   case STATE_TEXGEN:
      append_index(str, state[1]); /* tex unit [i] */
      append_token(str, state[2]); /* plane coef */
      break;
   case STATE_TEXENV_COLOR:
      append_index(str, state[1]); /* tex unit [i] */
      append(str, "color");
      break;
   case STATE_CLIPPLANE:
      append_index(str, state[1]); /* plane [i] */
      append(str, ".plane");
      break;
   case STATE_MODELVIEW_MATRIX:
   case STATE_PROJECTION_MATRIX:
   case STATE_MVP_MATRIX:
   case STATE_TEXTURE_MATRIX:
   case STATE_PROGRAM_MATRIX:
      {
         /* state[0] = modelview, projection, texture, etc. */
         /* state[1] = which texture matrix or program matrix */
         /* state[2] = first row to fetch */
         /* state[3] = last row to fetch */
         /* state[4] = transpose, inverse or invtrans */
         const gl_state_index mat = state[0];
         const GLuint index = (GLuint) state[1];
         const GLuint firstRow = (GLuint) state[2];
         const GLuint lastRow = (GLuint) state[3];
         const gl_state_index modifier = state[4];
         if (index ||
             mat == STATE_TEXTURE_MATRIX ||
             mat == STATE_PROGRAM_MATRIX)
            append_index(str, index);
         if (modifier)
            append_token(str, modifier);
         if (firstRow == lastRow)
            sprintf(tmp, ".row[%d]", firstRow);
         else
            sprintf(tmp, ".row[%d..%d]", firstRow, lastRow);
         append(str, tmp);
      }
      break;
   case STATE_POINT_SIZE:
      break;
   case STATE_POINT_ATTENUATION:
      break;
   case STATE_FOG_PARAMS:
      break;
   case STATE_FOG_COLOR:
      break;
   case STATE_NUM_SAMPLES:
      break;
   case STATE_DEPTH_RANGE:
      break;
   case STATE_FRAGMENT_PROGRAM:
   case STATE_VERTEX_PROGRAM:
      /* state[1] = {STATE_ENV, STATE_LOCAL} */
      /* state[2] = parameter index          */
      append_token(str, state[1]);
      append_index(str, state[2]);
      break;
   case STATE_NORMAL_SCALE:
      break;
   case STATE_INTERNAL:
      append_token(str, state[1]);
      if (state[1] == STATE_CURRENT_ATTRIB)
         append_index(str, state[2]);
       break;
   default:
      _mesa_problem(NULL, "Invalid state in _mesa_program_state_string");
      break;
   }

   return _mesa_strdup(str);
}


/**
 * Loop over all the parameters in a parameter list.  If the parameter
 * is a GL state reference, look up the current value of that state
 * variable and put it into the parameter's Value[4] array.
 * Other parameter types never change or are explicitly set by the user
 * with glUniform() or glProgramParameter(), etc.
 * This would be called at glBegin time.
 */
void
_mesa_load_state_parameters(struct gl_context *ctx,
                            struct gl_program_parameter_list *paramList)
{
   GLuint i;

   if (!paramList)
      return;

   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Type == PROGRAM_STATE_VAR) {
         _mesa_fetch_state(ctx,
			   paramList->Parameters[i].StateIndexes,
                           &paramList->ParameterValues[i][0].f);
      }
   }
}
