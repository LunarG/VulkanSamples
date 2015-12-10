/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 Valve Corporation
 * Copyright (C) 2015 Google, Inc.
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
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Create and use a pipeline cache accross runs.
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"
#include <sys/time.h>

// This sample tries to save and reuse pipeline cache data betwee runs
// On first run, no cache will be found, but it will be created and saved
// to disk. On later runs, the cache should be found, loaded, and used.
// Hopefully a speedup will observed.

// Design notes:
//    VkPipelineCacheCreateInfo.pInitialData
//        VkPipelineCacheCreateInfo.initialDataSize
//        If either are NULL, the cache will be new and clean
//        vkCreatePipelineCache ( VkPipelineCache )
//
//    Create a pipeline and use the pointer to cache when calling vkCreateGraphicsPipelines( VkPipelineCache )
//        If pipelineCache created above had data worth using, hopefully we see a speedup, indicating a cache hit
//        May need to create a complex pipeline to see speed up (complex shaders, etc.)
//
//
//
//    Extra credit:
//        Look on disk for second cache name
//            If found, load it into VkPipelineCacheCreateInfo.pInitialData
//            Create a new cache with it using vkCreatePipelineCache ( )
//            Merge with existing cache object, resulting in new cache object
//
//
//    Store the cache to disk by:
//        This is unclear to me.  Is the cache object populated automatically after vkCreateGraphicsPipelines uses it?
//        Let's assume so for now.  That means we shouldn't extract cache data until after the pipeline is created.
//        Pull data from cache with vkGetPipelineCacheData
//        Write it to disk (pData)
//
//    Destroy the pipeline cache object we created using: vkDestroyPipelineCache( )
//
//    Add timing data for each step, so it is clear that loading from cache is faster than creating a new pipeline...
//
//    Possibly use the following allocation Scope for memory, if the opportunity arises:
//        VK_SYSTEM_ALLOCATION_SCOPE_CACHE


