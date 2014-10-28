/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file mtypes.h
 * Main Mesa data structures.
 *
 * Please try to mark derived values with a leading underscore ('_').
 */

#ifndef MTYPES_H
#define MTYPES_H


#include <stdint.h>             /* uint32_t */
#include <stdbool.h>

#include "main/glheader.h"
#include "main/config.h"
#include "glapi/glapi.h"
#include "math/m_matrix.h"	/* GLmatrix */
#include "main/simple_list.h"	/* struct simple_node */
#include "main/formats.h"       /* MESA_FORMAT_COUNT */


#ifdef __cplusplus
extern "C" {
#endif


/**
 * \name 64-bit extension of GLbitfield.
 */
/*@{*/
typedef GLuint64 GLbitfield64;

/** Set a single bit */
#define BITFIELD64_BIT(b)      ((GLbitfield64)1 << (b))
/** Set all bits up to excluding bit b */
#define BITFIELD64_MASK(b)      \
   ((b) == 64 ? (~(GLbitfield64)0) : BITFIELD64_BIT(b) - 1)
/** Set count bits starting from bit b  */
#define BITFIELD64_RANGE(b, count) \
   (BITFIELD64_MASK((b) + (count)) & ~BITFIELD64_MASK(b))


/**
 * \name Some forward type declarations
 */
/*@{*/
struct _mesa_HashTable;
struct _mesa_threadpool;
struct _mesa_threadpool_task;
struct gl_attrib_node;
struct gl_list_extensions;
struct gl_meta_state;
struct gl_program_cache;
struct gl_texture_object;
struct gl_debug_state;
struct gl_context;
struct st_context;
struct gl_uniform_storage;
struct prog_instruction;
struct gl_program_parameter_list;
struct set;
struct set_entry;
struct vbo_context;
/*@}*/


/** Extra draw modes beyond GL_POINTS, GL_TRIANGLE_FAN, etc */
#define PRIM_MAX                 GL_TRIANGLE_STRIP_ADJACENCY
#define PRIM_OUTSIDE_BEGIN_END   (PRIM_MAX + 1)
#define PRIM_UNKNOWN             (PRIM_MAX + 2)



/**
 * Indexes for vertex program attributes.
 * GL_NV_vertex_program aliases generic attributes over the conventional
 * attributes.  In GL_ARB_vertex_program shader the aliasing is optional.
 * In GL_ARB_vertex_shader / OpenGL 2.0 the aliasing is disallowed (the
 * generic attributes are distinct/separate).
 */
typedef enum
{
   VERT_ATTRIB_POS = 0,
   VERT_ATTRIB_WEIGHT = 1,
   VERT_ATTRIB_NORMAL = 2,
   VERT_ATTRIB_COLOR0 = 3,
   VERT_ATTRIB_COLOR1 = 4,
   VERT_ATTRIB_FOG = 5,
   VERT_ATTRIB_COLOR_INDEX = 6,
   VERT_ATTRIB_EDGEFLAG = 7,
   VERT_ATTRIB_TEX0 = 8,
   VERT_ATTRIB_TEX1 = 9,
   VERT_ATTRIB_TEX2 = 10,
   VERT_ATTRIB_TEX3 = 11,
   VERT_ATTRIB_TEX4 = 12,
   VERT_ATTRIB_TEX5 = 13,
   VERT_ATTRIB_TEX6 = 14,
   VERT_ATTRIB_TEX7 = 15,
   VERT_ATTRIB_POINT_SIZE = 16,
   VERT_ATTRIB_GENERIC0 = 17,
   VERT_ATTRIB_GENERIC1 = 18,
   VERT_ATTRIB_GENERIC2 = 19,
   VERT_ATTRIB_GENERIC3 = 20,
   VERT_ATTRIB_GENERIC4 = 21,
   VERT_ATTRIB_GENERIC5 = 22,
   VERT_ATTRIB_GENERIC6 = 23,
   VERT_ATTRIB_GENERIC7 = 24,
   VERT_ATTRIB_GENERIC8 = 25,
   VERT_ATTRIB_GENERIC9 = 26,
   VERT_ATTRIB_GENERIC10 = 27,
   VERT_ATTRIB_GENERIC11 = 28,
   VERT_ATTRIB_GENERIC12 = 29,
   VERT_ATTRIB_GENERIC13 = 30,
   VERT_ATTRIB_GENERIC14 = 31,
   VERT_ATTRIB_GENERIC15 = 32,
   VERT_ATTRIB_MAX = 33
} gl_vert_attrib;

/**
 * Symbolic constats to help iterating over
 * specific blocks of vertex attributes.
 *
 * VERT_ATTRIB_FF
 *   includes all fixed function attributes as well as
 *   the aliased GL_NV_vertex_program shader attributes.
 * VERT_ATTRIB_TEX
 *   include the classic texture coordinate attributes.
 *   Is a subset of VERT_ATTRIB_FF.
 * VERT_ATTRIB_GENERIC
 *   include the OpenGL 2.0+ GLSL generic shader attributes.
 *   These alias the generic GL_ARB_vertex_shader attributes.
 */
#define VERT_ATTRIB_FF(i)           (VERT_ATTRIB_POS + (i))
#define VERT_ATTRIB_FF_MAX          VERT_ATTRIB_GENERIC0

#define VERT_ATTRIB_TEX(i)          (VERT_ATTRIB_TEX0 + (i))
#define VERT_ATTRIB_TEX_MAX         MAX_TEXTURE_COORD_UNITS

#define VERT_ATTRIB_GENERIC(i)      (VERT_ATTRIB_GENERIC0 + (i))
#define VERT_ATTRIB_GENERIC_MAX     MAX_VERTEX_GENERIC_ATTRIBS

/**
 * Bitflags for vertex attributes.
 * These are used in bitfields in many places.
 */
/*@{*/
#define VERT_BIT_POS             BITFIELD64_BIT(VERT_ATTRIB_POS)
#define VERT_BIT_WEIGHT          BITFIELD64_BIT(VERT_ATTRIB_WEIGHT)
#define VERT_BIT_NORMAL          BITFIELD64_BIT(VERT_ATTRIB_NORMAL)
#define VERT_BIT_COLOR0          BITFIELD64_BIT(VERT_ATTRIB_COLOR0)
#define VERT_BIT_COLOR1          BITFIELD64_BIT(VERT_ATTRIB_COLOR1)
#define VERT_BIT_FOG             BITFIELD64_BIT(VERT_ATTRIB_FOG)
#define VERT_BIT_COLOR_INDEX     BITFIELD64_BIT(VERT_ATTRIB_COLOR_INDEX)
#define VERT_BIT_EDGEFLAG        BITFIELD64_BIT(VERT_ATTRIB_EDGEFLAG)
#define VERT_BIT_TEX0            BITFIELD64_BIT(VERT_ATTRIB_TEX0)
#define VERT_BIT_TEX1            BITFIELD64_BIT(VERT_ATTRIB_TEX1)
#define VERT_BIT_TEX2            BITFIELD64_BIT(VERT_ATTRIB_TEX2)
#define VERT_BIT_TEX3            BITFIELD64_BIT(VERT_ATTRIB_TEX3)
#define VERT_BIT_TEX4            BITFIELD64_BIT(VERT_ATTRIB_TEX4)
#define VERT_BIT_TEX5            BITFIELD64_BIT(VERT_ATTRIB_TEX5)
#define VERT_BIT_TEX6            BITFIELD64_BIT(VERT_ATTRIB_TEX6)
#define VERT_BIT_TEX7            BITFIELD64_BIT(VERT_ATTRIB_TEX7)
#define VERT_BIT_POINT_SIZE      BITFIELD64_BIT(VERT_ATTRIB_POINT_SIZE)
#define VERT_BIT_GENERIC0        BITFIELD64_BIT(VERT_ATTRIB_GENERIC0)

#define VERT_BIT(i)              BITFIELD64_BIT(i)
#define VERT_BIT_ALL             BITFIELD64_RANGE(0, VERT_ATTRIB_MAX)

#define VERT_BIT_FF(i)           VERT_BIT(i)
#define VERT_BIT_FF_ALL          BITFIELD64_RANGE(0, VERT_ATTRIB_FF_MAX)
#define VERT_BIT_TEX(i)          VERT_BIT(VERT_ATTRIB_TEX(i))
#define VERT_BIT_TEX_ALL         \
   BITFIELD64_RANGE(VERT_ATTRIB_TEX(0), VERT_ATTRIB_TEX_MAX)

#define VERT_BIT_GENERIC(i)      VERT_BIT(VERT_ATTRIB_GENERIC(i))
#define VERT_BIT_GENERIC_ALL     \
   BITFIELD64_RANGE(VERT_ATTRIB_GENERIC(0), VERT_ATTRIB_GENERIC_MAX)
/*@}*/


/**
 * Indexes for vertex shader outputs, geometry shader inputs/outputs, and
 * fragment shader inputs.
 *
 * Note that some of these values are not available to all pipeline stages.
 *
 * When this enum is updated, the following code must be updated too:
 * - vertResults (in prog_print.c's arb_output_attrib_string())
 * - fragAttribs (in prog_print.c's arb_input_attrib_string())
 * - _mesa_varying_slot_in_fs()
 */
typedef enum
{
   VARYING_SLOT_POS,
   VARYING_SLOT_COL0, /* COL0 and COL1 must be contiguous */
   VARYING_SLOT_COL1,
   VARYING_SLOT_FOGC,
   VARYING_SLOT_TEX0, /* TEX0-TEX7 must be contiguous */
   VARYING_SLOT_TEX1,
   VARYING_SLOT_TEX2,
   VARYING_SLOT_TEX3,
   VARYING_SLOT_TEX4,
   VARYING_SLOT_TEX5,
   VARYING_SLOT_TEX6,
   VARYING_SLOT_TEX7,
   VARYING_SLOT_PSIZ, /* Does not appear in FS */
   VARYING_SLOT_BFC0, /* Does not appear in FS */
   VARYING_SLOT_BFC1, /* Does not appear in FS */
   VARYING_SLOT_EDGE, /* Does not appear in FS */
   VARYING_SLOT_CLIP_VERTEX, /* Does not appear in FS */
   VARYING_SLOT_CLIP_DIST0,
   VARYING_SLOT_CLIP_DIST1,
   VARYING_SLOT_PRIMITIVE_ID, /* Does not appear in VS */
   VARYING_SLOT_LAYER, /* Appears as VS or GS output */
   VARYING_SLOT_VIEWPORT, /* Appears as VS or GS output */
   VARYING_SLOT_FACE, /* FS only */
   VARYING_SLOT_PNTC, /* FS only */
   VARYING_SLOT_VAR0, /* First generic varying slot */
   VARYING_SLOT_MAX = VARYING_SLOT_VAR0 + MAX_VARYING
} gl_varying_slot;


/**
 * Bitflags for varying slots.
 */
/*@{*/
#define VARYING_BIT_POS BITFIELD64_BIT(VARYING_SLOT_POS)
#define VARYING_BIT_COL0 BITFIELD64_BIT(VARYING_SLOT_COL0)
#define VARYING_BIT_COL1 BITFIELD64_BIT(VARYING_SLOT_COL1)
#define VARYING_BIT_FOGC BITFIELD64_BIT(VARYING_SLOT_FOGC)
#define VARYING_BIT_TEX0 BITFIELD64_BIT(VARYING_SLOT_TEX0)
#define VARYING_BIT_TEX1 BITFIELD64_BIT(VARYING_SLOT_TEX1)
#define VARYING_BIT_TEX2 BITFIELD64_BIT(VARYING_SLOT_TEX2)
#define VARYING_BIT_TEX3 BITFIELD64_BIT(VARYING_SLOT_TEX3)
#define VARYING_BIT_TEX4 BITFIELD64_BIT(VARYING_SLOT_TEX4)
#define VARYING_BIT_TEX5 BITFIELD64_BIT(VARYING_SLOT_TEX5)
#define VARYING_BIT_TEX6 BITFIELD64_BIT(VARYING_SLOT_TEX6)
#define VARYING_BIT_TEX7 BITFIELD64_BIT(VARYING_SLOT_TEX7)
#define VARYING_BIT_TEX(U) BITFIELD64_BIT(VARYING_SLOT_TEX0 + (U))
#define VARYING_BITS_TEX_ANY BITFIELD64_RANGE(VARYING_SLOT_TEX0, \
                                              MAX_TEXTURE_COORD_UNITS)
