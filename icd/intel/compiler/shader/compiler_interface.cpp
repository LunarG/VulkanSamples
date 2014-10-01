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
#include "shader.h"
#include "compiler_interface.h"
#include "compiler/mesa-utils/src/glsl/ralloc.h"
#include "compiler/mesa-utils/src/glsl/glsl_parser_extras.h"
#include "compiler/shader/program.h"
#include "compiler/mesa-utils/src/mesa/main/context.h"
#include "compiler/shader/standalone_scaffolding.h"
#include "compiler/pipeline/brw_wm.h"
#include "compiler/pipeline/brw_shader.h"

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

void initialize_mesa_context_to_defaults(struct gl_context *ctx)
{
   memset(ctx, 0, sizeof(*ctx));

   ctx->API = API_OPENGL_CORE;

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
   ctx->Const.GLSLVersion = 330;

   ctx->Const.MaxComputeWorkGroupCount[0] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[1] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[2] = 65535;
   ctx->Const.MaxComputeWorkGroupSize[0] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[1] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[2] = 64;
   ctx->Const.MaxComputeWorkGroupInvocations = 1024;

   init_mesa_program_limits(ctx, MESA_SHADER_VERTEX,   &ctx->Const.Program[MESA_SHADER_VERTEX]);
   init_mesa_program_limits(ctx, MESA_SHADER_GEOMETRY, &ctx->Const.Program[MESA_SHADER_GEOMETRY]);
   init_mesa_program_limits(ctx, MESA_SHADER_FRAGMENT, &ctx->Const.Program[MESA_SHADER_FRAGMENT]);
   init_mesa_program_limits(ctx, MESA_SHADER_COMPUTE,  &ctx->Const.Program[MESA_SHADER_COMPUTE]);

   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxInputComponents = 0; /* not used */
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxOutputComponents = 0; /* not used */


   /* 3.30 minimums. */
   ctx->Const.MaxLights = 8;
   ctx->Const.MaxClipPlanes = 8;
   ctx->Const.MaxTextureUnits = 2;
   ctx->Const.MaxTextureCoordUnits = 8;
   ctx->Const.MaxVarying = 60 / 4;
   ctx->Const.MaxDrawBuffers = 1;

   ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxInputComponents = 0; /* not used */
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents = 64;

   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxInputComponents =
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents;
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxOutputComponents = 128;

   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents =
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxOutputComponents;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxOutputComponents = 0; /* not used */

   ctx->Const.MaxCombinedTextureImageUnits =
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits
      + ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits
      + ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits;

   ctx->Const.MaxGeometryOutputVertices = 256;
   ctx->Const.MaxGeometryTotalOutputComponents = 1024;

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
struct gl_shader_program *shader_create_program(struct intel_shader *sh,
                                                const struct icd_bil_header *bil)
{
    struct gl_context local_ctx;
    struct gl_context *ctx = &local_ctx;

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


    // We should parse the glsl text out of bil right now, but
    // instead we are just plopping down our glsl
    switch(bil->gen_magic) {
    case 'v':
        shader->Type = GL_VERTEX_SHADER;
        shader->Source =
                "#version 330\n"
                "out vec4 color;\n"
                "out vec4 scale;\n"
                "void main() {\n"
                "   vec2 vertices[3];"
                "      vertices[0] = vec2(-0.5, -0.5);\n"
                "      vertices[1] = vec2( 0.5, -0.5);\n"
                "      vertices[2] = vec2( 0.5,  0.5);\n"
                "   vec4 colors[3];\n"
                "      colors[0] = vec4(1.0, 0.0, 0.0, 1.0);\n"
                "      colors[1] = vec4(0.0, 1.0, 0.0, 1.0);\n"
                "      colors[2] = vec4(0.0, 0.0, 1.0, 1.0);\n"
                "   color = colors[int(mod(gl_VertexID, 3))];\n"
                "   scale = vec4(1.0, 1.0, 1.0, 1.0);\n"
                "   gl_Position = vec4(vertices[int(mod(gl_VertexID, 3))], 0.0, 1.0);\n"
                "}\n";
        break;
    case 'w':
        shader->Type = GL_FRAGMENT_SHADER;
        shader->Source =
                "#version 430\n"
                "in vec4 color;\n"
                "in vec4 scale;\n"
                "layout(location = 0) uniform vec4 foo;\n"
                "void main() {\n"
                "   gl_FragColor = color * scale + foo;\n"
                "}\n";
        break;
    default:
        assert(0);
        break;
    }

    struct _mesa_glsl_parse_state *state =
        new(shader) _mesa_glsl_parse_state(ctx, shader->Stage, shader);

    shader->Stage = _mesa_shader_enum_to_shader_stage(shader->Type);
    shader_program->Type = shader->Stage;

    bool dump_ast = false;
    bool dump_hir = true;
    bool do_link  = true;

    _mesa_glsl_compile_shader(ctx, shader, dump_ast, dump_hir);

    if (strlen(shader->InfoLog) > 0)
        printf("Info log:\n%s\n", shader->InfoLog);

    if (!shader->CompileStatus)
        return NULL;

    assert(shader_program->NumShaders == 1);

    // for XGL, we are independently compiling and linking individual
    // shaders, which matches this frontend's concept of SSO
    shader_program->SeparateShader = true;

    link_shaders(ctx, shader_program);
    if (!shader_program->LinkStatus)
        return NULL;

    if (strlen(shader_program->InfoLog) > 0)
        printf("Info log for linking:\n%s\n", shader_program->InfoLog);

    _mesa_destroy_shader_compiler();

    return shader_program;
}


void shader_destroy_program(struct gl_shader_program *shader_program)
{
    for (unsigned i = 0; i < MESA_SHADER_STAGES; i++)
       ralloc_free(shader_program->_LinkedShaders[i]);

    ralloc_free(shader_program);
}

} // extern "C"
