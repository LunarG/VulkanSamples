/*
 * XGL
 *
 * Copyright (C) 2014 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   LunarG
 */

#include "shader.h"
#include "pipeline.h"
#include "compiler/shader/compiler_interface.h"
#include "compiler/pipeline/pipeline_compiler_interface.h"
#include "compiler/pipeline/brw_context.h"
#include "compiler/pipeline/brw_shader.h"
#include "compiler/mesa-utils/src/mesa/main/context.h"
#include "compiler/mesa-utils/src/glsl/ralloc.h"
#include "compiler/pipeline/brw_device_info.h"
#include "compiler/pipeline/brw_wm.h"


void initialize_brw_context(struct brw_context *brw)
{

    // create a stripped down context for compilation
    initialize_mesa_context_to_defaults(&brw->ctx);

    //
    // init the things pulled from DRI in brwCreateContext
    //
    struct brw_device_info *devInfo = rzalloc(brw, struct brw_device_info);
    devInfo->gen = 7;
    devInfo->gt = 3;
    devInfo->is_g4x = false;
    devInfo->is_baytrail = false;
    devInfo->is_haswell = true;
    devInfo->has_llc = true;
    devInfo->has_pln = true;
    devInfo->has_compr4 = true;
    devInfo->has_negative_rhw_bug = false;
    devInfo->needs_unlit_centroid_workaround = true;

    // hand code values until we have something to pull from
    // use brw_device_info_hsw_gt3
    brw->intelScreen = rzalloc(brw, struct intel_screen);
    brw->intelScreen->devinfo = devInfo;

    brw->gen = brw->intelScreen->devinfo->gen;
    brw->gt = brw->intelScreen->devinfo->gt;
    brw->is_g4x = brw->intelScreen->devinfo->is_g4x;
    brw->is_baytrail = brw->intelScreen->devinfo->is_baytrail;
    brw->is_haswell = brw->intelScreen->devinfo->is_haswell;
    brw->has_llc = brw->intelScreen->devinfo->has_llc;
    brw->has_pln = brw->intelScreen->devinfo->has_pln;
    brw->has_compr4 = brw->intelScreen->devinfo->has_compr4;
    brw->has_negative_rhw_bug = brw->intelScreen->devinfo->has_negative_rhw_bug;
    brw->needs_unlit_centroid_workaround =
       brw->intelScreen->devinfo->needs_unlit_centroid_workaround;

    brw->vs.base.stage = MESA_SHADER_VERTEX;
    brw->gs.base.stage = MESA_SHADER_GEOMETRY;
    brw->wm.base.stage = MESA_SHADER_FRAGMENT;

    //
    // init what remains of intel_screen
    //
    brw->intelScreen->deviceID = 0;
    brw->intelScreen->program_id = 0;

    brw_fs_alloc_reg_sets(brw->intelScreen);
    brw_vec4_alloc_reg_set(brw->intelScreen);

    brw->shader_prog = brw_new_shader_program(&brw->ctx, 0);
}

static inline void pipeline_destroy_compile(struct brw_context *brw) {
    ralloc_free(brw->shader_prog);
    ralloc_free(brw);
}

static void hexdump(FILE *fp, void *ptr, int buflen) {
  unsigned int *buf = (unsigned int*)ptr;
  int i, j;
  for (i=0; i<(buflen/4); i+=4) {
    fprintf(fp,"%06x: ", i);
    for (j=0; j<4; j++)
      if (i+j < (buflen/4))
        fprintf(fp,"%08x ", buf[i+j]);
      else
        fprintf(fp,"   ");
    fprintf(fp,"\n");
  }
}