#define VARYING_BIT_PSIZ BITFIELD64_BIT(VARYING_SLOT_PSIZ)
#define VARYING_BIT_BFC0 BITFIELD64_BIT(VARYING_SLOT_BFC0)
#define VARYING_BIT_BFC1 BITFIELD64_BIT(VARYING_SLOT_BFC1)
#define VARYING_BIT_EDGE BITFIELD64_BIT(VARYING_SLOT_EDGE)
#define VARYING_BIT_CLIP_VERTEX BITFIELD64_BIT(VARYING_SLOT_CLIP_VERTEX)
#define VARYING_BIT_CLIP_DIST0 BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0)
#define VARYING_BIT_CLIP_DIST1 BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1)
#define VARYING_BIT_PRIMITIVE_ID BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_ID)
#define VARYING_BIT_LAYER BITFIELD64_BIT(VARYING_SLOT_LAYER)
#define VARYING_BIT_VIEWPORT BITFIELD64_BIT(VARYING_SLOT_VIEWPORT)
#define VARYING_BIT_FACE BITFIELD64_BIT(VARYING_SLOT_FACE)
#define VARYING_BIT_PNTC BITFIELD64_BIT(VARYING_SLOT_PNTC)
#define VARYING_BIT_VAR(V) BITFIELD64_BIT(VARYING_SLOT_VAR0 + (V))
/*@}*/

/**
 * Bitflags for system values.
 */
#define SYSTEM_BIT_SAMPLE_ID BITFIELD64_BIT(SYSTEM_VALUE_SAMPLE_ID)
#define SYSTEM_BIT_SAMPLE_POS BITFIELD64_BIT(SYSTEM_VALUE_SAMPLE_POS)
#define SYSTEM_BIT_SAMPLE_MASK_IN BITFIELD64_BIT(SYSTEM_VALUE_SAMPLE_MASK_IN)

/**
 * Determine if the given gl_varying_slot appears in the fragment shader.
 */
static inline GLboolean
_mesa_varying_slot_in_fs(gl_varying_slot slot)
{
   switch (slot) {
   case VARYING_SLOT_PSIZ:
   case VARYING_SLOT_BFC0:
   case VARYING_SLOT_BFC1:
   case VARYING_SLOT_EDGE:
   case VARYING_SLOT_CLIP_VERTEX:
   case VARYING_SLOT_LAYER:
      return GL_FALSE;
   default:
      return GL_TRUE;
   }
}


/**
 * Fragment program results
 */
typedef enum
{
   FRAG_RESULT_DEPTH = 0,
   FRAG_RESULT_STENCIL = 1,
   /* If a single color should be written to all render targets, this
    * register is written.  No FRAG_RESULT_DATAn will be written.
    */
   FRAG_RESULT_COLOR = 2,
   FRAG_RESULT_SAMPLE_MASK = 3,

   /* FRAG_RESULT_DATAn are the per-render-target (GLSL gl_FragData[n]
    * or ARB_fragment_program fragment.color[n]) color results.  If
    * any are written, FRAG_RESULT_COLOR will not be written.
    */
   FRAG_RESULT_DATA0 = 4,
   FRAG_RESULT_MAX = (FRAG_RESULT_DATA0 + MAX_DRAW_BUFFERS)
} gl_frag_result;


/**
 * Indexes for all renderbuffers
 */
typedef enum
{
   /* the four standard color buffers */
   BUFFER_FRONT_LEFT,
   BUFFER_BACK_LEFT,
   BUFFER_FRONT_RIGHT,
   BUFFER_BACK_RIGHT,
   BUFFER_DEPTH,
   BUFFER_STENCIL,
   BUFFER_ACCUM,
   /* optional aux buffer */
   BUFFER_AUX0,
   /* generic renderbuffers */
   BUFFER_COLOR0,
   BUFFER_COLOR1,
   BUFFER_COLOR2,
   BUFFER_COLOR3,
   BUFFER_COLOR4,
   BUFFER_COLOR5,
   BUFFER_COLOR6,
   BUFFER_COLOR7,
   BUFFER_COUNT
} gl_buffer_index;

/**
 * Bit flags for all renderbuffers
 */
#define BUFFER_BIT_FRONT_LEFT   (1 << BUFFER_FRONT_LEFT)
#define BUFFER_BIT_BACK_LEFT    (1 << BUFFER_BACK_LEFT)
#define BUFFER_BIT_FRONT_RIGHT  (1 << BUFFER_FRONT_RIGHT)
#define BUFFER_BIT_BACK_RIGHT   (1 << BUFFER_BACK_RIGHT)
#define BUFFER_BIT_AUX0         (1 << BUFFER_AUX0)
#define BUFFER_BIT_AUX1         (1 << BUFFER_AUX1)
#define BUFFER_BIT_AUX2         (1 << BUFFER_AUX2)
#define BUFFER_BIT_AUX3         (1 << BUFFER_AUX3)
#define BUFFER_BIT_DEPTH        (1 << BUFFER_DEPTH)
#define BUFFER_BIT_STENCIL      (1 << BUFFER_STENCIL)
#define BUFFER_BIT_ACCUM        (1 << BUFFER_ACCUM)
#define BUFFER_BIT_COLOR0       (1 << BUFFER_COLOR0)
#define BUFFER_BIT_COLOR1       (1 << BUFFER_COLOR1)
#define BUFFER_BIT_COLOR2       (1 << BUFFER_COLOR2)
#define BUFFER_BIT_COLOR3       (1 << BUFFER_COLOR3)
#define BUFFER_BIT_COLOR4       (1 << BUFFER_COLOR4)
#define BUFFER_BIT_COLOR5       (1 << BUFFER_COLOR5)
#define BUFFER_BIT_COLOR6       (1 << BUFFER_COLOR6)
#define BUFFER_BIT_COLOR7       (1 << BUFFER_COLOR7)

/**
 * Mask of all the color buffer bits (but not accum).
 */
#define BUFFER_BITS_COLOR  (BUFFER_BIT_FRONT_LEFT | \
                            BUFFER_BIT_BACK_LEFT | \
                            BUFFER_BIT_FRONT_RIGHT | \
                            BUFFER_BIT_BACK_RIGHT | \
                            BUFFER_BIT_AUX0 | \
                            BUFFER_BIT_COLOR0 | \
                            BUFFER_BIT_COLOR1 | \
                            BUFFER_BIT_COLOR2 | \
                            BUFFER_BIT_COLOR3 | \
                            BUFFER_BIT_COLOR4 | \
                            BUFFER_BIT_COLOR5 | \
                            BUFFER_BIT_COLOR6 | \
                            BUFFER_BIT_COLOR7)


/**
 * Shader stages. Note that these will become 5 with tessellation.
 *
 * The order must match how shaders are ordered in the pipeline.
 * The GLSL linker assumes that if i<j, then the j-th shader is
 * executed later than the i-th shader.
 */
typedef enum
{
   MESA_SHADER_VERTEX = 0,
   MESA_SHADER_GEOMETRY = 1,
   MESA_SHADER_FRAGMENT = 2,
   MESA_SHADER_COMPUTE = 3,
} gl_shader_stage;

#define MESA_SHADER_STAGES (MESA_SHADER_COMPUTE + 1)



/**
 * \name Bit flags used for updating material values.
 */
/*@{*/
#define MAT_ATTRIB_FRONT_AMBIENT           0 
#define MAT_ATTRIB_BACK_AMBIENT            1
#define MAT_ATTRIB_FRONT_DIFFUSE           2 
#define MAT_ATTRIB_BACK_DIFFUSE            3
#define MAT_ATTRIB_FRONT_SPECULAR          4 
#define MAT_ATTRIB_BACK_SPECULAR           5
#define MAT_ATTRIB_FRONT_EMISSION          6
#define MAT_ATTRIB_BACK_EMISSION           7
#define MAT_ATTRIB_FRONT_SHININESS         8
#define MAT_ATTRIB_BACK_SHININESS          9
#define MAT_ATTRIB_FRONT_INDEXES           10
#define MAT_ATTRIB_BACK_INDEXES            11
#define MAT_ATTRIB_MAX                     12

#define MAT_ATTRIB_AMBIENT(f)  (MAT_ATTRIB_FRONT_AMBIENT+(f))  
#define MAT_ATTRIB_DIFFUSE(f)  (MAT_ATTRIB_FRONT_DIFFUSE+(f))  
#define MAT_ATTRIB_SPECULAR(f) (MAT_ATTRIB_FRONT_SPECULAR+(f)) 
#define MAT_ATTRIB_EMISSION(f) (MAT_ATTRIB_FRONT_EMISSION+(f)) 
#define MAT_ATTRIB_SHININESS(f)(MAT_ATTRIB_FRONT_SHININESS+(f))
#define MAT_ATTRIB_INDEXES(f)  (MAT_ATTRIB_FRONT_INDEXES+(f))  

#define MAT_INDEX_AMBIENT  0
#define MAT_INDEX_DIFFUSE  1
#define MAT_INDEX_SPECULAR 2

#define MAT_BIT_FRONT_AMBIENT         (1<<MAT_ATTRIB_FRONT_AMBIENT)
#define MAT_BIT_BACK_AMBIENT          (1<<MAT_ATTRIB_BACK_AMBIENT)
#define MAT_BIT_FRONT_DIFFUSE         (1<<MAT_ATTRIB_FRONT_DIFFUSE)
#define MAT_BIT_BACK_DIFFUSE          (1<<MAT_ATTRIB_BACK_DIFFUSE)
#define MAT_BIT_FRONT_SPECULAR        (1<<MAT_ATTRIB_FRONT_SPECULAR)
#define MAT_BIT_BACK_SPECULAR         (1<<MAT_ATTRIB_BACK_SPECULAR)
#define MAT_BIT_FRONT_EMISSION        (1<<MAT_ATTRIB_FRONT_EMISSION)
#define MAT_BIT_BACK_EMISSION         (1<<MAT_ATTRIB_BACK_EMISSION)
#define MAT_BIT_FRONT_SHININESS       (1<<MAT_ATTRIB_FRONT_SHININESS)
#define MAT_BIT_BACK_SHININESS        (1<<MAT_ATTRIB_BACK_SHININESS)
#define MAT_BIT_FRONT_INDEXES         (1<<MAT_ATTRIB_FRONT_INDEXES)
#define MAT_BIT_BACK_INDEXES          (1<<MAT_ATTRIB_BACK_INDEXES)


#define FRONT_MATERIAL_BITS	(MAT_BIT_FRONT_EMISSION | 	\
				 MAT_BIT_FRONT_AMBIENT |	\
				 MAT_BIT_FRONT_DIFFUSE | 	\
				 MAT_BIT_FRONT_SPECULAR |	\
				 MAT_BIT_FRONT_SHININESS | 	\
				 MAT_BIT_FRONT_INDEXES)

