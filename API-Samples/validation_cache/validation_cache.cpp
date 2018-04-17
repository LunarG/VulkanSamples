/*
 * Vulkan Samples
 *
 * Copyright (C) 2016 Valve Corporation
 * Copyright (C) 2016 LunarG, Inc.
 * Copyright (C) 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Create and use a validation cache across runs.
*/

#include <util_init.hpp>
#include <array>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

// This sample tries to save and reuse validation cache data between runs.
// On first run, no cache will be found, it will be created and saved
// to disk. On later runs, the cache should be found, loaded, and used.
// Hopefully a speedup will observed.  In the future, the shader could
// be complicated a bit, to show a greater cache benefit.  Also, two
// caches could be created and merged.

const char *vertShaderText =
    "#version 400\n"
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
    "}\n";

// The fragment shader contains a 32-bit integer constant (tweak_value)
// which we can search for in the compiled SPIRV and replace with new
// values to generate "different" shaders.
const char *fragShaderText =
    "#version 400\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "const uint tweak_value = 0xdeadbeef;\n"
    "layout (binding = 1) uniform sampler2D tex;\n"
    "layout (location = 0) in vec2 texcoord;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = textureLod(tex, texcoord, 0.0);\n"
    "   outColor.a = float(tweak_value);\n"
    "}\n";

struct ShaderVariant {
    std::vector<uint32_t> spv;
#if defined(VK_EXT_validation_cache)
    VkShaderModuleValidationCacheCreateInfoEXT moduleValidationCacheCreateInfo;
#endif
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkShaderModule module;
};

int sample_main(int argc, char *argv[]) {
#if !defined(VK_EXT_validation_cache)
    fprintf(stderr, "%s not defined at build time.\n", VK_EXT_VALIDATION_CACHE_EXTENSION_NAME);
    fprintf(stderr, "To build this sample, update your Vulkan SDK to 1.0.61 or later.\n");
    return 0;
#endif

    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Validation Cache";
    const bool depthPresent = true;

    process_command_line_args(info, argc, argv);
    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    // The VK_EXT_validation_cache extension is implemented by the validation layers, so
    // they must be enabled in order to use it.
    info.instance_layer_names.push_back("VK_LAYER_LUNARG_standard_validation");
    if (!demo_check_layers(info.instance_layer_properties, info.instance_layer_names)) {
        std::cout << "Set the environment variable VK_LAYER_PATH to point to the location of your layers" << std::endl;
        exit(1);
    }
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_window_size(info, 500, 500);
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
#if defined(VK_EXT_validation_cache)
    bool foundExtension = false;
    for (const auto &layer_props : info.instance_layer_properties) {
        for (const auto &ext_props : layer_props.device_extensions) {
            if (strcmp(ext_props.extensionName, VK_EXT_VALIDATION_CACHE_EXTENSION_NAME) == 0) {
                foundExtension = true;
                break;
            }
        }
    }
    if (!foundExtension) {
        fprintf(stderr, "%s not supported.\n", VK_EXT_VALIDATION_CACHE_EXTENSION_NAME);
        fprintf(stderr, "(Is VK_LAYER_LUNARG_core_validation enabled and up to date?)");
        return 0;
    }
    info.device_extension_names.push_back(VK_EXT_VALIDATION_CACHE_EXTENSION_NAME);
#endif
    init_device(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_texture(info, "blue.ppm");
    init_uniform_buffer(info);
    init_descriptor_and_pipeline_layouts(info, true);
    init_renderpass(info, depthPresent);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, depthPresent);
    init_vertex_buffer(info, g_vb_texture_Data, sizeof(g_vb_texture_Data), sizeof(g_vb_texture_Data[0]), true);
    init_descriptor_pool(info, true);
    init_descriptor_set(info, true);

    /* VULKAN_KEY_START */

    // Check disk for existing cache data
    size_t startCacheSize = 0;
    void *startCacheData = nullptr;

    std::string directoryName = get_file_directory();
    std::string readFileName = directoryName + "validation_cache_data.bin";
    FILE *pReadFile = fopen(readFileName.c_str(), "rb");

    if (pReadFile) {
        // Determine cache size
        fseek(pReadFile, 0, SEEK_END);
        startCacheSize = ftell(pReadFile);
        rewind(pReadFile);

        // Allocate memory to hold the initial cache data
        startCacheData = (char *)malloc(sizeof(char) * startCacheSize);
        if (startCacheData == nullptr) {
            fputs("Memory error", stderr);
            exit(EXIT_FAILURE);
        }

        // Read the data into our buffer
        size_t result = fread(startCacheData, 1, startCacheSize, pReadFile);
        if (result != startCacheSize) {
            fputs("Reading error", stderr);
            free(startCacheData);
            exit(EXIT_FAILURE);
        }

        // Clean up and print results
        fclose(pReadFile);
        printf("  Validation cache HIT!\n");
        printf("  cacheData loaded from %s\n", readFileName.c_str());

    } else {
        // No cache found on disk
        printf("  Validation cache miss!\n");
    }

    if (startCacheData != nullptr) {
        // clang-format off
        //
        // Check for cache validity
        //
        // TODO: Update this as the spec evolves. The fields are not defined by the header.
        //
        // The code below supports SDK 1.0.65 Vulkan spec, which contains the following table:
        //
        // Offset	 Size            Meaning
        // ------    ------------    ------------------------------------------------------------------
        //      0               4    length in bytes of the entire validation cache header written as a
        //                           stream of bytes, with the least significant byte first
        //      4               4    a VkValidationCacheHeaderVersionEXT value written as a stream of
        //                           bytes, with the least significant byte first
        //      8    VK_UUID_SIZE    a layer commit ID expressed as a UUID, which uniquely identifies
        //                           the version of the validation layers used to generate these
        //                           validation results
        //
        // clang-format on
        uint32_t headerLength = 0;
        uint32_t cacheHeaderVersion = 0;
        uint8_t validationCacheUUID[VK_UUID_SIZE] = {};

        memcpy(&headerLength, (uint8_t *)startCacheData + 0, 4);
        memcpy(&cacheHeaderVersion, (uint8_t *)startCacheData + 4, 4);
        memcpy(validationCacheUUID, (uint8_t *)startCacheData + 8, VK_UUID_SIZE);

        // Check each field and report bad values before freeing existing cache
        bool badCache = false;

        if (headerLength <= 0) {
            badCache = true;
            printf("  Bad header length in %s.\n", readFileName.c_str());
            printf("    Cache contains: 0x%.8x\n", headerLength);
        }

        if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE) {
            badCache = true;
            printf("  Unsupported cache header version in %s.\n", readFileName.c_str());
            printf("    Cache contains: 0x%.8x\n", cacheHeaderVersion);
        }

        // Unlike pipeline caches, there's nothing meaningful for an application to compare the cache's UUID
        // field to. The UUID is checked internally to make sure it matches the version of the SPIRV validator
        // used to build the layers. We'll print it here anyway, for informational purposes.
        printf("Cache UUID: ");
        print_UUID(validationCacheUUID);
        printf("\n");

        if (badCache) {
            // Don't submit initial cache data if any version info is incorrect
            free(startCacheData);
            startCacheSize = 0;
            startCacheData = nullptr;

            // And clear out the old cache file for use in next run
            printf("  Deleting cache entry %s to repopulate.\n", readFileName.c_str());
            if (remove(readFileName.c_str()) != 0) {
                fputs("Reading error", stderr);
                exit(EXIT_FAILURE);
            }
        }
    }

