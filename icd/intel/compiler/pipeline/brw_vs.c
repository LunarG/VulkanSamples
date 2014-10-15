/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "main/compiler.h"
#include "brw_context.h"
#include "brw_vs.h"
//#include "brw_util.h"  // LunarG: Remove
//#include "brw_state.h" // LunarG: Remove
#include "program/prog_print.h"
#include "program/prog_parameter.h"

#include "glsl/ralloc.h"

#include "icd-utils.h"  // LunarG: ADD

static inline void assign_vue_slot(struct brw_vue_map *vue_map,
                                   int varying)
{
   /* Make sure this varying hasn't been assigned a slot already */
   assert (vue_map->varying_to_slot[varying] == -1);

   vue_map->varying_to_slot[varying] = vue_map->num_slots;
   vue_map->slot_to_varying[vue_map->num_slots++] = varying;
}

/**
 * Compute the VUE map for vertex shader program.
 */
void
brw_compute_vue_map(struct brw_context *brw, struct brw_vue_map *vue_map,
                    GLbitfield64 slots_valid)
{
   vue_map->slots_valid = slots_valid;
   int i;

   /* gl_Layer and gl_ViewportIndex don't get their own varying slots -- they
    * are stored in the first VUE slot (VARYING_SLOT_PSIZ).
    */
   slots_valid &= ~(VARYING_BIT_LAYER | VARYING_BIT_VIEWPORT);

   /* Make sure that the values we store in vue_map->varying_to_slot and
    * vue_map->slot_to_varying won't overflow the signed chars that are used
    * to store them.  Note that since vue_map->slot_to_varying sometimes holds
    * values equal to BRW_VARYING_SLOT_COUNT, we need to ensure that
    * BRW_VARYING_SLOT_COUNT is <= 127, not 128.
    */
   STATIC_ASSERT(BRW_VARYING_SLOT_COUNT <= 127);

   vue_map->num_slots = 0;
   for (i = 0; i < BRW_VARYING_SLOT_COUNT; ++i) {
      vue_map->varying_to_slot[i] = -1;
      vue_map->slot_to_varying[i] = BRW_VARYING_SLOT_COUNT;
   }

   /* VUE header: format depends on chip generation and whether clipping is
    * enabled.
    */
   if (brw->gen < 6) {
      /* There are 8 dwords in VUE header pre-Ironlake:
       * dword 0-3 is indices, point width, clip flags.
       * dword 4-7 is ndc position
       * dword 8-11 is the first vertex data.
       *
       * On Ironlake the VUE header is nominally 20 dwords, but the hardware
       * will accept the same header layout as Gen4 [and should be a bit faster]
       */
      assign_vue_slot(vue_map, VARYING_SLOT_PSIZ);
      assign_vue_slot(vue_map, BRW_VARYING_SLOT_NDC);
      assign_vue_slot(vue_map, VARYING_SLOT_POS);
   } else {
      /* There are 8 or 16 DWs (D0-D15) in VUE header on Sandybridge:
       * dword 0-3 of the header is indices, point width, clip flags.
       * dword 4-7 is the 4D space position
       * dword 8-15 of the vertex header is the user clip distance if
       * enabled.
       * dword 8-11 or 16-19 is the first vertex element data we fill.
       */
      assign_vue_slot(vue_map, VARYING_SLOT_PSIZ);
      assign_vue_slot(vue_map, VARYING_SLOT_POS);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0))
         assign_vue_slot(vue_map, VARYING_SLOT_CLIP_DIST0);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1))
         assign_vue_slot(vue_map, VARYING_SLOT_CLIP_DIST1);

      /* front and back colors need to be consecutive so that we can use
       * ATTRIBUTE_SWIZZLE_INPUTATTR_FACING to swizzle them when doing
       * two-sided color.
       */
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_COL0))
         assign_vue_slot(vue_map, VARYING_SLOT_COL0);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_BFC0))
         assign_vue_slot(vue_map, VARYING_SLOT_BFC0);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_COL1))
         assign_vue_slot(vue_map, VARYING_SLOT_COL1);
      if (slots_valid & BITFIELD64_BIT(VARYING_SLOT_BFC1))
         assign_vue_slot(vue_map, VARYING_SLOT_BFC1);
   }

   /* The hardware doesn't care about the rest of the vertex outputs, so just
    * assign them contiguously.  Don't reassign outputs that already have a
    * slot.
    *
    * We generally don't need to assign a slot for VARYING_SLOT_CLIP_VERTEX,
    * since it's encoded as the clip distances by emit_clip_distances().
    * However, it may be output by transform feedback, and we'd rather not
    * recompute state when TF changes, so we just always include it.
    */
   for (int i = 0; i < VARYING_SLOT_MAX; ++i) {
      if ((slots_valid & BITFIELD64_BIT(i)) &&
          vue_map->varying_to_slot[i] == -1) {
         assign_vue_slot(vue_map, i);
      }
   }
}