#define BACK_MATERIAL_BITS	(MAT_BIT_BACK_EMISSION |	\
				 MAT_BIT_BACK_AMBIENT |		\
				 MAT_BIT_BACK_DIFFUSE |		\
				 MAT_BIT_BACK_SPECULAR |	\
				 MAT_BIT_BACK_SHININESS |	\
				 MAT_BIT_BACK_INDEXES)

#define ALL_MATERIAL_BITS	(FRONT_MATERIAL_BITS | BACK_MATERIAL_BITS)
/*@}*/


/**
 * Light state flags.
 */
/*@{*/
#define LIGHT_SPOT         0x1
#define LIGHT_LOCAL_VIEWER 0x2
#define LIGHT_POSITIONAL   0x4
#define LIGHT_NEED_VERTICES (LIGHT_POSITIONAL|LIGHT_LOCAL_VIEWER)
/*@}*/





/**
 * Multisample attribute group (GL_MULTISAMPLE_BIT).
 */
struct gl_multisample_attrib
{
   GLboolean Enabled;
   GLboolean _Enabled;   /**< true if Enabled and multisample buffer */
   GLboolean SampleAlphaToCoverage;
   GLboolean SampleAlphaToOne;
   GLboolean SampleCoverage;
   GLfloat SampleCoverageValue;
   GLboolean SampleCoverageInvert;
   GLboolean SampleShading;
   GLfloat MinSampleShadingValue;

   /* ARB_texture_multisample / GL3.2 additions */
   GLboolean SampleMask;
   /** The GL spec defines this as an array but >32x MSAA is madness */
   GLbitfield SampleMaskValue;
};


/**
 * An index for each type of texture object.  These correspond to the GL
 * texture target enums, such as GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc.
 * Note: the order is from highest priority to lowest priority.
 */
typedef enum
{
   TEXTURE_2D_MULTISAMPLE_INDEX,
   TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX,
   TEXTURE_CUBE_ARRAY_INDEX,
   TEXTURE_BUFFER_INDEX,
   TEXTURE_2D_ARRAY_INDEX,
   TEXTURE_1D_ARRAY_INDEX,
   TEXTURE_EXTERNAL_INDEX,
   TEXTURE_CUBE_INDEX,
   TEXTURE_3D_INDEX,
   TEXTURE_RECT_INDEX,
   TEXTURE_2D_INDEX,
   TEXTURE_1D_INDEX,
   NUM_TEXTURE_TARGETS
} gl_texture_index;


/**
 * Bit flags for each type of texture object
 */
/*@{*/
#define TEXTURE_2D_MULTISAMPLE_BIT (1 << TEXTURE_2D_MULTISAMPLE_INDEX)
#define TEXTURE_2D_MULTISAMPLE_ARRAY_BIT (1 << TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX)
#define TEXTURE_CUBE_ARRAY_BIT (1 << TEXTURE_CUBE_ARRAY_INDEX)
#define TEXTURE_BUFFER_BIT   (1 << TEXTURE_BUFFER_INDEX)
#define TEXTURE_2D_ARRAY_BIT (1 << TEXTURE_2D_ARRAY_INDEX)
#define TEXTURE_1D_ARRAY_BIT (1 << TEXTURE_1D_ARRAY_INDEX)
#define TEXTURE_EXTERNAL_BIT (1 << TEXTURE_EXTERNAL_INDEX)
#define TEXTURE_CUBE_BIT     (1 << TEXTURE_CUBE_INDEX)
#define TEXTURE_3D_BIT       (1 << TEXTURE_3D_INDEX)
#define TEXTURE_RECT_BIT     (1 << TEXTURE_RECT_INDEX)
#define TEXTURE_2D_BIT       (1 << TEXTURE_2D_INDEX)
#define TEXTURE_1D_BIT       (1 << TEXTURE_1D_INDEX)
/*@}*/


/**
 * Indexes for cube map faces.
 */
typedef enum
{
   FACE_POS_X = 0,
   FACE_NEG_X = 1,
   FACE_POS_Y = 2,
   FACE_NEG_Y = 3,
   FACE_POS_Z = 4,
   FACE_NEG_Z = 5,
   MAX_FACES = 6
} gl_face_index;



/** Up to four combiner sources are possible with GL_NV_texture_env_combine4 */
#define MAX_COMBINER_TERMS 4


/**
 * Texture combine environment state.
 */
struct gl_tex_env_combine_state
{
   GLenum ModeRGB;       /**< GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   GLenum ModeA;         /**< GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   /** Source terms: GL_PRIMARY_COLOR, GL_TEXTURE, etc */
   GLenum SourceRGB[MAX_COMBINER_TERMS];
   GLenum SourceA[MAX_COMBINER_TERMS];
   /** Source operands: GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, etc */
   GLenum OperandRGB[MAX_COMBINER_TERMS];
   GLenum OperandA[MAX_COMBINER_TERMS];
   GLuint ScaleShiftRGB; /**< 0, 1 or 2 */
   GLuint ScaleShiftA;   /**< 0, 1 or 2 */
   GLuint _NumArgsRGB;   /**< Number of inputs used for the RGB combiner */
   GLuint _NumArgsA;     /**< Number of inputs used for the A combiner */
};


/**
 * TexGenEnabled flags.
 */
/*@{*/
#define S_BIT 1
#define T_BIT 2
#define R_BIT 4
#define Q_BIT 8
#define STR_BITS (S_BIT | T_BIT | R_BIT)
/*@}*/


/**
 * Bit flag versions of the corresponding GL_ constants.
 */
/*@{*/
#define TEXGEN_SPHERE_MAP        0x1
#define TEXGEN_OBJ_LINEAR        0x2
#define TEXGEN_EYE_LINEAR        0x4
#define TEXGEN_REFLECTION_MAP_NV 0x8
#define TEXGEN_NORMAL_MAP_NV     0x10

#define TEXGEN_NEED_NORMALS      (TEXGEN_SPHERE_MAP        | \
				  TEXGEN_REFLECTION_MAP_NV | \
				  TEXGEN_NORMAL_MAP_NV)
#define TEXGEN_NEED_EYE_COORD    (TEXGEN_SPHERE_MAP        | \
				  TEXGEN_REFLECTION_MAP_NV | \
				  TEXGEN_NORMAL_MAP_NV     | \
				  TEXGEN_EYE_LINEAR)
/*@}*/



/** Tex-gen enabled for texture unit? */
#define ENABLE_TEXGEN(unit) (1 << (unit))

/** Non-identity texture matrix for texture unit? */
#define ENABLE_TEXMAT(unit) (1 << (unit))

/**
 * Data structure representing a single clip plane (e.g. one of the elements
 * of the ctx->Transform.EyeUserPlane or ctx->Transform._ClipUserPlane array).
 */
typedef GLfloat gl_clip_plane[4];

typedef enum {
   MAP_USER,
   MAP_INTERNAL,

   MAP_COUNT
} gl_map_buffer_index;


/**
 * Fields describing a mapped buffer range.
 */

struct gl_transform_feedback_varying_info
{
   char *Name;
   GLenum Type;
   GLint Size;
};


/**
 * Per-output info vertex shaders for transform feedback.
 */
struct gl_transform_feedback_output
{
   unsigned OutputRegister;
   unsigned OutputBuffer;
   unsigned NumComponents;

   /** offset (in DWORDs) of this output within the interleaved structure */
   unsigned DstOffset;

   /**
    * Offset into the output register of the data to output.  For example,
    * if NumComponents is 2 and ComponentOffset is 1, then the data to
    * offset is in the y and z components of the output register.
    */
   unsigned ComponentOffset;
};


/** Post-link transform feedback info. */
struct gl_transform_feedback_info
{
   unsigned NumOutputs;

   /**
    * Number of transform feedback buffers in use by this program.
    */
   unsigned NumBuffers;

   struct gl_transform_feedback_output *Outputs;

   /** Transform feedback varyings used for the linking of this shader program.
    *
    * Use for glGetTransformFeedbackVarying().
    */
   struct gl_transform_feedback_varying_info *Varyings;
   GLint NumVarying;

   /**
    * Total number of components stored in each buffer.  This may be used by
    * hardware back-ends to determine the correct stride when interleaving
    * multiple transform feedback outputs in the same buffer.
    */
   unsigned BufferStride[MAX_FEEDBACK_BUFFERS];
};


/**
 * Names of the various vertex/fragment program register files, etc.
 *
 * NOTE: first four tokens must fit into 2 bits (see t_vb_arbprogram.c)
 * All values should fit in a 4-bit field.
 *
 * NOTE: PROGRAM_STATE_VAR, PROGRAM_CONSTANT, and PROGRAM_UNIFORM can all be
 * considered to be "uniform" variables since they can only be set outside
 * glBegin/End.  They're also all stored in the same Parameters array.
 */
typedef enum
{
   PROGRAM_TEMPORARY,   /**< machine->Temporary[] */
   PROGRAM_ARRAY,       /**< Arrays & Matrixes */
   PROGRAM_INPUT,       /**< machine->Inputs[] */
   PROGRAM_OUTPUT,      /**< machine->Outputs[] */
   PROGRAM_STATE_VAR,   /**< gl_program->Parameters[] */
   PROGRAM_CONSTANT,    /**< gl_program->Parameters[] */
   PROGRAM_UNIFORM,     /**< gl_program->Parameters[] */
   PROGRAM_WRITE_ONLY,  /**< A dummy, write-only register */
   PROGRAM_ADDRESS,     /**< machine->AddressReg */
   PROGRAM_SAMPLER,     /**< for shader samplers, compile-time only */
   PROGRAM_SYSTEM_VALUE,/**< InstanceId, PrimitiveID, etc. */
   PROGRAM_UNDEFINED,   /**< Invalid/TBD value */
   PROGRAM_FILE_MAX
} gl_register_file;


/**
 * If the register file is PROGRAM_SYSTEM_VALUE, the register index will be
 * one of these values.
 */
typedef enum
{
   SYSTEM_VALUE_FRONT_FACE,     /**< Fragment shader only (not done yet) */
   SYSTEM_VALUE_VERTEX_ID,      /**< Vertex shader only */
   SYSTEM_VALUE_INSTANCE_ID,    /**< Vertex shader only */
   SYSTEM_VALUE_SAMPLE_ID,      /**< Fragment shader only */
   SYSTEM_VALUE_SAMPLE_POS,     /**< Fragment shader only */
   SYSTEM_VALUE_SAMPLE_MASK_IN, /**< Fragment shader only */
   SYSTEM_VALUE_INVOCATION_ID,  /**< Geometry shader only */
   SYSTEM_VALUE_MAX             /**< Number of values */
} gl_system_value;


/**
 * The possible interpolation qualifiers that can be applied to a fragment
 * shader input in GLSL.
 *
 * Note: INTERP_QUALIFIER_NONE must be 0 so that memsetting the
 * gl_fragment_program data structure to 0 causes the default behavior.
 */
enum glsl_interp_qualifier
{
   INTERP_QUALIFIER_NONE = 0,
   INTERP_QUALIFIER_SMOOTH,
   INTERP_QUALIFIER_FLAT,
   INTERP_QUALIFIER_NOPERSPECTIVE,
   INTERP_QUALIFIER_COUNT /**< Number of interpolation qualifiers */
};


/**
 * \brief Layout qualifiers for gl_FragDepth.
 *
 * Extension AMD_conservative_depth allows gl_FragDepth to be redeclared with
 * a layout qualifier.
 *
 * \see enum ir_depth_layout
 */
