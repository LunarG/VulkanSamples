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

#include "icd-bil.h"
#include "pipeline.h"
#include "compiler_interface.h"
#include "compiler/mesa-utils/src/glsl/ralloc.h"
#include "compiler/mesa-utils/src/glsl/glsl_parser_extras.h"
#include "compiler/shader/program.h"
#include "compiler/mesa-utils/src/mesa/main/context.h"
#include "compiler/mesa-utils/src/mesa/main/config.h"
#include "compiler/shader/standalone_scaffolding.h"
#include "compiler/pipeline/brw_wm.h"
#include "compiler/pipeline/brw_shader.h"
#include "SPIRV/spirv.h"

/**
 * Init vertex/fragment/geometry program limits.
 * Base on init_program_limits()
 */
void init_mesa_program_limits(struct gl_context *ctx, gl_shader_stage stage,
                              struct gl_program_constants *prog)
{
    prog->MaxInstructions = MAX_PROGRAM_INSTRUCTIONS;
    prog->MaxAluInstructions = MAX_PROGRAM_INSTRUCTIONS;
    prog->MaxTexInstructions = MAX_PROGRAM_INSTRUCTIONS;
    prog->MaxTexIndirections = MAX_PROGRAM_INSTRUCTIONS;
    prog->MaxTemps = MAX_PROGRAM_TEMPS;
    prog->MaxEnvParams = MAX_PROGRAM_ENV_PARAMS;
    prog->MaxLocalParams = MAX_PROGRAM_LOCAL_PARAMS;
    prog->MaxAddressOffset = MAX_PROGRAM_LOCAL_PARAMS;

    switch (stage) {
    case MESA_SHADER_VERTEX:
        prog->MaxParameters = MAX_VERTEX_PROGRAM_PARAMS;
        prog->MaxAttribs = MAX_VERTEX_GENERIC_ATTRIBS;
        prog->MaxAddressRegs = MAX_VERTEX_PROGRAM_ADDRESS_REGS;
        prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
        prog->MaxInputComponents = 0; /* value not used */
        prog->MaxOutputComponents = 16 * 4; /* old limit not to break tnl and swrast */
        break;
    case MESA_SHADER_FRAGMENT:
        prog->MaxParameters = MAX_NV_FRAGMENT_PROGRAM_PARAMS;
        prog->MaxAttribs = MAX_NV_FRAGMENT_PROGRAM_INPUTS;
        prog->MaxAddressRegs = MAX_FRAGMENT_PROGRAM_ADDRESS_REGS;
        prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
        prog->MaxInputComponents = 16 * 4; /* old limit not to break tnl and swrast */
        prog->MaxOutputComponents = 0; /* value not used */
        break;
    case MESA_SHADER_GEOMETRY:
        prog->MaxParameters = MAX_VERTEX_PROGRAM_PARAMS;
        prog->MaxAttribs = MAX_VERTEX_GENERIC_ATTRIBS;
        prog->MaxAddressRegs = MAX_VERTEX_PROGRAM_ADDRESS_REGS;
        prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
        prog->MaxInputComponents = 16 * 4; /* old limit not to break tnl and swrast */
        prog->MaxOutputComponents = 16 * 4; /* old limit not to break tnl and swrast */
        break;
    case MESA_SHADER_COMPUTE:
        prog->MaxParameters = 0; /* not meaningful for compute shaders */
        prog->MaxAttribs = 0; /* not meaningful for compute shaders */
        prog->MaxAddressRegs = 0; /* not meaningful for compute shaders */
        prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
        prog->MaxInputComponents = 0; /* not meaningful for compute shaders */
        prog->MaxOutputComponents = 0; /* not meaningful for compute shaders */
        break;
    default:
        assert(0 && "Bad shader stage in init_program_limits()");
    }

    /* Set the native limits to zero.  This implies that there is no native
        * support for shaders.  Let the drivers fill in the actual values.
        */
    prog->MaxNativeInstructions = 0;
    prog->MaxNativeAluInstructions = 0;
    prog->MaxNativeTexInstructions = 0;
    prog->MaxNativeTexIndirections = 0;
    prog->MaxNativeAttribs = 0;
    prog->MaxNativeTemps = 0;
    prog->MaxNativeAddressRegs = 0;
    prog->MaxNativeParameters = 0;