// LunarG: TODO - How to handle user clip planes?
/**
 * Decide which set of clip planes should be used when clipping via
 * gl_Position or gl_ClipVertex.
 */
//gl_clip_plane *brw_select_clip_planes(struct gl_context *ctx)
//{
//   if (ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX]) {
//      /* There is currently a GLSL vertex shader, so clip according to GLSL
//       * rules, which means compare gl_ClipVertex (or gl_Position, if
//       * gl_ClipVertex wasn't assigned) against the eye-coordinate clip planes
//       * that were stored in EyeUserPlane at the time the clip planes were
//       * specified.
//       */
//      return ctx->Transform.EyeUserPlane;
//   } else {
//      /* Either we are using fixed function or an ARB vertex program.  In
//       * either case the clip planes are going to be compared against
//       * gl_Position (which is in clip coordinates) so we have to clip using
//       * _ClipUserPlane, which was transformed into clip coordinates by Mesa
//       * core.
//       */
//      return ctx->Transform._ClipUserPlane;
//   }
//}


bool
brw_vs_prog_data_compare(const void *in_a, const void *in_b)
{
   const struct brw_vs_prog_data *a = in_a;
   const struct brw_vs_prog_data *b = in_b;

   /* Compare the base structure. */
   if (!brw_stage_prog_data_compare(&a->base.base, &b->base.base))
      return false;

   /* Compare the rest of the struct. */
   const unsigned offset = sizeof(struct brw_stage_prog_data);
   if (memcmp(((char *) a) + offset, ((char *) b) + offset,
              sizeof(struct brw_vs_prog_data) - offset)) {
      return false;
   }

   return true;
}

static void
brw_vs_init_compile(struct brw_context *brw,
	            struct gl_shader_program *prog,
	            struct brw_vertex_program *vp,
	            const struct brw_vs_prog_key *key,
	            struct brw_vs_compile *c)
{
   memset(c, 0, sizeof(*c));

   memcpy(&c->key, key, sizeof(*key));
   c->vp = vp;
   c->base.shader_prog = prog;
   c->base.mem_ctx = ralloc_context(NULL);
}

static bool
brw_vs_do_compile(struct brw_context *brw,
	          struct brw_vs_compile *c)
{
   struct brw_stage_prog_data *stage_prog_data = &c->prog_data.base.base;
   struct gl_shader *vs = NULL;
   int i;

   if (c->base.shader_prog)
      vs = c->base.shader_prog->_LinkedShaders[MESA_SHADER_VERTEX];

   /* Allocate the references to the uniforms that will end up in the
    * prog_data associated with the compiled program, and which will be freed
    * by the state cache.
    */
   int param_count;
   if (vs) {
      /* We add padding around uniform values below vec4 size, with the worst
       * case being a float value that gets blown up to a vec4, so be
       * conservative here.
       */
      param_count = vs->num_uniform_components * 4;

   } else {
      param_count = c->vp->program.Base.Parameters->NumParameters * 4;
   }
   /* vec4_visitor::setup_uniform_clipplane_values() also uploads user clip
    * planes as uniforms.
    */
   param_count += c->key.base.nr_userclip_plane_consts * 4;

   stage_prog_data->param = rzalloc_array(NULL, const float *, param_count);
   stage_prog_data->pull_param = rzalloc_array(NULL, const float *, param_count);