enum gl_frag_depth_layout
{
   FRAG_DEPTH_LAYOUT_NONE, /**< No layout is specified. */
   FRAG_DEPTH_LAYOUT_ANY,
   FRAG_DEPTH_LAYOUT_GREATER,
   FRAG_DEPTH_LAYOUT_LESS,
   FRAG_DEPTH_LAYOUT_UNCHANGED
};


/**
 * Base class for any kind of program object
 */
struct gl_program
{
   GLuint Id;
   GLubyte *String;  /**< Null-terminated program text */
   GLint RefCount;
   GLenum Target;    /**< GL_VERTEX/FRAGMENT_PROGRAM_ARB, GL_GEOMETRY_PROGRAM_NV */
   GLenum Format;    /**< String encoding format */

   struct prog_instruction *Instructions;

   GLbitfield64 InputsRead;     /**< Bitmask of which input regs are read */
   GLbitfield64 OutputsWritten; /**< Bitmask of which output regs are written */
   GLbitfield SystemValuesRead;   /**< Bitmask of SYSTEM_VALUE_x inputs used */
   GLbitfield InputFlags[MAX_PROGRAM_INPUTS];   /**< PROG_PARAM_BIT_x flags */
   GLbitfield OutputFlags[MAX_PROGRAM_OUTPUTS]; /**< PROG_PARAM_BIT_x flags */
   GLbitfield TexturesUsed[MAX_COMBINED_TEXTURE_IMAGE_UNITS];  /**< TEXTURE_x_BIT bitmask */
   GLbitfield SamplersUsed;   /**< Bitfield of which samplers are used */
   GLbitfield ShadowSamplers; /**< Texture units used for shadow sampling. */

   GLboolean UsesGather; /**< Does this program use gather4 at all? */

   /**
    * For vertex and geometry shaders, true if the program uses the
    * gl_ClipDistance output.  Ignored for fragment shaders.
    */
   GLboolean UsesClipDistanceOut;


   /** Named parameters, constants, etc. from program text */
   struct gl_program_parameter_list *Parameters;

   /**
    * Local parameters used by the program.
    *
    * It's dynamically allocated because it is rarely used (just
    * assembly-style programs), and MAX_PROGRAM_LOCAL_PARAMS entries once it's
    * allocated.
    */
   GLfloat (*LocalParams)[4];

   /** Map from sampler unit to texture unit (set by glUniform1i()) */
   GLubyte SamplerUnits[MAX_SAMPLERS];

   /** Bitmask of which register files are read/written with indirect
    * addressing.  Mask of (1 << PROGRAM_x) bits.
    */
   GLbitfield IndirectRegisterFiles;

   /** Logical counts */
   /*@{*/
   GLuint NumInstructions;
   GLuint NumTemporaries;
   GLuint NumParameters;
   GLuint NumAttributes;
   GLuint NumAddressRegs;
   GLuint NumAluInstructions;
   GLuint NumTexInstructions;
   GLuint NumTexIndirections;
   /*@}*/
   /** Native, actual h/w counts */
   /*@{*/
   GLuint NumNativeInstructions;
   GLuint NumNativeTemporaries;
   GLuint NumNativeParameters;
   GLuint NumNativeAttributes;
   GLuint NumNativeAddressRegs;
   GLuint NumNativeAluInstructions;
   GLuint NumNativeTexInstructions;
   GLuint NumNativeTexIndirections;
   /*@}*/
};


/** Vertex program object */
struct gl_vertex_program
{
   struct gl_program Base;   /**< base class */
   GLboolean IsPositionInvariant;
};


/** Geometry program object */
struct gl_geometry_program
{
   struct gl_program Base;   /**< base class */

   GLint VerticesIn;
   GLint VerticesOut;
   GLint Invocations;
   GLenum InputType;  /**< GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_ARB,
                           GL_TRIANGLES, or GL_TRIANGLES_ADJACENCY_ARB */
   GLenum OutputType; /**< GL_POINTS, GL_LINE_STRIP or GL_TRIANGLE_STRIP */
   GLboolean UsesEndPrimitive;
};


/** Fragment program object */
struct gl_fragment_program
{
   struct gl_program Base;   /**< base class */
   GLboolean UsesKill;          /**< shader uses KIL instruction */
   GLboolean UsesDFdy;          /**< shader uses DDY instruction */
   GLboolean OriginUpperLeft;
   GLboolean PixelCenterInteger;
   enum gl_frag_depth_layout FragDepthLayout;

   /**
    * GLSL interpolation qualifier associated with each fragment shader input.
    * For inputs that do not have an interpolation qualifier specified in
    * GLSL, the value is INTERP_QUALIFIER_NONE.
    */
   enum glsl_interp_qualifier InterpQualifier[VARYING_SLOT_MAX];

   /**
    * Bitfield indicating, for each fragment shader input, 1 if that input
    * uses centroid interpolation, 0 otherwise.  Unused inputs are 0.
    */
   GLbitfield64 IsCentroid;

   /**
    * Bitfield indicating, for each fragment shader input, 1 if that input
    * uses sample interpolation, 0 otherwise.  Unused inputs are 0.
    */
   GLbitfield64 IsSample;
};


/** Compute program object */
struct gl_compute_program
{
   struct gl_program Base;   /**< base class */

   /**
    * Size specified using local_size_{x,y,z}.
    */
   unsigned LocalSize[3];
};


/**
 * State common to vertex and fragment programs.
 */
struct gl_program_state
{
   GLint ErrorPos;                       /* GL_PROGRAM_ERROR_POSITION_ARB/NV */
   const char *ErrorString;              /* GL_PROGRAM_ERROR_STRING_ARB/NV */
};


/**
 * Context state for vertex programs.
 */
struct gl_vertex_program_state
{
   GLboolean Enabled;            /**< User-set GL_VERTEX_PROGRAM_ARB/NV flag */
   GLboolean _Enabled;           /**< Enabled and _valid_ user program? */
   GLboolean PointSizeEnabled;   /**< GL_VERTEX_PROGRAM_POINT_SIZE_ARB/NV */
   GLboolean TwoSideEnabled;     /**< GL_VERTEX_PROGRAM_TWO_SIDE_ARB/NV */
   /** Computed two sided lighting for fixed function/programs. */
   GLboolean _TwoSideEnabled;
   struct gl_vertex_program *Current;  /**< User-bound vertex program */

   /** Currently enabled and valid vertex program (including internal
    * programs, user-defined vertex programs and GLSL vertex shaders).
    * This is the program we must use when rendering.
    */
   struct gl_vertex_program *_Current;

   GLfloat Parameters[MAX_PROGRAM_ENV_PARAMS][4]; /**< Env params */

   /** Should fixed-function T&L be implemented with a vertex prog? */
   GLboolean _MaintainTnlProgram;

   /** Program to emulate fixed-function T&L (see above) */
   struct gl_vertex_program *_TnlProgram;

   /** Cache of fixed-function programs */
   struct gl_program_cache *Cache;

   GLboolean _Overriden;
};


/**
 * Context state for geometry programs.
 */
struct gl_geometry_program_state
{
   GLboolean Enabled;               /**< GL_ARB_GEOMETRY_SHADER4 */
   GLboolean _Enabled;              /**< Enabled and valid program? */
   struct gl_geometry_program *Current;  /**< user-bound geometry program */

   /** Currently enabled and valid program (including internal programs
    * and compiled shader programs).
    */
   struct gl_geometry_program *_Current;

   GLfloat Parameters[MAX_PROGRAM_ENV_PARAMS][4]; /**< Env params */

   /** Cache of fixed-function programs */
   struct gl_program_cache *Cache;
};

/**
 * Context state for fragment programs.
 */
struct gl_fragment_program_state
{
   GLboolean Enabled;     /**< User-set fragment program enable flag */
   GLboolean _Enabled;    /**< Enabled and _valid_ user program? */
   struct gl_fragment_program *Current;  /**< User-bound fragment program */

   /** Currently enabled and valid fragment program (including internal
    * programs, user-defined fragment programs and GLSL fragment shaders).
    * This is the program we must use when rendering.
    */
   struct gl_fragment_program *_Current;

   GLfloat Parameters[MAX_PROGRAM_ENV_PARAMS][4]; /**< Env params */

   /** Should fixed-function texturing be implemented with a fragment prog? */
   GLboolean _MaintainTexEnvProgram;

   /** Program to emulate fixed-function texture env/combine (see above) */
   struct gl_fragment_program *_TexEnvProgram;

   /** Cache of fixed-function programs */
   struct gl_program_cache *Cache;
};


/**
 * ATI_fragment_shader runtime state
 */
#define ATI_FS_INPUT_PRIMARY 0
#define ATI_FS_INPUT_SECONDARY 1

struct atifs_instruction;
struct atifs_setupinst;

/**
 * ATI fragment shader
 */
struct ati_fragment_shader
{
   GLuint Id;
   GLint RefCount;
   struct atifs_instruction *Instructions[2];
   struct atifs_setupinst *SetupInst[2];
   GLfloat Constants[8][4];
   GLbitfield LocalConstDef;  /**< Indicates which constants have been set */
   GLubyte numArithInstr[2];
   GLubyte regsAssigned[2];
   GLubyte NumPasses;         /**< 1 or 2 */
   GLubyte cur_pass;
   GLubyte last_optype;
   GLboolean interpinp1;
   GLboolean isValid;
   GLuint swizzlerq;
};

/**
 * Context state for GL_ATI_fragment_shader
 */
struct gl_ati_fragment_shader_state
{
   GLboolean Enabled;
   GLboolean _Enabled;                  /**< enabled and valid shader? */
   GLboolean Compiling;
   GLfloat GlobalConstants[8][4];
   struct ati_fragment_shader *Current;
};


/** Set by #pragma directives */
struct gl_sl_pragmas
{
   GLboolean IgnoreOptimize;  /**< ignore #pragma optimize(on/off) ? */
   GLboolean IgnoreDebug;     /**< ignore #pragma debug(on/off) ? */
   GLboolean Optimize;  /**< defaults on */
   GLboolean Debug;     /**< defaults off */
};


/**
 * A GLSL vertex or fragment shader object.
 */
struct gl_shader
{
   /** GL_FRAGMENT_SHADER || GL_VERTEX_SHADER || GL_GEOMETRY_SHADER_ARB.
    * Must be the first field.
    */
   GLenum Type;
   gl_shader_stage Stage;
   GLuint Name;  /**< AKA the handle */
   GLchar *Label;   /**< GL_KHR_debug */
   GLint RefCount;  /**< Reference count */
   GLboolean DeletePending;
   GLboolean CompileStatus;
   const GLchar *Source;  /**< BIL or GLSL source code */
   GLuint Size;         /**< BIL size */
   GLuint SourceChecksum;       /**< for debug/logging purposes */
   struct gl_program *Program;  /**< Post-compile assembly code */
   GLchar *InfoLog;
   struct gl_sl_pragmas Pragmas;

   unsigned Version;       /**< GLSL version used for linking */
   GLboolean IsES;         /**< True if this shader uses GLSL ES */

   /**
    * \name Sampler tracking
    *
    * \note Each of these fields is only set post-linking.
    */
   /*@{*/
   unsigned num_samplers;	/**< Number of samplers used by this shader. */
   GLbitfield active_samplers;	/**< Bitfield of which samplers are used */
   GLbitfield shadow_samplers;	/**< Samplers used for shadow sampling. */
   /*@}*/

   /**
    * Map from sampler unit to texture unit (set by glUniform1i())
    *
    * A sampler unit is associated with each sampler uniform by the linker.
    * The sampler unit associated with each uniform is stored in the
    * \c gl_uniform_storage::sampler field.
    */
   GLubyte SamplerUnits[MAX_SAMPLERS];
   /** Which texture target is being sampled (TEXTURE_1D/2D/3D/etc_INDEX) */
   gl_texture_index SamplerTargets[MAX_SAMPLERS];