const char *vertShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout (std140, binding = 0) uniform buf {\n"
        "        mat4 mvp;\n"
        "} ubuf;\n"
        "layout (location = 0) in vec4 pos;\n"
        "layout (location = 1) in vec2 inTexCoords;\n"
        "layout (location = 0) out vec2 texcoord;\n"
        "void main() {\n"
        "   texcoord = inTexCoords;\n"
        "   gl_Position = ubuf.mvp * pos;\n"
        "\n"
        "   // GL->VK conventions\n"
        "   gl_Position.y = -gl_Position.y;\n"
        "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
        "}\n";

const char *fragShaderText=
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout (binding = 1) uniform sampler2D tex;\n"
        "layout (location = 0) in vec2 texcoord;\n"
        "layout (location = 0) out vec4 outColor;\n"
        "void main() {\n"
        "   outColor = textureLod(tex, texcoord, 0.0);\n"
        "}\n";

// Some timing code to detect if our cache hits matter
typedef unsigned long long timestamp_t;
static timestamp_t get_timestamp ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Draw Textured Cube";
    const bool depthPresent = true;

    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    info.width = info.height = 500;
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_texture(info);
    init_uniform_buffer(info);
    init_descriptor_and_pipeline_layouts(info, true);
    init_renderpass(info, depthPresent);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, depthPresent);
    init_vertex_buffer(info, g_vb_texture_Data, sizeof(g_vb_texture_Data),
                       sizeof(g_vb_texture_Data[0]), true);
    init_descriptor_pool(info, true);
    init_descriptor_set(info, true);

    /* VULKAN_KEY_START */

    // Check disk for existing cache data
    size_t startCacheSize = 0;
    void*  startCacheData = {};

    const char* readFileName = "pipeline_cache_data.bin";
    FILE *pReadFile = fopen(readFileName, "rb");

    if (pReadFile) {
        // Determine cache size
        fseek (pReadFile , 0 , SEEK_END);
        startCacheSize = ftell (pReadFile);
        rewind (pReadFile);

        // Allocate memory to hold the initial cache data
        startCacheData = (char*) malloc (sizeof(char) * startCacheSize);
        if (startCacheData == NULL) {
            fputs ("Memory error",stderr);
            exit (2);
        }

        // Read the data into our buffer
        size_t result = fread (startCacheData, 1, startCacheSize, pReadFile);
        if (result != startCacheSize) {
            fputs ("Reading error", stderr);
            exit (3);
        }

        fclose (pReadFile);
        printf("  Pipeline cache HIT!\n");
        printf("  cacheData loaded from %s\n", readFileName);

    } else {

        printf("  Pipeline cache miss!\n");
    }

    VkPipelineCacheCreateInfo pipelineCache;
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCache.pNext = NULL;
    pipelineCache.initialDataSize = startCacheSize;
    pipelineCache.pInitialData = startCacheData;
    pipelineCache.flags = 0;
    res = vkCreatePipelineCache(info.device, &pipelineCache, nullptr, &info.pipelineCache);
    assert(res == VK_SUCCESS);

    // Free our initialData now that pipeline has been created
    free (startCacheData);

    // Time (roughly) the time taken to create the graphics pipeline
    timestamp_t t0 = get_timestamp();
    init_pipeline(info, depthPresent);
    timestamp_t t1 = get_timestamp();
    double milliSeconds = (t1 - t0) / 1000.0L;
    printf("  vkCreateGraphicsPipeline time: %f ms\n", milliSeconds);

    // Begin standard draw stuff

    init_presentable_image(info);
    VkClearValue clear_values[2];
    init_clear_color_and_depth(info, clear_values);
    VkRenderPassBeginInfo rp_begin;
    init_render_pass_begin_info(info, rp_begin);
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;
    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, NUM_DESCRIPTOR_SETS, info.desc_set.data(), 0, NULL);
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);
    init_viewports(info);
    init_scissors(info);
    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);
    execute_pre_present_barrier(info);
    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);
    VkFence drawFence = {};
    init_fence(info, drawFence);
    VkSubmitInfo submit_info = {};
    init_submit_info(info, submit_info);
    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, &submit_info, drawFence);
    assert(res == VK_SUCCESS);
    /* Now present the image in the window */
    VkPresentInfoKHR present = {};
    init_present_info(info, present);
    /* Make sure command buffer is finished before presenting */
    res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    assert(res == VK_SUCCESS);
    res = info.fpQueuePresentKHR(info.queue, &present);
    assert(res == VK_SUCCESS);
    wait_seconds(1);

    // End standard draw stuff

    // If we loaded a pipeline, let's try merging it with our current one.
    if (startCacheData) {
        // TODO: Create another pipeline, preferably different from the first one
        //       and merge it here.  Then store the merged one.
        // vkMergePipelineCaches()
    }

    // Store away the cache that we've populated.  This could concievably happen earlier,
    // depends on when the pipeline cache stops being populated internally.
    size_t endCacheSize = 0;
    void*  endCacheData = {};

    // Call with nullptr to get cache size
    vkGetPipelineCacheData(info.device, info.pipelineCache, &endCacheSize, nullptr);

    // Allocate memory to hold the initial cache data
    endCacheData = (char*) malloc (sizeof(char) * endCacheSize);
    if (!endCacheData) {
        fputs ("Memory error", stderr);
        exit (2);
    }

    // Call again with pointer to buffer
    vkGetPipelineCacheData(info.device, info.pipelineCache, &endCacheSize, endCacheData);

    // Write the file to disk, overwriting whatever was there
    FILE * pWriteFile;
    const char* writeFileName = "pipeline_cache_data.bin";
    pWriteFile = fopen (writeFileName, "wb");
    fwrite (endCacheData, sizeof(char), endCacheSize, pWriteFile);
    fclose (pWriteFile);
    printf ("  cacheData written to %s\n", writeFileName);

    /* VULKAN_KEY_END */

    vkDestroyFence(info.device, drawFence, NULL);
    vkDestroySemaphore(info.device, info.presentCompleteSemaphore, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_texture(info);
    destroy_descriptor_pool(info);
    destroy_vertex_buffer(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    destroy_descriptor_and_pipeline_layouts(info);
    destroy_uniform_buffer(info);
    destroy_depth_buffer(info);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}