    /* Set GLSL datatype range/precision info assuming IEEE float values.
        * Drivers should override these defaults as needed.
        */
    prog->MediumFloat.RangeMin = 127;
    prog->MediumFloat.RangeMax = 127;
    prog->MediumFloat.Precision = 23;
    prog->LowFloat = prog->HighFloat = prog->MediumFloat;

    /* Assume ints are stored as floats for now, since this is the least-common
        * denominator.  The OpenGL ES spec implies (page 132) that the precision
        * of integer types should be 0.  Practically speaking, IEEE
        * single-precision floating point values can only store integers in the
        * range [-0x01000000, 0x01000000] without loss of precision.
        */
    prog->MediumInt.RangeMin = 24;
    prog->MediumInt.RangeMax = 24;
    prog->MediumInt.Precision = 0;
    prog->LowInt = prog->HighInt = prog->MediumInt;

    prog->MaxUniformBlocks = 12;
    prog->MaxCombinedUniformComponents = (prog->MaxUniformComponents +
                                          ctx->Const.MaxUniformBlockSize / 4 *
                                          prog->MaxUniformBlocks);

    prog->MaxAtomicBuffers = 0;
    prog->MaxAtomicCounters = 0;
}

// Copied from context.c:_mesa_init_constants
void
initialize_mesa_constants(struct gl_context *ctx)
{
   int i;
   assert(ctx);

   /* Constants, may be overriden (usually only reduced) by device drivers */
   ctx->Const.MaxTextureMbytes = MAX_TEXTURE_MBYTES;
   ctx->Const.MaxTextureLevels = MAX_TEXTURE_LEVELS;
   ctx->Const.Max3DTextureLevels = MAX_3D_TEXTURE_LEVELS;
   ctx->Const.MaxCubeTextureLevels = MAX_CUBE_TEXTURE_LEVELS;
   ctx->Const.MaxTextureRectSize = MAX_TEXTURE_RECT_SIZE;
   ctx->Const.MaxArrayTextureLayers = MAX_ARRAY_TEXTURE_LAYERS;
   ctx->Const.MaxTextureCoordUnits = MAX_TEXTURE_COORD_UNITS;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   ctx->Const.MaxTextureUnits = MIN2(ctx->Const.MaxTextureCoordUnits,
                                     ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits);
   ctx->Const.MaxTextureMaxAnisotropy = MAX_TEXTURE_MAX_ANISOTROPY;
   ctx->Const.MaxTextureLodBias = MAX_TEXTURE_LOD_BIAS;
   ctx->Const.MaxTextureBufferSize = 65536;
   ctx->Const.TextureBufferOffsetAlignment = 1;
   ctx->Const.MaxArrayLockSize = MAX_ARRAY_LOCK_SIZE;
   ctx->Const.SubPixelBits = SUB_PIXEL_BITS;
   ctx->Const.MinPointSize = MIN_POINT_SIZE;
   ctx->Const.MaxPointSize = MAX_POINT_SIZE;
   ctx->Const.MinPointSizeAA = MIN_POINT_SIZE;
   ctx->Const.MaxPointSizeAA = MAX_POINT_SIZE;
   ctx->Const.PointSizeGranularity = (GLfloat) POINT_SIZE_GRANULARITY;
   ctx->Const.MinLineWidth = MIN_LINE_WIDTH;
   ctx->Const.MaxLineWidth = MAX_LINE_WIDTH;
   ctx->Const.MinLineWidthAA = MIN_LINE_WIDTH;
   ctx->Const.MaxLineWidthAA = MAX_LINE_WIDTH;
   ctx->Const.LineWidthGranularity = (GLfloat) LINE_WIDTH_GRANULARITY;
   ctx->Const.MaxClipPlanes = 6;
   ctx->Const.MaxLights = MAX_LIGHTS;
   ctx->Const.MaxShininess = 128.0;
   ctx->Const.MaxSpotExponent = 128.0;
   ctx->Const.MaxViewportWidth = MAX_VIEWPORT_WIDTH;
   ctx->Const.MaxViewportHeight = MAX_VIEWPORT_HEIGHT;
   ctx->Const.MinMapBufferAlignment = 64;

   /* Driver must override these values if ARB_viewport_array is supported. */
   ctx->Const.MaxViewports = 1;
   ctx->Const.ViewportSubpixelBits = 0;
   ctx->Const.ViewportBounds.Min = 0;
   ctx->Const.ViewportBounds.Max = 0;

   /** GL_ARB_uniform_buffer_object */
   ctx->Const.MaxCombinedUniformBlocks = 36;
   ctx->Const.MaxUniformBufferBindings = 36;
   ctx->Const.MaxUniformBlockSize = 16384;
   ctx->Const.UniformBufferOffsetAlignment = 1;

   for (i = 0; i < MESA_SHADER_STAGES; i++)
      init_mesa_program_limits(ctx, (gl_shader_stage)i, &ctx->Const.Program[i]);

   ctx->Const.MaxProgramMatrices = MAX_PROGRAM_MATRICES;
   ctx->Const.MaxProgramMatrixStackDepth = MAX_PROGRAM_MATRIX_STACK_DEPTH;

   /* CheckArrayBounds is overriden by drivers/x11 for X server */
   ctx->Const.CheckArrayBounds = GL_FALSE;

   /* GL_ARB_draw_buffers */
   ctx->Const.MaxDrawBuffers = MAX_DRAW_BUFFERS;

   ctx->Const.MaxColorAttachments = MAX_COLOR_ATTACHMENTS;
   ctx->Const.MaxRenderbufferSize = MAX_RENDERBUFFER_SIZE;

   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   ctx->Const.MaxCombinedTextureImageUnits = MAX_COMBINED_TEXTURE_IMAGE_UNITS;
   ctx->Const.MaxVarying = 16; /* old limit not to break tnl and swrast */
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   ctx->Const.MaxGeometryOutputVertices = MAX_GEOMETRY_OUTPUT_VERTICES;
   ctx->Const.MaxGeometryTotalOutputComponents = MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS;

   /* Shading language version */
   ctx->Const.GLSLVersion = 140;

   /* GL_ARB_framebuffer_object */
   ctx->Const.MaxSamples = 0;

   /* GL_ARB_sync */
   ctx->Const.MaxServerWaitTimeout = 0x1fff7fffffffULL;

   /* GL_ATI_envmap_bumpmap */
   ctx->Const.SupportedBumpUnits = SUPPORTED_ATI_BUMP_UNITS;

   /* GL_EXT_provoking_vertex */
   ctx->Const.QuadsFollowProvokingVertexConvention = GL_TRUE;

   /* GL_EXT_transform_feedback */
   ctx->Const.MaxTransformFeedbackBuffers = MAX_FEEDBACK_BUFFERS;
   ctx->Const.MaxTransformFeedbackSeparateComponents = 4 * MAX_FEEDBACK_ATTRIBS;
   ctx->Const.MaxTransformFeedbackInterleavedComponents = 4 * MAX_FEEDBACK_ATTRIBS;
   ctx->Const.MaxVertexStreams = 1;

   /* GL 3.2  */
   ctx->Const.ProfileMask = (ctx->API == API_OPENGL_CORE || ctx->API == API_XGL)
                          ? GL_CONTEXT_CORE_PROFILE_BIT
                          : GL_CONTEXT_COMPATIBILITY_PROFILE_BIT;

   /** GL_EXT_gpu_shader4 */
   ctx->Const.MinProgramTexelOffset = -8;
   ctx->Const.MaxProgramTexelOffset = 7;

   /* GL_ARB_texture_gather */
   ctx->Const.MinProgramTextureGatherOffset = -8;
   ctx->Const.MaxProgramTextureGatherOffset = 7;

   /* GL_ARB_robustness */
   ctx->Const.ResetStrategy = GL_NO_RESET_NOTIFICATION_ARB;

   /* PrimitiveRestart */
   ctx->Const.PrimitiveRestartInSoftware = GL_FALSE;

   /* ES 3.0 or ARB_ES3_compatibility */
   ctx->Const.MaxElementIndex = 0xffffffffu;

   /* GL_ARB_texture_multisample */
   ctx->Const.MaxColorTextureSamples = 1;
   ctx->Const.MaxDepthTextureSamples = 1;
   ctx->Const.MaxIntegerSamples = 1;

   /* GL_ARB_shader_atomic_counters */
   ctx->Const.MaxAtomicBufferBindings = MAX_COMBINED_ATOMIC_BUFFERS;
   ctx->Const.MaxAtomicBufferSize = MAX_ATOMIC_COUNTERS * ATOMIC_COUNTER_SIZE;
   ctx->Const.MaxCombinedAtomicBuffers = MAX_COMBINED_ATOMIC_BUFFERS;
   ctx->Const.MaxCombinedAtomicCounters = MAX_ATOMIC_COUNTERS;

   /* GL_ARB_vertex_attrib_binding */
   ctx->Const.MaxVertexAttribRelativeOffset = 2047;
   ctx->Const.MaxVertexAttribBindings = MAX_VERTEX_GENERIC_ATTRIBS;

   /* GL_ARB_compute_shader */
   ctx->Const.MaxComputeWorkGroupCount[0] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[1] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[2] = 65535;
   ctx->Const.MaxComputeWorkGroupSize[0] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[1] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[2] = 64;
   ctx->Const.MaxComputeWorkGroupInvocations = 1024;

   /** GL_ARB_gpu_shader5 */
   ctx->Const.MinFragmentInterpolationOffset = MIN_FRAGMENT_INTERPOLATION_OFFSET;
   ctx->Const.MaxFragmentInterpolationOffset = MAX_FRAGMENT_INTERPOLATION_OFFSET;

   ctx->Const.GlassMode = 0;
}