   /**
    * Number of default uniform block components used by this shader.
    *
    * This field is only set post-linking.
    */
   unsigned num_uniform_components;

   /**
    * Number of combined uniform components used by this shader.
    *
    * This field is only set post-linking.  It is the sum of the uniform block
    * sizes divided by sizeof(float), and num_uniform_compoennts.
    */
   unsigned num_combined_uniform_components;

   /**
    * This shader's uniform block information.
    *
    * These fields are only set post-linking.
    */
   struct gl_uniform_block *UniformBlocks;
   unsigned NumUniformBlocks;

   struct exec_list *ir;
   struct glsl_symbol_table *symbols;

   bool uses_builtin_functions;
   bool uses_gl_fragcoord;
   bool redeclares_gl_fragcoord;
   bool ARB_fragment_coord_conventions_enable;

   /**
    * Fragment shader state from GLSL 1.50 layout qualifiers.
    */
   bool origin_upper_left;
   bool pixel_center_integer;

   /**
    * Geometry shader state from GLSL 1.50 layout qualifiers.
    */
   struct {
      GLint VerticesOut;
      /**
       * 0 - Invocations count not declared in shader, or
       * 1 .. MAX_GEOMETRY_SHADER_INVOCATIONS
       */
      GLint Invocations;
      /**
       * GL_POINTS, GL_LINES, GL_LINES_ADJACENCY, GL_TRIANGLES, or
       * GL_TRIANGLES_ADJACENCY, or PRIM_UNKNOWN if it's not set in this
       * shader.
       */
      GLenum InputType;
       /**
        * GL_POINTS, GL_LINE_STRIP or GL_TRIANGLE_STRIP, or PRIM_UNKNOWN if
        * it's not set in this shader.
        */
      GLenum OutputType;
   } Geom;

   /**
    * Map from image uniform index to image unit (set by glUniform1i())
    *
    * An image uniform index is associated with each image uniform by
    * the linker.  The image index associated with each uniform is
    * stored in the \c gl_uniform_storage::image field.
    */
   GLubyte ImageUnits[MAX_IMAGE_UNIFORMS];

   /**
    * Access qualifier specified in the shader for each image uniform
    * index.  Either \c GL_READ_ONLY, \c GL_WRITE_ONLY or \c
    * GL_READ_WRITE.
    *
    * It may be different, though only more strict than the value of
    * \c gl_image_unit::Access for the corresponding image unit.
    */
   GLenum ImageAccess[MAX_IMAGE_UNIFORMS];

   /**
    * Number of image uniforms defined in the shader.  It specifies
    * the number of valid elements in the \c ImageUnits and \c
    * ImageAccess arrays above.
    */
   GLuint NumImages;

   /**
    * Compute shader state from ARB_compute_shader layout qualifiers.
    */
   struct {
      /**
       * Size specified using local_size_{x,y,z}, or all 0's to indicate that
       * it's not set in this shader.
       */
      unsigned LocalSize[3];
   } Comp;

   /**
    * Deferred task of glCompileShader.  We should extend the mutex, not only
    * to protect the deferred task, but to protect the entire gl_shader.
    *
    * MUST BE LAST FOR SHADER CACHE TO WORK
    */
   mtx_t Mutex;
   struct _mesa_threadpool_task *Task;
   void *TaskData;
};


struct gl_uniform_buffer_variable
{
   char *Name;

   /**
    * Name of the uniform as seen by glGetUniformIndices.
    *
    * glGetUniformIndices requires that the block instance index \b not be
    * present in the name of queried uniforms.
    *
    * \note
    * \c gl_uniform_buffer_variable::IndexName and
    * \c gl_uniform_buffer_variable::Name may point to identical storage.
    */
   char *IndexName;

   const struct glsl_type *Type;
   unsigned int Offset;
   GLboolean RowMajor;
};


enum gl_uniform_block_packing
{
   ubo_packing_std140,
   ubo_packing_shared,
   ubo_packing_packed
};


struct gl_uniform_block
{
   /** Declared name of the uniform block */
   char *Name;

   /** Array of supplemental information about UBO ir_variables. */
   struct gl_uniform_buffer_variable *Uniforms;
   GLuint NumUniforms;

   /**
    * Index (GL_UNIFORM_BLOCK_BINDING) into ctx->UniformBufferBindings[] to use
    * with glBindBufferBase to bind a buffer object to this uniform block.  When
    * updated in the program, _NEW_BUFFER_OBJECT will be set.
    */
   GLuint Binding;

   /**
    * Minimum size of a buffer object to back this uniform buffer
    * (GL_UNIFORM_BLOCK_DATA_SIZE).
    */
   GLuint UniformBufferSize;

   /**
    * Layout specified in the shader
    *
    * This isn't accessible through the API, but it is used while
    * cross-validating uniform blocks.
    */
   enum gl_uniform_block_packing _Packing;
};

/**
 * Structure that represents a reference to an atomic buffer from some
 * shader program.
 */
struct gl_active_atomic_buffer
{
   /** Uniform indices of the atomic counters declared within it. */
   GLuint *Uniforms;
   GLuint NumUniforms;

   /** Binding point index associated with it. */
   GLuint Binding;

   /** Minimum reasonable size it is expected to have. */
   GLuint MinimumSize;

   /** Shader stages making use of it. */
   GLboolean StageReferences[MESA_SHADER_STAGES];
};

/**
 * A GLSL program object.
 * Basically a linked collection of vertex and fragment shaders.
 */
struct gl_shader_program
{
   GLenum Type;  /**< Always GL_SHADER_PROGRAM (internal token) */
   GLuint Name;  /**< aka handle or ID */
   GLchar *Label;   /**< GL_KHR_debug */
   GLint RefCount;  /**< Reference count */
   GLboolean DeletePending;

   /**
    * Is the application intending to glGetProgramBinary this program?
    */
   GLboolean BinaryRetreivableHint;

   /**
    * Indicates whether program can be bound for individual pipeline stages
    * using UseProgramStages after it is next linked.
    */
   GLboolean SeparateShader;

   GLuint NumShaders;          /**< number of attached shaders */
   struct gl_shader **Shaders; /**< List of attached the shaders */

   /**
    * User-defined attribute bindings
    *
    * These are set via \c glBindAttribLocation and are used to direct the
    * GLSL linker.  These are \b not the values used in the compiled shader,
    * and they are \b not the values returned by \c glGetAttribLocation.
    */
   struct string_to_uint_map *AttributeBindings;

   /**
    * User-defined fragment data bindings
    *
    * These are set via \c glBindFragDataLocation and are used to direct the
    * GLSL linker.  These are \b not the values used in the compiled shader,
    * and they are \b not the values returned by \c glGetFragDataLocation.
    */
   struct string_to_uint_map *FragDataBindings;
   struct string_to_uint_map *FragDataIndexBindings;

   /**
    * Transform feedback varyings last specified by
    * glTransformFeedbackVaryings().
    *
    * For the current set of transform feeedback varyings used for transform
    * feedback output, see LinkedTransformFeedback.
    */
   struct {
      GLenum BufferMode;
      GLuint NumVarying;
      GLchar **VaryingNames;  /**< Array [NumVarying] of char * */
   } TransformFeedback;

   /** Post-link transform feedback info. */
   struct gl_transform_feedback_info LinkedTransformFeedback;

   /** Post-link gl_FragDepth layout for ARB_conservative_depth. */
   enum gl_frag_depth_layout FragDepthLayout;

   /**
    * Geometry shader state - copied into gl_geometry_program by
    * _mesa_copy_linked_program_data().
    */
   struct {
      GLint VerticesIn;
      GLint VerticesOut;
      /**
       * 1 .. MAX_GEOMETRY_SHADER_INVOCATIONS
       */
      GLint Invocations;
      GLenum InputType;  /**< GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_ARB,
                              GL_TRIANGLES, or GL_TRIANGLES_ADJACENCY_ARB */
      GLenum OutputType; /**< GL_POINTS, GL_LINE_STRIP or GL_TRIANGLE_STRIP */
      /**
       * True if gl_ClipDistance is written to.  Copied into
       * gl_geometry_program by _mesa_copy_linked_program_data().
       */
      GLboolean UsesClipDistance;
      GLuint ClipDistanceArraySize; /**< Size of the gl_ClipDistance array, or
                                         0 if not present. */
      GLboolean UsesEndPrimitive;
   } Geom;

   /** Vertex shader state */
   struct {
      /**
       * True if gl_ClipDistance is written to.  Copied into gl_vertex_program
       * by _mesa_copy_linked_program_data().
       */
      GLboolean UsesClipDistance;
      GLuint ClipDistanceArraySize; /**< Size of the gl_ClipDistance array, or
                                         0 if not present. */
   } Vert;

   /**
    * Compute shader state - copied into gl_compute_program by
    * _mesa_copy_linked_program_data().
    */
   struct {
      /**
       * If this shader contains a compute stage, size specified using
       * local_size_{x,y,z}.  Otherwise undefined.
       */
      unsigned LocalSize[3];
   } Comp;

   /* post-link info: */
   unsigned NumUserUniformStorage;
   struct gl_uniform_storage *UniformStorage;

   /**
    * Mapping from GL uniform locations returned by \c glUniformLocation to
    * UniformStorage entries. Arrays will have multiple contiguous slots
    * in the UniformRemapTable, all pointing to the same UniformStorage entry.
    */
   unsigned NumUniformRemapTable;
   struct gl_uniform_storage **UniformRemapTable;

   /**
    * Size of the gl_ClipDistance array that is output from the last pipeline
    * stage before the fragment shader.
    */
   unsigned LastClipDistanceArraySize;

   struct gl_uniform_block *UniformBlocks;
   unsigned NumUniformBlocks;

   /**
    * Indices into the _LinkedShaders's UniformBlocks[] array for each stage
    * they're used in, or -1.
    *
    * This is used to maintain the Binding values of the stage's UniformBlocks[]
    * and to answer the GL_UNIFORM_BLOCK_REFERENCED_BY_*_SHADER queries.
    */
   int *UniformBlockStageIndex[MESA_SHADER_STAGES];

   /**
    * Map of active uniform names to locations
    *
    * Maps any active uniform that is not an array element to a location.
    * Each active uniform, including individual structure members will appear
    * in this map.  This roughly corresponds to the set of names that would be
    * enumerated by \c glGetActiveUniform.
    */
   struct string_to_uint_map *UniformHash;

   struct gl_active_atomic_buffer *AtomicBuffers;
   unsigned NumAtomicBuffers;

   GLboolean LinkStatus;   /**< GL_LINK_STATUS */
   GLboolean Validated;
   GLboolean _Used;        /**< Ever used for drawing? */
   GLboolean _Linked;      /**< Ever linked? */
   GLchar *InfoLog;

   unsigned Version;       /**< GLSL version used for linking */
   GLboolean IsES;         /**< True if this program uses GLSL ES */

   /**
    * Per-stage shaders resulting from the first stage of linking.
    *
    * Set of linked shaders for this program.  The array is accessed using the
    * \c MESA_SHADER_* defines.  Entries for non-existent stages will be
    * \c NULL.
    */
   struct gl_shader *_LinkedShaders[MESA_SHADER_STAGES];

   /* True if any of the fragment shaders attached to this program use:
    * #extension ARB_fragment_coord_conventions: enable
    */
   GLboolean ARB_fragment_coord_conventions_enable;

