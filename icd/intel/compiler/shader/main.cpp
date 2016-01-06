/*
 * Copyright © 2008, 2009 Intel Corporation
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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

/** @file main.cpp
 *
 * This file is the main() routine and scaffolding for producing
 * builtin_compiler (which doesn't include builtins itself and is used
 * to generate the profile information for builtin_function.cpp), and
 * for glsl_compiler (which does include builtins and can be used to
 * offline compile GLSL code and examine the resulting GLSL IR.
 */

#include "gpu.h"
#include "pipeline.h"
#include "compiler_interface.h"
#include "compiler/mesa-utils/src/glsl/ralloc.h"
#include "pipeline_compiler_interface.h"


static char* load_spv_file(const char *filename, size_t *psize)
{
    long int size;
    void *shader_code;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    shader_code = malloc(size);
    size_t tmp = fread(shader_code, size, 1, fp);
    (void) tmp;

    *psize = size;

    return (char *) shader_code;
}


static char* load_glsl_file(const char *filename, size_t *psize, VkShaderStageFlagBits stage)
{
    long int size;
    void *shader_code;

    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp) + sizeof(icd_spv_header) + 1;

    fseek(fp, 0L, SEEK_SET);

    shader_code = malloc(size);
    size_t s = fread((char *)shader_code + sizeof(icd_spv_header), size - sizeof(icd_spv_header), 1, fp);
    (void) s;
    ((char *)shader_code)[size-1] = 0;

    icd_spv_header* header = (icd_spv_header*)shader_code;
    header->magic = ICD_SPV_MAGIC;
    header->version = 0; // not SPV
    header->gen_magic = stage;

    *psize = size;

    return (char *) shader_code;
}

int dump_ast = 0;
int dump_hir = 0;
int dump_lir = 0;
int do_link = 0;

const struct option compiler_opts[] = {
   { "dump-ast", no_argument, &dump_ast, 1 },
   { "dump-hir", no_argument, &dump_hir, 1 },
   { "dump-lir", no_argument, &dump_lir, 1 },
   { "link",     no_argument, &do_link,  1 },
   { "version",  required_argument, NULL, 'v' },
   { NULL, 0, NULL, 0 }
};


bool checkFileName(char* fileName)
{
    const unsigned fileNameLength = strlen(fileName);
    if (fileNameLength < 5 ||
            strncmp(".spv", &fileName[fileNameLength - 4], 4) != 0) {
        printf("file must be .spv, .vert, .geom, or .frag\n");
        return false;
    }

    return true;
}


bool checkFileExt(char* fileName, const char* ext)
{
    const unsigned fileNameLength = strlen(fileName);
    if (strncmp(ext, &fileName[fileNameLength - strlen(ext)], strlen(ext)) != 0) {
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
   int status = EXIT_SUCCESS;

   switch (argc) {
   case 2:
       {
           // Call vkCreateShader on the single shader

           printf("Frontend compile %s\n", argv[1]);
           fflush(stdout);

           void *shaderCode = 0;
           size_t size = 0;
           VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;

           if (checkFileExt(argv[1], "vert.spv")) {
               shaderCode = load_spv_file(argv[1], &size);
               stage = VK_SHADER_STAGE_VERTEX_BIT;
           } else if (checkFileExt(argv[1], "frag.spv")) {
               shaderCode = load_spv_file(argv[1], &size);
               stage = VK_SHADER_STAGE_FRAGMENT_BIT;
           } else if (checkFileExt(argv[1], "geom.spv")) {
               shaderCode = load_spv_file(argv[1], &size);
               stage = VK_SHADER_STAGE_GEOMETRY_BIT;
           } else if (checkFileExt(argv[1], ".spv")) {
               shaderCode = load_spv_file(argv[1], &size);
           } else if (checkFileExt(argv[1], ".vert")) {
               stage = VK_SHADER_STAGE_VERTEX_BIT;
           } else if (checkFileExt(argv[1], ".geom")) {
               stage = VK_SHADER_STAGE_GEOMETRY_BIT;
           } else if (checkFileExt(argv[1], ".frag")) {
               stage = VK_SHADER_STAGE_FRAGMENT_BIT;
           } else {
               return EXIT_FAILURE;
           }

           if (!shaderCode)
               shaderCode = load_glsl_file(argv[1], &size, stage);

           assert(shaderCode);

           struct intel_ir *shader_program = shader_create_ir(NULL, shaderCode, size, stage);
           assert(shader_program);

           // Set up only the fields needed for backend compile
           struct intel_gpu gpu = { 0 };
           gpu.gen_opaque = INTEL_GEN(7.5);
           gpu.gt = 3;

           printf("Backend compile %s\n", argv[1]);
           fflush(stdout);

           // struct timespec before;
           // clock_gettime(CLOCK_MONOTONIC, &before);
           // uint64_t beforeNanoSeconds = before.tv_nsec + before.tv_sec*INT64_C(1000000000);

           struct intel_pipeline_shader pipe_shader;
           VkResult ret = intel_pipeline_shader_compile(&pipe_shader, &gpu, NULL, NULL, shader_program);

           // struct timespec after;
           // clock_gettime(CLOCK_MONOTONIC, &after);
           // uint64_t afterNanoSeconds = after.tv_nsec + after.tv_sec*INT64_C(1000000000);
           // printf("file: %s, intel_pipeline_shader_compile = %" PRIu64 " milliseconds\n", argv[1], (afterNanoSeconds - beforeNanoSeconds)/1000000);
           // fflush(stdout);

           if (ret != VK_SUCCESS)
               return ret;

           intel_pipeline_shader_cleanup(&pipe_shader, &gpu);
           shader_destroy_ir(shader_program);
        }
       break;
   case 3:
       // Call vkCreateShader on both shaders, then call vkCreateGraphicsPipeline?
       // Only need to hook this up if we start invoking the backend once for the whole pipeline

       printf("Multiple shaders not hooked up yet\n");
       break;

       // Ensure both filenames have a .spv extension
       if (!checkFileName(argv[1]))
           return EXIT_FAILURE;

       if (!checkFileName(argv[2]))
           return EXIT_FAILURE;

       void *shaderCode[2];
       size_t size[2];
       struct intel_ir *result[2];

       // Compile first shader
       shaderCode[0] = load_spv_file(argv[1], &size[0]);
       assert(shaderCode[0]);
       printf("Compiling %s\n", argv[1]);
       result[0] = shader_create_ir(NULL, shaderCode[0], size[0], VK_SHADER_STAGE_VERTEX_BIT);
       assert(result[0]);

       // Compile second shader
       shaderCode[1] = load_spv_file(argv[2], &size[1]);
       assert(shaderCode[1]);
       printf("Compiling %s\n", argv[2]);
       result[1] = shader_create_ir(NULL, shaderCode[1], size[1], VK_SHADER_STAGE_FRAGMENT_BIT);
       assert(result[1]);


       shader_destroy_ir(result[0]);
       shader_destroy_ir(result[1]);

       break;
   case 0:
   case 1:
   default:
       printf("Please provide one .spv, .vert or .frag file as input\n");
       break;
   }

   return status;
}