   /* Setting nr_params here NOT to the size of the param and pull_param
    * arrays, but to the number of uniform components vec4_visitor
    * needs. vec4_visitor::setup_uniforms() will set it back to a proper value.
    */
   stage_prog_data->nr_params = ALIGN(param_count, 4) / 4;
   if (vs) {
      stage_prog_data->nr_params += vs->num_samplers;
   }

   GLbitfield64 outputs_written = c->vp->program.Base.OutputsWritten;
   c->prog_data.inputs_read = c->vp->program.Base.InputsRead;

   if (c->key.copy_edgeflag) {
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_EDGE);
      c->prog_data.inputs_read |= VERT_BIT_EDGEFLAG;
   }

   if (brw->gen < 6) {
      /* Put dummy slots into the VUE for the SF to put the replaced
       * point sprite coords in.  We shouldn't need these dummy slots,
       * which take up precious URB space, but it would mean that the SF
       * doesn't get nice aligned pairs of input coords into output
       * coords, which would be a pain to handle.
       */
      for (i = 0; i < 8; i++) {
         if (c->key.point_coord_replace & (1 << i))
            outputs_written |= BITFIELD64_BIT(VARYING_SLOT_TEX0 + i);
      }

      /* if back colors are written, allocate slots for front colors too */
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC0))
         outputs_written |= BITFIELD64_BIT(VARYING_SLOT_COL0);
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC1))
         outputs_written |= BITFIELD64_BIT(VARYING_SLOT_COL1);
   }

   /* In order for legacy clipping to work, we need to populate the clip
    * distance varying slots whenever clipping is enabled, even if the vertex
    * shader doesn't write to gl_ClipDistance.
    */
   if (c->key.base.userclip_active) {
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0);
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1);
   }

   brw_compute_vue_map(brw, &c->prog_data.base.vue_map, outputs_written);

   if (0) {
      _mesa_fprint_program_opt(stderr, &c->vp->program.Base, PROG_PRINT_DEBUG,
			       true);
   }

   /* Emit GEN4 code.
    */
   c->base.program = brw_vs_emit(brw, c->base.shader_prog, c,
         &c->prog_data, c->base.mem_ctx, &c->base.program_size);
   if (c->base.program == NULL)
      return false;

   if (c->base.last_scratch) {
      c->prog_data.base.total_scratch
         = brw_get_scratch_size(c->base.last_scratch*REG_SIZE);
   }

   return true;
}

static void
brw_vs_clear_compile(struct brw_context *brw,
	             struct brw_vs_compile *c)
{
   ralloc_free(c->base.mem_ctx);
}

// LunarG : TODO - user clip planes?
//void
//brw_setup_vec4_key_clip_info(struct brw_context *brw,
//                             struct brw_vec4_prog_key *key,
//                             bool program_uses_clip_distance)
//{
//   struct gl_context *ctx = &brw->ctx;

//   key->userclip_active = (ctx->Transform.ClipPlanesEnabled != 0);
//   if (key->userclip_active && !program_uses_clip_distance) {
//      key->nr_userclip_plane_consts
//         = _mesa_logbase2(ctx->Transform.ClipPlanesEnabled) + 1;
//   }
//}

bool
brw_vs_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_vs_prog_key key;

   if (!prog->_LinkedShaders[MESA_SHADER_VERTEX])
      return true;

   struct gl_vertex_program *vp = (struct gl_vertex_program *)
      prog->_LinkedShaders[MESA_SHADER_VERTEX]->Program;
   struct brw_vertex_program *bvp = brw_vertex_program(vp);

   memset(&key, 0, sizeof(key));

   brw_vec4_setup_prog_key_for_precompile(ctx, &key.base, bvp->id, &vp->Base);

   struct brw_vs_compile c;

   brw_vs_init_compile(brw, prog, bvp, &key, &c);
   if (!brw_vs_do_compile(brw, &c)) {
      brw_vs_clear_compile(brw, &c);
      return false;
   }

   // Rather than defer or upload to cache, hand off
   // the compile results back to the brw_context
   brw_shader_program_save_vs_compile(brw->shader_prog, &c);

   return true;
}