   /**
    * Deferred task of glLinkProgram.  We should extend the mutex, not only
    * to protect the deferred task, but to protect the entire
    * gl_shader_program.
    *
    * MUST BE LAST FOR SHADER CACHE TO WORK
    */
   mtx_t Mutex;
   struct _mesa_threadpool_task *Task;
   void *TaskData;
};   


#define GLSL_DUMP      0x1  /**< Dump shaders to stdout */
#define GLSL_LOG       0x2  /**< Write shaders to files */
#define GLSL_OPT       0x4  /**< Force optimizations (override pragmas) */
#define GLSL_NO_OPT    0x8  /**< Force no optimizations (override pragmas) */
#define GLSL_UNIFORMS 0x10  /**< Print glUniform calls */
#define GLSL_NOP_VERT 0x20  /**< Force no-op vertex shaders */
#define GLSL_NOP_FRAG 0x40  /**< Force no-op fragment shaders */
#define GLSL_USE_PROG 0x80  /**< Log glUseProgram calls */
#define GLSL_REPORT_ERRORS 0x100  /**< Print compilation errors */
#define GLSL_DUMP_ON_ERROR 0x200 /**< Dump shaders to stderr on compile error */
#define GLSL_USE_GLASS     0x400 /**< Use LunarGlass optimizer */

/**
 * Compiler options for a single GLSL shaders type
 */
struct gl_shader_compiler_options
{
   /** Driver-selectable options: */
   GLboolean EmitCondCodes;             /**< Use condition codes? */
   GLboolean EmitNoLoops;
   GLboolean EmitNoFunctions;
   GLboolean EmitNoCont;                  /**< Emit CONT opcode? */
   GLboolean EmitNoMainReturn;            /**< Emit CONT/RET opcodes? */
   GLboolean EmitNoNoise;                 /**< Emit NOISE opcodes? */
   GLboolean EmitNoPow;                   /**< Emit POW opcodes? */
   GLboolean LowerClipDistance; /**< Lower gl_ClipDistance from float[8] to vec4[2]? */

   /**
    * \name Forms of indirect addressing the driver cannot do.
    */
   /*@{*/
   GLboolean EmitNoIndirectInput;   /**< No indirect addressing of inputs */
   GLboolean EmitNoIndirectOutput;  /**< No indirect addressing of outputs */
   GLboolean EmitNoIndirectTemp;    /**< No indirect addressing of temps */
   GLboolean EmitNoIndirectUniform; /**< No indirect addressing of constants */
   /*@}*/

   GLuint MaxIfDepth;               /**< Maximum nested IF blocks */
   GLuint MaxUnrollIterations;

   /**
    * Optimize code for array of structures backends.
    *
    * This is a proxy for:
    *   - preferring DP4 instructions (rather than MUL/MAD) for
    *     matrix * vector operations, such as position transformation.
    */
   GLboolean OptimizeForAOS;

   struct gl_sl_pragmas DefaultPragmas; /**< Default #pragma settings */
};

/**
 * State which can be shared by multiple contexts:
 */
struct gl_shared_state
{
   mtx_t Mutex;		   /**< for thread safety */
   GLint RefCount;			   /**< Reference count */

   /**
    * \name Vertex/geometry/fragment programs
    */
   /*@{*/
   struct _mesa_HashTable *Programs; /**< All vertex/fragment programs */
   struct gl_vertex_program *DefaultVertexProgram;
   struct gl_fragment_program *DefaultFragmentProgram;
   struct gl_geometry_program *DefaultGeometryProgram;
   /*@}*/

   /* GL_ATI_fragment_shader */
   struct _mesa_HashTable *ATIShaders;
   struct ati_fragment_shader *DefaultFragmentShader;

   /** Table of both gl_shader and gl_shader_program objects */
   struct _mesa_HashTable *ShaderObjects;

   /**
    * Some context in this share group was affected by a GPU reset
    *
    * On the next call to \c glGetGraphicsResetStatus, contexts that have not
    * been affected by a GPU reset must also return
    * \c GL_INNOCENT_CONTEXT_RESET_ARB.
    *
    * Once this field becomes true, it is never reset to false.
    */
   bool ShareGroupReset;
};



/**
 * Precision info for shader datatypes.  See glGetShaderPrecisionFormat().
 */
struct gl_precision
{
   GLushort RangeMin;   /**< min value exponent */
   GLushort RangeMax;   /**< max value exponent */
   GLushort Precision;  /**< number of mantissa bits */
};


/**
 * Limits for vertex, geometry and fragment programs/shaders.
 */
struct gl_program_constants
{
   /* logical limits */
   GLuint MaxInstructions;
   GLuint MaxAluInstructions;
   GLuint MaxTexInstructions;
   GLuint MaxTexIndirections;
   GLuint MaxAttribs;
   GLuint MaxTemps;
   GLuint MaxAddressRegs;
   GLuint MaxAddressOffset;  /**< [-MaxAddressOffset, MaxAddressOffset-1] */
   GLuint MaxParameters;
   GLuint MaxLocalParams;
   GLuint MaxEnvParams;
   /* native/hardware limits */
   GLuint MaxNativeInstructions;
   GLuint MaxNativeAluInstructions;
   GLuint MaxNativeTexInstructions;
   GLuint MaxNativeTexIndirections;
   GLuint MaxNativeAttribs;
   GLuint MaxNativeTemps;
   GLuint MaxNativeAddressRegs;
   GLuint MaxNativeParameters;
   /* For shaders */
   GLuint MaxUniformComponents;  /**< Usually == MaxParameters * 4 */

   /**
    * \name Per-stage input / output limits
    *
    * Previous to OpenGL 3.2, the intrastage data limits were advertised with
    * a single value: GL_MAX_VARYING_COMPONENTS (GL_MAX_VARYING_VECTORS in
    * ES).  This is stored as \c gl_constants::MaxVarying.
    *
    * Starting with OpenGL 3.2, the limits are advertised with per-stage
    * variables.  Each stage as a certain number of outputs that it can feed
    * to the next stage and a certain number inputs that it can consume from
    * the previous stage.
    *
    * Vertex shader inputs do not participate this in this accounting.
    * These are tracked exclusively by \c gl_program_constants::MaxAttribs.
    *
    * Fragment shader outputs do not participate this in this accounting.
    * These are tracked exclusively by \c gl_constants::MaxDrawBuffers.
    */
   /*@{*/
   GLuint MaxInputComponents;
   GLuint MaxOutputComponents;
   /*@}*/

   /* ES 2.0 and GL_ARB_ES2_compatibility */
   struct gl_precision LowFloat, MediumFloat, HighFloat;
   struct gl_precision LowInt, MediumInt, HighInt;
   /* GL_ARB_uniform_buffer_object */
   GLuint MaxUniformBlocks;
   GLuint MaxCombinedUniformComponents;
   GLuint MaxTextureImageUnits;

   /* GL_ARB_shader_atomic_counters */
   GLuint MaxAtomicBuffers;
   GLuint MaxAtomicCounters;

   /* GL_ARB_shader_image_load_store */
   GLuint MaxImageUniforms;
};


/**
 * Constants which may be overridden by device driver during context creation
 * but are never changed after that.
 */
struct gl_constants
{
   GLuint MaxTextureMbytes;      /**< Max memory per image, in MB */
   GLuint MaxTextureLevels;      /**< Max mipmap levels. */ 
   GLuint Max3DTextureLevels;    /**< Max mipmap levels for 3D textures */
   GLuint MaxCubeTextureLevels;  /**< Max mipmap levels for cube textures */
   GLuint MaxArrayTextureLayers; /**< Max layers in array textures */
   GLuint MaxTextureRectSize;    /**< Max rectangle texture size, in pixes */
   GLuint MaxTextureCoordUnits;
   GLuint MaxCombinedTextureImageUnits;
   GLuint MaxTextureUnits; /**< = MIN(CoordUnits, FragmentProgram.ImageUnits) */
   GLfloat MaxTextureMaxAnisotropy;  /**< GL_EXT_texture_filter_anisotropic */
   GLfloat MaxTextureLodBias;        /**< GL_EXT_texture_lod_bias */
   GLuint MaxTextureBufferSize;      /**< GL_ARB_texture_buffer_object */

   GLuint TextureBufferOffsetAlignment; /**< GL_ARB_texture_buffer_range */

   GLuint MaxArrayLockSize;

   GLint SubPixelBits;

   GLfloat MinPointSize, MaxPointSize;	     /**< aliased */
   GLfloat MinPointSizeAA, MaxPointSizeAA;   /**< antialiased */
   GLfloat PointSizeGranularity;
   GLfloat MinLineWidth, MaxLineWidth;       /**< aliased */
   GLfloat MinLineWidthAA, MaxLineWidthAA;   /**< antialiased */
   GLfloat LineWidthGranularity;

   GLuint MaxClipPlanes;
   GLuint MaxLights;
   GLfloat MaxShininess;                     /**< GL_NV_light_max_exponent */
   GLfloat MaxSpotExponent;                  /**< GL_NV_light_max_exponent */

   GLuint MaxViewportWidth, MaxViewportHeight;
   GLuint MaxViewports;                      /**< GL_ARB_viewport_array */
   GLuint ViewportSubpixelBits;              /**< GL_ARB_viewport_array */
   struct {
      GLfloat Min;
      GLfloat Max;
   } ViewportBounds;                         /**< GL_ARB_viewport_array */

   struct gl_program_constants Program[MESA_SHADER_STAGES];
   GLuint MaxProgramMatrices;
   GLuint MaxProgramMatrixStackDepth;

   struct {
      GLuint SamplesPassed;
      GLuint TimeElapsed;
      GLuint Timestamp;
      GLuint PrimitivesGenerated;
      GLuint PrimitivesWritten;
   } QueryCounterBits;

   /** vertex array / buffer object bounds checking */
   GLboolean CheckArrayBounds;

   GLuint MaxDrawBuffers;    /**< GL_ARB_draw_buffers */

   GLuint MaxColorAttachments;   /**< GL_EXT_framebuffer_object */
   GLuint MaxRenderbufferSize;   /**< GL_EXT_framebuffer_object */
   GLuint MaxSamples;            /**< GL_ARB_framebuffer_object */

   /** Number of varying vectors between any two shader stages. */
   GLuint MaxVarying;

   /** @{
    * GL_ARB_uniform_buffer_object
    */
   GLuint MaxCombinedUniformBlocks;
   GLuint MaxUniformBufferBindings;
   GLuint MaxUniformBlockSize;
   GLuint UniformBufferOffsetAlignment;
   /** @} */

   /** GL_ARB_geometry_shader4 */
   GLuint MaxGeometryOutputVertices;
   GLuint MaxGeometryTotalOutputComponents;

   GLuint GLSLVersion;  /**< GLSL version supported (ex: 120 = 1.20) */

   /**
    * Changes default GLSL extension behavior from "error" to "warn".  It's out
    * of spec, but it can make some apps work that otherwise wouldn't.
    */
   GLboolean ForceGLSLExtensionsWarn;

   /**
    * If non-zero, forces GLSL shaders without the #version directive to behave
    * as if they began with "#version ForceGLSLVersion".
    */
   GLuint ForceGLSLVersion;

   /**
    * LunarGlass optimizer mode:
    * 0 = never use (force disable)
    * 1 = use driver whitelist
    * 2 = always use (force enable)
    */
   GLuint GlassMode;

   /**
    * LunarGlass optimization flags:
    * This is just one for now, but more should be added.
    */
   GLboolean GlassEnableReassociation;

   /**
    * Does the driver support real 32-bit integers?  (Otherwise, integers are
    * simulated via floats.)
    */
   GLboolean NativeIntegers;