#if defined(VK_EXT_validation_cache)
    // Load extension functions
    auto vkCreateValidationCache = (PFN_vkCreateValidationCacheEXT)vkGetDeviceProcAddr(info.device, "vkCreateValidationCacheEXT");
    auto vkDestroyValidationCache =
        (PFN_vkDestroyValidationCacheEXT)vkGetDeviceProcAddr(info.device, "vkDestroyValidationCacheEXT");
    auto vkGetValidationCacheData =
        (PFN_vkGetValidationCacheDataEXT)vkGetDeviceProcAddr(info.device, "vkGetValidationCacheDataEXT");

    // Feed the initial cache data into cache creation
    VkValidationCacheCreateInfoEXT validationCacheCreateInfo;
    validationCacheCreateInfo.sType = VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT;
    validationCacheCreateInfo.pNext = NULL;
    validationCacheCreateInfo.initialDataSize = startCacheSize;
    validationCacheCreateInfo.pInitialData = startCacheData;
    validationCacheCreateInfo.flags = 0;
    VkValidationCacheEXT validationCache = VK_NULL_HANDLE;
    res = vkCreateValidationCache(info.device, &validationCacheCreateInfo, nullptr, &validationCache);
    assert(res == VK_SUCCESS);
#endif

    // Free our initialData now that validation cache has been created
    free(startCacheData);
    startCacheData = NULL;

    // Generate a set of "different" SPIRV modules by patching in new
    // values for tweak_value in the fragment shader.

    // Compile fragment shader to SPIRV to use as a template for the remaining variants.
    init_glslang();
    std::vector<uint32_t> spvTemplate;
    bool compileSuccess = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderText, spvTemplate);
    assert(compileSuccess);
    finalize_glslang();
    // Locate tweak_value in the compiled binary and save its word index.
    int32_t tweakValueIndex = -1;
    for (size_t i = 0; i < spvTemplate.size(); ++i) {
        if (spvTemplate[i] == 0xdeadbeef) {
            tweakValueIndex = i;
            break;
        }
    }
    assert(tweakValueIndex >= 0);

    // Generate the unique variants from the template
    const size_t SHADER_COUNT = 10000;
    std::vector<ShaderVariant> shaderVariants(SHADER_COUNT);
    for (size_t i = 0; i < SHADER_COUNT; ++i) {
        shaderVariants[i].spv = spvTemplate;
        shaderVariants[i].spv[tweakValueIndex] = i;
#if defined(VK_EXT_validation_cache)
        shaderVariants[i].moduleValidationCacheCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT;
        shaderVariants[i].moduleValidationCacheCreateInfo.pNext = 0;
        shaderVariants[i].moduleValidationCacheCreateInfo.validationCache = validationCache;
        shaderVariants[i].moduleCreateInfo.pNext = &shaderVariants[i].moduleValidationCacheCreateInfo;
#endif
        shaderVariants[i].moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderVariants[i].moduleCreateInfo.codeSize = static_cast<uint32_t>(spvTemplate.size() * sizeof(uint32_t));
        shaderVariants[i].moduleCreateInfo.pCode = shaderVariants[i].spv.data();
        shaderVariants[i].moduleCreateInfo.flags = 0;
    }

    // Time (roughly) taken to create (and validate) the shader modules.
    timestamp_t start = get_milliseconds();
    for (auto &variant : shaderVariants) {
        res = vkCreateShaderModule(info.device, &variant.moduleCreateInfo, NULL, &variant.module);
        assert(res == VK_SUCCESS);
    }
    timestamp_t elapsed = get_milliseconds() - start;
    printf("  vkCreateShaderModule time: %0.f ms for %u calls\n", static_cast<double>(elapsed),
           static_cast<uint32_t>(SHADER_COUNT));

    // Delete module variants
    for (auto &variant : shaderVariants) {
        vkDestroyShaderModule(info.device, variant.module, NULL);
    }

    // Replace the module entry of info.shaderStages with a module created with the
    // validation cache active
    vkDestroyShaderModule(info.device, info.shaderStages[1].module, NULL);
    res = vkCreateShaderModule(info.device, &shaderVariants[0].moduleCreateInfo, NULL, &info.shaderStages[1].module);
    assert(res == VK_SUCCESS);

    // Begin standard draw stuff
    init_pipeline(info, depthPresent);
    init_presentable_image(info);
    VkClearValue clear_values[2];
    init_clear_color_and_depth(info, clear_values);
    VkRenderPassBeginInfo rp_begin;
    init_render_pass_begin_info(info, rp_begin);
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;
    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout, 0, NUM_DESCRIPTOR_SETS,
                            info.desc_set.data(), 0, NULL);
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);
    init_viewports(info);
    init_scissors(info);
    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);
    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);
    VkFence drawFence = {};
    init_fence(info, drawFence);
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    init_submit_info(info, submit_info, pipe_stage_flags);
    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.graphics_queue, 1, &submit_info, drawFence);
    assert(res == VK_SUCCESS);
    /* Now present the image in the window */
    VkPresentInfoKHR present = {};
    init_present_info(info, present);
    /* Make sure command buffer is finished before presenting */
    do {
        res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while (res == VK_TIMEOUT);
    assert(res == VK_SUCCESS);
    res = vkQueuePresentKHR(info.present_queue, &present);
    assert(res == VK_SUCCESS);
    wait_seconds(1);
    if (info.save_images) {
        write_ppm(info, "validation_cache");
    }

    // End standard draw stuff

#if defined(VK_EXT_validation_cache)
    // TODO: Create another validation cache, preferably different from the first
    // one and merge it here.  Then store the merged one.

    // Store away the cache that we've populated.  This could conceivably happen
    // earlier, depends on when the validation cache stops being populated
    // internally.
    size_t endCacheSize = 0;
    void *endCacheData = nullptr;

    // Call with nullptr to get cache size
    res = vkGetValidationCacheData(info.device, validationCache, &endCacheSize, nullptr);
    assert(res == VK_SUCCESS);

    // Allocate memory to hold the populated cache data
    endCacheData = (char *)malloc(sizeof(char) * endCacheSize);
    if (!endCacheData) {
        fputs("Memory error", stderr);
        exit(EXIT_FAILURE);
    }

    // Call again with pointer to buffer
    res = vkGetValidationCacheData(info.device, validationCache, &endCacheSize, endCacheData);
    assert(res == VK_SUCCESS);

    // Write the file to disk, overwriting whatever was there
    FILE *pWriteFile;
    std::string writeFileName = directoryName + "validation_cache_data.bin";
    pWriteFile = fopen(writeFileName.c_str(), "wb");
    if (pWriteFile) {
        fwrite(endCacheData, sizeof(char), endCacheSize, pWriteFile);
        fclose(pWriteFile);
        printf("  %u bytes of cacheData written to %s\n", static_cast<uint32_t>(endCacheSize), writeFileName.c_str());
    } else {
        // Something bad happened
        printf("  Unable to write cache data to disk!\n");
    }
    vkDestroyValidationCache(info.device, validationCache, NULL);
#endif

    /* VULKAN_KEY_END */

    vkDestroyFence(info.device, drawFence, NULL);
    vkDestroySemaphore(info.device, info.imageAcquiredSemaphore, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_textures(info);
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
    destroy_device(info);
    destroy_window(info);
    destroy_instance(info);
    return 0;
}