void initialize_mesa_context_to_defaults(struct gl_context *ctx)
{
   memset(ctx, 0, sizeof(*ctx));

   ctx->API = API_XGL;

   ctx->Extensions.dummy_false = false;
   ctx->Extensions.dummy_true = true;
   ctx->Extensions.ARB_compute_shader = true;
   ctx->Extensions.ARB_conservative_depth = true;
   ctx->Extensions.ARB_draw_instanced = true;
   ctx->Extensions.ARB_ES2_compatibility = true;
   ctx->Extensions.ARB_ES3_compatibility = true;
   ctx->Extensions.ARB_explicit_attrib_location = true;
   ctx->Extensions.ARB_fragment_coord_conventions = true;
   ctx->Extensions.ARB_gpu_shader5 = true;
   ctx->Extensions.ARB_sample_shading = true;
   ctx->Extensions.ARB_shader_bit_encoding = true;
   ctx->Extensions.ARB_shader_stencil_export = true;
   ctx->Extensions.ARB_shader_texture_lod = true;
   ctx->Extensions.ARB_shading_language_420pack = true;
   ctx->Extensions.ARB_shading_language_packing = true;
   ctx->Extensions.ARB_texture_cube_map_array = true;
   ctx->Extensions.ARB_texture_gather = true;
   ctx->Extensions.ARB_texture_multisample = true;
   ctx->Extensions.ARB_texture_query_levels = true;
   ctx->Extensions.ARB_texture_query_lod = true;
   ctx->Extensions.ARB_uniform_buffer_object = true;
   ctx->Extensions.ARB_viewport_array = true;
   ctx->Extensions.OES_EGL_image_external = true;
   ctx->Extensions.OES_standard_derivatives = true;
   ctx->Extensions.EXT_shader_integer_mix = true;
   ctx->Extensions.EXT_texture3D = true;
   ctx->Extensions.EXT_texture_array = true;
   ctx->Extensions.NV_texture_rectangle = true;

   // Set some initial constants, override as needed
   initialize_mesa_constants( ctx );

   /* Set up default shader compiler options. */
   struct gl_shader_compiler_options options;
   memset(&options, 0, sizeof(options));
   options.MaxUnrollIterations = 32;
   options.MaxIfDepth = UINT_MAX;

   /* Default pragma settings */
   options.DefaultPragmas.Optimize = true;

   for (int sh = 0; sh < MESA_SHADER_STAGES; ++sh)
      memcpy(&ctx->ShaderCompilerOptions[sh], &options, sizeof(options));


   ctx->Driver.NewShader = _mesa_new_shader;
   ctx->Driver.DeleteShader = _mesa_delete_shader;
}