extern "C" {

// invoke backend compiler to generate ISA and supporting data structures
XGL_RESULT intel_pipeline_shader_compile(struct intel_pipeline_shader *pipe_shader,
                                         const struct intel_shader *shader)
{
    XGL_RESULT status = XGL_SUCCESS;

    // create a brw_context
    struct brw_context *brw = rzalloc(NULL, struct brw_context);

    // allocate sub structures on the stack
    initialize_brw_context(brw);

    // LunarG : TODO - should this have been set for us somewhere?
    shader->ir->shader_program->Type =
            shader->ir->shader_program->Shaders[0]->Stage;

    if (brw_link_shader(&brw->ctx, shader->ir->shader_program)) {

        // first take at standalone backend compile
        switch(shader->ir->shader_program->Shaders[0]->Type) {
        case GL_FRAGMENT_SHADER:
        {
            // Start pulling bits out of our compile result.
            // see upload_ps_state() for references about what I believe each of these values are

            // I would prefer to find a way to pull this data out without exposing
            // the internals of the compiler, but it hasn't presented itself yet

            pipe_shader->codeSize = get_wm_program_size(brw->shader_prog);

            pipe_shader->pCode = icd_alloc(pipe_shader->codeSize, 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
            if (!pipe_shader->pCode) {
                status = XGL_ERROR_OUT_OF_MEMORY;
                break;
            }

            // copy the ISA out of our compile context, it is about to poof away
            memcpy(pipe_shader->pCode, get_wm_program(brw->shader_prog), pipe_shader->codeSize);

            struct brw_wm_prog_data *data = get_wm_prog_data(brw->shader_prog);
            pipe_shader->surface_count = data->base.binding_table.size_bytes / 4;
            pipe_shader->urb_grf_start = data->first_curbe_grf;

            // The following continue to match what is baked in to test case
            pipe_shader->uses = shader->uses;
            pipe_shader->in_count = shader->in_count;
            pipe_shader->out_count = shader->out_count;
            pipe_shader->sampler_count = shader->sampler_count;
            pipe_shader->barycentric_interps = shader->barycentric_interps;

            fprintf(stderr,"\nISA generated by compiler:\n");
            fprintf(stderr,"ISA size: %i\n", pipe_shader->codeSize);
            hexdump(stderr, pipe_shader->pCode, pipe_shader->codeSize);
            fflush(stderr);
        }

            break;

        case GL_VERTEX_SHADER:
        {
            pipe_shader->codeSize = get_vs_program_size(brw->shader_prog);

            pipe_shader->pCode = icd_alloc(pipe_shader->codeSize, 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
            if (!pipe_shader->pCode) {
                status = XGL_ERROR_OUT_OF_MEMORY;
                break;
            }

            // copy the ISA out of our compile context, it is about to poof away
            memcpy(pipe_shader->pCode, get_vs_program(brw->shader_prog), pipe_shader->codeSize);

            struct brw_vs_prog_data *data = get_vs_prog_data(brw->shader_prog);

            if (data->uses_vertexid)
                pipe_shader->uses |= INTEL_SHADER_USE_VID;

            // These are really best guesses, and will require more work to
            // understand as we turn on more features
            pipe_shader->in_count = data->base.urb_read_length;// = 1;
            pipe_shader->out_count = data->base.vue_map.num_slots;// = 2;
            pipe_shader->urb_grf_start = data->base.dispatch_grf_start_reg;// = 1;

            // The following continue to match what is baked in to test case
            pipe_shader->sampler_count = shader->sampler_count;
            pipe_shader->barycentric_interps = shader->barycentric_interps;


            fprintf(stderr,"\nISA generated by compiler:\n");
            fprintf(stderr,"ISA size: %i\n", pipe_shader->codeSize);
            hexdump(stderr, pipe_shader->pCode, pipe_shader->codeSize);
            fflush(stderr);
        }
            break;

        case GL_GEOMETRY_SHADER:
        case GL_COMPUTE_SHADER:
        default:
            assert(0);
            status = XGL_ERROR_BAD_PIPELINE_DATA;
        }
    } else {
        assert(0);
        status = XGL_ERROR_BAD_PIPELINE_DATA;
    }

    pipeline_destroy_compile(brw);

    return status;
}


} // extern "C"