   /**
    * If the driver supports real 32-bit integers, what integer value should be
    * used for boolean true in uniform uploads?  (Usually 1 or ~0.)
    */
   GLuint UniformBooleanTrue;

   /** Which texture units support GL_ATI_envmap_bumpmap as targets */
   GLbitfield SupportedBumpUnits;

   /**
    * Maximum amount of time, measured in nanseconds, that the server can wait.
    */
   GLuint64 MaxServerWaitTimeout;

   /** GL_EXT_provoking_vertex */
   GLboolean QuadsFollowProvokingVertexConvention;

   /** OpenGL version 3.0 */
   GLbitfield ContextFlags;  /**< Ex: GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT */

   /** OpenGL version 3.2 */
   GLbitfield ProfileMask;   /**< Mask of CONTEXT_x_PROFILE_BIT */

   /** GL_EXT_transform_feedback */
   GLuint MaxTransformFeedbackBuffers;
   GLuint MaxTransformFeedbackSeparateComponents;
   GLuint MaxTransformFeedbackInterleavedComponents;
   GLuint MaxVertexStreams;

   /** GL_EXT_gpu_shader4 */
   GLint MinProgramTexelOffset, MaxProgramTexelOffset;

   /** GL_ARB_texture_gather */
   GLuint MinProgramTextureGatherOffset;
   GLuint MaxProgramTextureGatherOffset;
   GLuint MaxProgramTextureGatherComponents;

   /* GL_ARB_robustness */
   GLenum ResetStrategy;

   /* GL_ARB_blend_func_extended */
   GLuint MaxDualSourceDrawBuffers;

   /**
    * Whether the implementation strips out and ignores texture borders.
    *
    * Many GPU hardware implementations don't support rendering with texture
    * borders and mipmapped textures.  (Note: not static border color, but the
    * old 1-pixel border around each edge).  Implementations then have to do
    * slow fallbacks to be correct, or just ignore the border and be fast but
    * wrong.  Setting the flag strips the border off of TexImage calls,
    * providing "fast but wrong" at significantly reduced driver complexity.
    *
    * Texture borders are deprecated in GL 3.0.
    **/
   GLboolean StripTextureBorder;

   /**
    * For drivers which can do a better job at eliminating unused uniforms
    * than the GLSL compiler.
    *
    * XXX Remove these as soon as a better solution is available.
    */
   GLboolean GLSLSkipStrictMaxUniformLimitCheck;

   /**
    * Force software support for primitive restart in the VBO module.
    */
   GLboolean PrimitiveRestartInSoftware;

   /**
    * Always use the GetTransformFeedbackVertexCount() driver hook, rather
    * than passing the transform feedback object to the drawing function.
    */
   GLboolean AlwaysUseGetTransformFeedbackVertexCount;

   /** GL_ARB_map_buffer_alignment */
   GLuint MinMapBufferAlignment;

   /**
    * Disable varying packing.  This is out of spec, but potentially useful
    * for older platforms that supports a limited number of texture
    * indirections--on these platforms, unpacking the varyings in the fragment
    * shader increases the number of texture indirections by 1, which might
    * make some shaders not executable at all.
    *
    * Drivers that support transform feedback must set this value to GL_FALSE.
    */
   GLboolean DisableVaryingPacking;

   /*
    * Maximum value supported for an index in DrawElements and friends.
    *
    * This must be at least (1ull<<24)-1.  The default value is
    * (1ull<<32)-1.
    *
    * \since ES 3.0 or GL_ARB_ES3_compatibility
    * \sa _mesa_init_constants
    */
   GLuint64 MaxElementIndex;

   /**
    * Disable interpretation of line continuations (lines ending with a
    * backslash character ('\') in GLSL source.
    */
   GLboolean DisableGLSLLineContinuations;

   /** GL_ARB_texture_multisample */
   GLint MaxColorTextureSamples;
   GLint MaxDepthTextureSamples;
   GLint MaxIntegerSamples;

   /** GL_ARB_shader_atomic_counters */
   GLuint MaxAtomicBufferBindings;
   GLuint MaxAtomicBufferSize;
   GLuint MaxCombinedAtomicBuffers;
   GLuint MaxCombinedAtomicCounters;

   /** GL_ARB_vertex_attrib_binding */
   GLint MaxVertexAttribRelativeOffset;
   GLint MaxVertexAttribBindings;

   /* GL_ARB_shader_image_load_store */
   GLuint MaxImageUnits;
   GLuint MaxCombinedImageUnitsAndFragmentOutputs;
   GLuint MaxImageSamples;
   GLuint MaxCombinedImageUniforms;

   /** GL_ARB_compute_shader */
   GLuint MaxComputeWorkGroupCount[3]; /* Array of x, y, z dimensions */
   GLuint MaxComputeWorkGroupSize[3]; /* Array of x, y, z dimensions */
   GLuint MaxComputeWorkGroupInvocations;

   /** GL_ARB_gpu_shader5 */
   GLfloat MinFragmentInterpolationOffset;
   GLfloat MaxFragmentInterpolationOffset;

   GLboolean FakeSWMSAA;

   /*
    * Defer certain operations to a thread pool.
    *
    * When DeferLinkProgram is set, these functions must be thread-safe
    *
    *   ctx->Driver.NewShader
    *   ctx->Driver.DeleteShader
    *   ctx->Driver.LinkShader
    */
   GLboolean DeferCompileShader;
   GLboolean DeferLinkProgram;

   /* The following is used to limit both program and shader cache sizes */
   /* If set to 0, it will disable caching */
   GLuint MaxShaderCacheSize;
};


/**
 * Enable flag for each OpenGL extension.  Different device drivers will
 * enable different extensions at runtime.
 */
struct gl_extensions
{
   GLboolean dummy;  /* don't remove this! */
   GLboolean dummy_true;  /* Set true by _mesa_init_extensions(). */
   GLboolean dummy_false; /* Set false by _mesa_init_extensions(). */
   GLboolean ANGLE_texture_compression_dxt;
   GLboolean ARB_ES2_compatibility;
   GLboolean ARB_ES3_compatibility;
   GLboolean ARB_arrays_of_arrays;
   GLboolean ARB_base_instance;
   GLboolean ARB_blend_func_extended;
   GLboolean ARB_buffer_storage;
   GLboolean ARB_color_buffer_float;
   GLboolean ARB_compute_shader;
   GLboolean ARB_conservative_depth;
   GLboolean ARB_depth_buffer_float;
   GLboolean ARB_depth_clamp;
   GLboolean ARB_depth_texture;
   GLboolean ARB_draw_buffers_blend;
   GLboolean ARB_draw_elements_base_vertex;
   GLboolean ARB_draw_indirect;
   GLboolean ARB_draw_instanced;
   GLboolean ARB_fragment_coord_conventions;
   GLboolean ARB_fragment_program;
   GLboolean ARB_fragment_program_shadow;
   GLboolean ARB_fragment_shader;
   GLboolean ARB_framebuffer_object;
   GLboolean ARB_explicit_attrib_location;
   GLboolean ARB_geometry_shader4;
   GLboolean ARB_gpu_shader5;
   GLboolean ARB_half_float_vertex;
   GLboolean ARB_instanced_arrays;
   GLboolean ARB_internalformat_query;
   GLboolean ARB_map_buffer_range;
   GLboolean ARB_occlusion_query;
   GLboolean ARB_occlusion_query2;
   GLboolean ARB_point_sprite;
   GLboolean ARB_sample_shading;
   GLboolean ARB_seamless_cube_map;
   GLboolean ARB_shader_atomic_counters;
   GLboolean ARB_shader_bit_encoding;
   GLboolean ARB_shader_image_load_store;
   GLboolean ARB_shader_stencil_export;
   GLboolean ARB_shader_texture_lod;
   GLboolean ARB_shading_language_packing;
   GLboolean ARB_shading_language_420pack;
   GLboolean ARB_shadow;
   GLboolean ARB_stencil_texturing;
   GLboolean ARB_sync;
   GLboolean ARB_texture_border_clamp;
   GLboolean ARB_texture_buffer_object;
   GLboolean ARB_texture_buffer_object_rgb32;
   GLboolean ARB_texture_buffer_range;
   GLboolean ARB_texture_compression_rgtc;
   GLboolean ARB_texture_cube_map;
   GLboolean ARB_texture_cube_map_array;
   GLboolean ARB_texture_env_combine;
   GLboolean ARB_texture_env_crossbar;
   GLboolean ARB_texture_env_dot3;
   GLboolean ARB_texture_float;
   GLboolean ARB_texture_gather;
   GLboolean ARB_texture_mirror_clamp_to_edge;
   GLboolean ARB_texture_multisample;
   GLboolean ARB_texture_non_power_of_two;
   GLboolean ARB_texture_stencil8;
   GLboolean ARB_texture_query_levels;
   GLboolean ARB_texture_query_lod;
   GLboolean ARB_texture_rg;
   GLboolean ARB_texture_rgb10_a2ui;
   GLboolean ARB_texture_view;
   GLboolean ARB_timer_query;
   GLboolean ARB_transform_feedback2;
   GLboolean ARB_transform_feedback3;
   GLboolean ARB_transform_feedback_instanced;
   GLboolean ARB_uniform_buffer_object;
   GLboolean ARB_vertex_program;
   GLboolean ARB_vertex_shader;
   GLboolean ARB_vertex_type_10f_11f_11f_rev;
   GLboolean ARB_vertex_type_2_10_10_10_rev;
   GLboolean ARB_viewport_array;
   GLboolean EXT_blend_color;
   GLboolean EXT_blend_equation_separate;
   GLboolean EXT_blend_func_separate;
   GLboolean EXT_blend_minmax;
   GLboolean EXT_depth_bounds_test;
   GLboolean EXT_draw_buffers2;
   GLboolean EXT_framebuffer_multisample;
   GLboolean EXT_framebuffer_multisample_blit_scaled;
   GLboolean EXT_framebuffer_sRGB;
   GLboolean EXT_gpu_program_parameters;
   GLboolean EXT_gpu_shader4;
   GLboolean EXT_packed_float;
   GLboolean EXT_pixel_buffer_object;
   GLboolean EXT_point_parameters;
   GLboolean EXT_provoking_vertex;
   GLboolean EXT_shader_integer_mix;
   GLboolean EXT_stencil_two_side;
   GLboolean EXT_texture3D;
   GLboolean EXT_texture_array;
   GLboolean EXT_texture_compression_latc;
   GLboolean EXT_texture_compression_s3tc;
   GLboolean EXT_texture_env_dot3;
   GLboolean EXT_texture_filter_anisotropic;
   GLboolean EXT_texture_integer;
   GLboolean EXT_texture_mirror_clamp;
   GLboolean EXT_texture_shared_exponent;
   GLboolean EXT_texture_snorm;
   GLboolean EXT_texture_sRGB;
   GLboolean EXT_texture_sRGB_decode;
   GLboolean EXT_texture_swizzle;
   GLboolean EXT_transform_feedback;
   GLboolean EXT_timer_query;
   GLboolean EXT_vertex_array_bgra;
   GLboolean OES_standard_derivatives;
   /* vendor extensions */
   GLboolean AMD_performance_monitor;
   GLboolean AMD_seamless_cubemap_per_texture;
   GLboolean AMD_vertex_shader_layer;
   GLboolean APPLE_object_purgeable;
   GLboolean ATI_envmap_bumpmap;
   GLboolean ATI_texture_compression_3dc;
   GLboolean ATI_texture_mirror_once;
   GLboolean ATI_texture_env_combine3;
   GLboolean ATI_fragment_shader;
   GLboolean ATI_separate_stencil;
   GLboolean INTEL_performance_query;
   GLboolean MESA_pack_invert;
   GLboolean MESA_ycbcr_texture;
   GLboolean NV_conditional_render;
   GLboolean NV_fog_distance;
   GLboolean NV_fragment_program_option;
   GLboolean NV_point_sprite;
   GLboolean NV_primitive_restart;
   GLboolean NV_texture_barrier;
   GLboolean NV_texture_env_combine4;
   GLboolean NV_texture_rectangle;
   GLboolean NV_vdpau_interop;
   GLboolean TDFX_texture_compression_FXT1;
   GLboolean OES_EGL_image;
   GLboolean OES_draw_texture;
   GLboolean OES_depth_texture_cube_map;
   GLboolean OES_EGL_image_external;
   GLboolean OES_compressed_ETC1_RGB8_texture;
   GLboolean extension_sentinel;
   /** The extension string */
   const GLubyte *String;
   /** Number of supported extensions */
   GLuint Count;
};