extern "C" {

// invoke front end compiler to generate an independently linked
// program object that contains Mesa HIR
struct intel_ir *shader_create_ir(const struct intel_gpu *gpu,
                                  const void *code, size_t size)
{
    // Wrap this path in a mutex until we can clean up initialization
    static mtx_t mutex = _MTX_INITIALIZER_NP;
    mtx_lock(&mutex);

    struct icd_bil_header header;
    struct gl_context local_ctx;
    struct gl_context *ctx = &local_ctx;

    memcpy(&header, code, sizeof(header));
    if (header.magic != ICD_BIL_MAGIC) {
        mtx_unlock(&mutex);
        return NULL;
    }

    _mesa_create_shader_compiler();
    initialize_mesa_context_to_defaults(ctx);

    struct gl_shader_program *shader_program = brw_new_shader_program(ctx, 0);
    assert(shader_program != NULL);

    shader_program->InfoLog = ralloc_strdup(shader_program, "");
    shader_program->Shaders =
    reralloc(shader_program, shader_program->Shaders,
        struct gl_shader *, shader_program->NumShaders + 1);
    assert(shader_program->Shaders != NULL);

    struct gl_shader *shader = rzalloc(shader_program, struct gl_shader);

    shader_program->Shaders[shader_program->NumShaders] = shader;
    shader_program->NumShaders++;

    if (header.version == 0) {
        // version 0 means we really have GLSL Source
        shader->Source = (const char *) code + sizeof(header);

        switch(header.gen_magic) {
        case XGL_SHADER_STAGE_VERTEX:
            shader->Type = GL_VERTEX_SHADER;
            break;
        case XGL_SHADER_STAGE_FRAGMENT:
            shader->Type = GL_FRAGMENT_SHADER;
            break;
        default:
            assert(0);
            break;
        }
    } else {

        shader->Source = (const GLchar*)code;
        shader->Size   = size / sizeof(unsigned);  // size in BIL words

        spv::ExecutionModel executionModel = spv::ModelVertex;

        unsigned bilWord = 5;

        while (bilWord < size) {
            const unsigned      opWord = ((unsigned int*)code)[bilWord];
            const spv::OpCode   op     = spv::OpCode((opWord & 0xffff));

            if (op == spv::OpEntryPoint) {
                executionModel = spv::ExecutionModel(((unsigned int*)code)[bilWord+1]);
                break;
            }

            bilWord += (opWord & 0xffff0000) >> 16;
        }

        // We should parse the glsl text out of bil right now, but
        // instead we are just plopping down our glsl
        switch(executionModel) {
        case spv::ModelVertex:
            shader->Type = GL_VERTEX_SHADER;
            break;
        case spv::ModelFragment:
            shader->Type = GL_FRAGMENT_SHADER;
            break;
        default:
            assert(0);
            break;
        }
    }

    shader->Stage = _mesa_shader_enum_to_shader_stage(shader->Type);

    shader_program->Type = shader->Stage;

    bool dump_ast = false;
    bool dump_hir = false;

    _mesa_glsl_compile_shader(ctx, shader, dump_ast, dump_hir);

    if (strlen(shader->InfoLog) > 0) {
        printf("Info log:\n%s\n", shader->InfoLog);
        fflush(stdout);
    }

    if (!shader->CompileStatus) {
        _mesa_destroy_shader_compiler();
        mtx_unlock(&mutex);
        return NULL;
    }

    assert(shader_program->NumShaders == 1);

    // for XGL, we are independently compiling and linking individual
    // shaders, which matches this frontend's concept of SSO
    shader_program->SeparateShader = true;

    link_shaders(ctx, shader_program);

    if (strlen(shader_program->InfoLog) > 0) {
        printf("Info log for linking:\n%s\n", shader_program->InfoLog);
        fflush(stdout);
    }

    if (!shader_program->LinkStatus) {
        _mesa_destroy_shader_compiler();
        mtx_unlock(&mutex);
        return NULL;
    }

    _mesa_destroy_shader_compiler();

    mtx_unlock(&mutex);
    return (struct intel_ir *) shader_program;
}


void shader_destroy_ir(struct intel_ir *ir)
{
    struct gl_shader_program *sh_prog = (struct gl_shader_program *) ir;

    for (unsigned i = 0; i < MESA_SHADER_STAGES; i++)
       ralloc_free(sh_prog->_LinkedShaders[i]);

    ralloc_free(sh_prog);
}

} // extern "C"