/**
 * A stack of matrices (projection, modelview, color, texture, etc).
 */
struct gl_matrix_stack
{
   GLmatrix *Top;      /**< points into Stack */
   GLmatrix *Stack;    /**< array [MaxDepth] of GLmatrix */
   GLuint Depth;       /**< 0 <= Depth < MaxDepth */
   GLuint MaxDepth;    /**< size of Stack[] array */
   GLuint DirtyFlag;   /**< _NEW_MODELVIEW or _NEW_PROJECTION, for example */
};


/**
 * \name Bits for image transfer operations 
 * \sa __struct gl_contextRec::ImageTransferState.
 */
/*@{*/
#define IMAGE_SCALE_BIAS_BIT                      0x1
#define IMAGE_SHIFT_OFFSET_BIT                    0x2
#define IMAGE_MAP_COLOR_BIT                       0x4
#define IMAGE_CLAMP_BIT                           0x800


/** Pixel Transfer ops */
#define IMAGE_BITS (IMAGE_SCALE_BIAS_BIT |			\
		    IMAGE_SHIFT_OFFSET_BIT |			\
		    IMAGE_MAP_COLOR_BIT)

/**
 * \name Bits to indicate what state has changed.  
 */
/*@{*/
#define _NEW_MODELVIEW         (1 << 0)   /**< gl_context::ModelView */
#define _NEW_PROJECTION        (1 << 1)   /**< gl_context::Projection */
#define _NEW_TEXTURE_MATRIX    (1 << 2)   /**< gl_context::TextureMatrix */
#define _NEW_COLOR             (1 << 3)   /**< gl_context::Color */
#define _NEW_DEPTH             (1 << 4)   /**< gl_context::Depth */
#define _NEW_EVAL              (1 << 5)   /**< gl_context::Eval, EvalMap */
#define _NEW_FOG               (1 << 6)   /**< gl_context::Fog */
#define _NEW_HINT              (1 << 7)   /**< gl_context::Hint */
#define _NEW_LIGHT             (1 << 8)   /**< gl_context::Light */
#define _NEW_LINE              (1 << 9)   /**< gl_context::Line */
#define _NEW_PIXEL             (1 << 10)  /**< gl_context::Pixel */
#define _NEW_POINT             (1 << 11)  /**< gl_context::Point */
#define _NEW_POLYGON           (1 << 12)  /**< gl_context::Polygon */
#define _NEW_POLYGONSTIPPLE    (1 << 13)  /**< gl_context::PolygonStipple */
#define _NEW_SCISSOR           (1 << 14)  /**< gl_context::Scissor */
#define _NEW_STENCIL           (1 << 15)  /**< gl_context::Stencil */
#define _NEW_TEXTURE           (1 << 16)  /**< gl_context::Texture */
#define _NEW_TRANSFORM         (1 << 17)  /**< gl_context::Transform */
#define _NEW_VIEWPORT          (1 << 18)  /**< gl_context::Viewport */
/* gap, re-use for core Mesa state only; use ctx->DriverFlags otherwise */
#define _NEW_ARRAY             (1 << 20)  /**< gl_context::Array */
#define _NEW_RENDERMODE        (1 << 21)  /**< gl_context::RenderMode, etc */
#define _NEW_BUFFERS           (1 << 22)  /**< gl_context::Visual, DrawBuffer, */
#define _NEW_CURRENT_ATTRIB    (1 << 23)  /**< gl_context::Current */
#define _NEW_MULTISAMPLE       (1 << 24)  /**< gl_context::Multisample */
#define _NEW_TRACK_MATRIX      (1 << 25)  /**< gl_context::VertexProgram */
#define _NEW_PROGRAM           (1 << 26)  /**< New program/shader state */
#define _NEW_PROGRAM_CONSTANTS (1 << 27)
#define _NEW_BUFFER_OBJECT     (1 << 28)
#define _NEW_FRAG_CLAMP        (1 << 29)
/* gap, re-use for core Mesa state only; use ctx->DriverFlags otherwise */
#define _NEW_VARYING_VP_INPUTS (1 << 31) /**< gl_context::varying_vp_inputs */
#define _NEW_ALL ~0
/*@}*/


/**
 * Composite state flags
 */
/*@{*/
#define _MESA_NEW_NEED_EYE_COORDS         (_NEW_LIGHT |		\
                                           _NEW_TEXTURE |	\
                                           _NEW_POINT |		\
                                           _NEW_PROGRAM |	\
                                           _NEW_MODELVIEW)

#define _MESA_NEW_SEPARATE_SPECULAR        (_NEW_LIGHT | \
                                            _NEW_FOG | \
                                            _NEW_PROGRAM)


/*@}*/




/* This has to be included here. */
#include "dd.h"


/**
 * Display list flags.
 * Strictly this is a tnl-private concept, but it doesn't seem
 * worthwhile adding a tnl private structure just to hold this one bit
 * of information:
 */
#define DLIST_DANGLING_REFS     0x1 

/** @{
 *
 * These are a mapping of the GL_ARB_debug_output/GL_KHR_debug enums
 * to small enums suitable for use as an array index.
 */

enum mesa_debug_source {
   MESA_DEBUG_SOURCE_API,
   MESA_DEBUG_SOURCE_WINDOW_SYSTEM,
   MESA_DEBUG_SOURCE_SHADER_COMPILER,
   MESA_DEBUG_SOURCE_THIRD_PARTY,
   MESA_DEBUG_SOURCE_APPLICATION,
   MESA_DEBUG_SOURCE_OTHER,
   MESA_DEBUG_SOURCE_COUNT
};

enum mesa_debug_type {
   MESA_DEBUG_TYPE_ERROR,
   MESA_DEBUG_TYPE_DEPRECATED,
   MESA_DEBUG_TYPE_UNDEFINED,
   MESA_DEBUG_TYPE_PORTABILITY,
   MESA_DEBUG_TYPE_PERFORMANCE,
   MESA_DEBUG_TYPE_OTHER,
   MESA_DEBUG_TYPE_MARKER,
   MESA_DEBUG_TYPE_PUSH_GROUP,
   MESA_DEBUG_TYPE_POP_GROUP,
   MESA_DEBUG_TYPE_COUNT
};

enum mesa_debug_severity {
   MESA_DEBUG_SEVERITY_LOW,
   MESA_DEBUG_SEVERITY_MEDIUM,
   MESA_DEBUG_SEVERITY_HIGH,
   MESA_DEBUG_SEVERITY_NOTIFICATION,
   MESA_DEBUG_SEVERITY_COUNT
};

/** @} */

/**
 * Enum for the OpenGL APIs we know about and may support.
 *
 * NOTE: This must match the api_enum table in
 * src/mesa/main/get_hash_generator.py
 */
typedef enum
{
   API_OPENGL_COMPAT,      /* legacy / compatibility contexts */
   API_OPENGLES,
   API_OPENGLES2,
   API_OPENGL_CORE,
   API_XGL,
   API_OPENGL_LAST = API_OPENGL_CORE
} gl_api;


/**
 * Mesa rendering context.
 *
 * This is the central context data structure for Mesa.  Almost all
 * OpenGL state is contained in this structure.
 * Think of this as a base class from which device drivers will derive
 * sub classes.
 */
struct gl_context
{
   /** State possibly shared with other contexts in the address space */
    struct gl_shared_state *Shared;

   /** \name API function pointer tables */
   /*@{*/
    gl_api API;

   /**
    * Device driver function pointer table
    */
    struct dd_function_table Driver;  // LunarG: MARKED_FOR_DEATH

   /** Core/Driver constants */
   struct gl_constants Const;

   /** Extension information */
   struct gl_extensions Extensions;

   /** GL version integer, for example 31 for GL 3.1, or 20 for GLES 2.0. */
   GLuint Version;
   char *VersionString;

   struct gl_multisample_attrib Multisample;

   struct gl_program_state Program;  /**< general program state */
   struct gl_vertex_program_state VertexProgram;
   struct gl_fragment_program_state FragmentProgram;
   struct gl_geometry_program_state GeometryProgram;
   struct gl_ati_fragment_shader_state ATIFragmentShader;

   struct gl_shader_compiler_options ShaderCompilerOptions[MESA_SHADER_STAGES];

   GLenum ErrorValue;        /**< Last error code */

   /**
    * Recognize and silence repeated error debug messages in buggy apps.
    */
   const char *ErrorDebugFmtString;
   GLuint ErrorDebugCount;

   /* GL_ARB_debug_output/GL_KHR_debug */
    // LunarG: MARKED_FOR_DEATH
   mtx_t DebugMutex;
   struct gl_debug_state *Debug;


   struct gl_list_extensions *ListExt; /**< driver dlist extensions */

   GLbitfield GlslFlags;                    /**< Mask of GLSL_x flags */

   /* A thread pool for threaded shader compilation */
   struct _mesa_threadpool *ThreadPool;
};


#ifdef DEBUG
extern int MESA_VERBOSE;
extern int MESA_DEBUG_FLAGS;
# define MESA_FUNCTION __FUNCTION__
#else
# define MESA_VERBOSE 0
# define MESA_DEBUG_FLAGS 0
# define MESA_FUNCTION "a function"
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif


/** The MESA_VERBOSE var is a bitmask of these flags */
enum _verbose
{
   VERBOSE_VARRAY		= 0x0001,
   VERBOSE_TEXTURE		= 0x0002,
   VERBOSE_MATERIAL		= 0x0004,
   VERBOSE_PIPELINE		= 0x0008,
   VERBOSE_DRIVER		= 0x0010,
   VERBOSE_STATE		= 0x0020,
   VERBOSE_API			= 0x0040,
   VERBOSE_DISPLAY_LIST		= 0x0100,
   VERBOSE_LIGHTING		= 0x0200,
   VERBOSE_PRIMS		= 0x0400,
   VERBOSE_VERTS		= 0x0800,
   VERBOSE_DISASSEM		= 0x1000,
   VERBOSE_DRAW                 = 0x2000,
   VERBOSE_SWAPBUFFERS          = 0x4000
};


/** The MESA_DEBUG_FLAGS var is a bitmask of these flags */
enum _debug
{
   DEBUG_SILENT                 = (1 << 0),
   DEBUG_ALWAYS_FLUSH		= (1 << 1),
   DEBUG_INCOMPLETE_TEXTURE     = (1 << 2),
   DEBUG_INCOMPLETE_FBO         = (1 << 3)
};


#define DRI_CONF_GLASS_MODE_NEVER 0
#define DRI_CONF_GLASS_MODE_WHITELIST 1
#define DRI_CONF_GLASS_MODE_ALWAYS 2


#ifdef __cplusplus
}
#endif

#endif /* MTYPES_H */
