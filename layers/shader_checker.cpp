/*
 * Vulkan
 *
 * Copyright (C) 2015 LunarG, Inc.
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <map>
#include <unordered_map>
#include <map>
#include <vector>
#include "loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vkLayer.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

#include "SPIRV/spirv.h"

static std::unordered_map<void *, VkLayerDispatchTable *> tableMap;


struct shader_source {
    std::vector<uint32_t> words;

    shader_source(VkShaderCreateInfo const *pCreateInfo) :
        words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)) {
    }
};


static std::unordered_map<void *, shader_source *> shader_map;


static VkLayerDispatchTable * initLayerTable(const VkBaseLayerObject *gpuw)
{
    VkLayerDispatchTable *pTable;

    assert(gpuw);
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) gpuw->baseObject);
    if (it == tableMap.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap[(void *) gpuw->baseObject] = pTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (VkPhysicalGpu) gpuw->nextObject);

    return pTable;
}


VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalGpu gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkLayerDispatchTable* pTable = tableMap[gpu];
    VkResult result = pTable->CreateDevice(gpu, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table
    tableMap.emplace(*pDevice, pTable);
    return result;
}


VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalGpu gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pOutLayers[1] == NULL || pReserved == NULL)
        return VK_ERROR_INVALID_POINTER;

    if (maxLayerCount < 1)
        return VK_ERROR_INITIALIZATION_FAILED;
    *pOutLayerCount = 1;
    strncpy((char *) pOutLayers[0], "ShaderChecker", maxStringSize);
    return VK_SUCCESS;
}


struct extProps {
    uint32_t version;
    const char * const name;
};
#define SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE 1
static const struct extProps shaderCheckerExts[SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE] = {
    // TODO what is the version?
    0x10, "ShaderChecker",
};


VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    VkResult result;

    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    VkExtensionProperties *ext_props;
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = shaderCheckerExts[extensionIndex].version;
            strncpy(ext_props->extName, shaderCheckerExts[extensionIndex].name,
                                        VK_MAX_EXTENSION_NAME);
            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}


static int
value_or_default(std::unordered_map<unsigned, unsigned> const &map, unsigned id, int def)
{
    auto it = map.find(id);
    if (it == map.end())
        return def;
    else
        return it->second;
}


struct interface_var {
    uint32_t id;
    uint32_t type_id;
    /* TODO: collect the name, too? Isn't required to be present. */
};


static void
collect_interface_by_location(shader_source const *src, spv::StorageClass interface,
                              std::map<uint32_t, interface_var> &out,
                              std::map<uint32_t, interface_var> &builtins_out)
{
    unsigned int const *code = (unsigned int const *)&src->words[0];
    size_t size = src->words.size();

    if (code[0] != spv::MagicNumber) {
        printf("Invalid magic.\n");
        return;
    }

    std::unordered_map<unsigned, unsigned> var_locations;
    std::unordered_map<unsigned, unsigned> var_builtins;

    unsigned word = 5;
    while (word < size) {

        unsigned opcode = code[word] & 0x0ffffu;
        unsigned oplen = (code[word] & 0xffff0000u) >> 16;

        /* We consider two interface models: SSO rendezvous-by-location, and
         * builtins. Complain about anything that fits neither model.
         */
        if (opcode == spv::OpDecorate) {
            if (code[word+2] == spv::DecLocation) {
                var_locations[code[word+1]] = code[word+3];
            }

            if (code[word+2] == spv::DecBuiltIn) {
                var_builtins[code[word+1]] = code[word+3];
            }
        }

        /* TODO: handle grouped decorations */
        /* TODO: handle index=1 dual source outputs from FS -- two vars will
         * have the same location, and we DONT want to clobber. */

        if (opcode == spv::OpVariable && code[word+3] == interface) {
            int location = value_or_default(var_locations, code[word+2], -1);
            int builtin = value_or_default(var_builtins, code[word+2], -1);

            if (location == -1 && builtin == -1) {
                /* No location defined, and not bound to an API builtin.
                 * The spec says nothing about how this case works (or doesn't)
                 * for interface matching.
                 */
                printf("WARN: var %d (type %d) in %s interface has no Location or Builtin decoration\n",
                       code[word+2], code[word+1], interface == spv::StorageInput ? "input" : "output");
            }
            else if (location != -1) {
                /* A user-defined interface variable, with a location. */
                interface_var v;
                v.id = code[word+2];
                v.type_id = code[word+1];
                out[location] = v;
            }
            else {
                /* A builtin interface variable */
                interface_var v;
                v.id = code[word+2];
                v.type_id = code[word+1];
                builtins_out[builtin] = v;
            }
        }

        word += oplen;
    }
}


VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo *pCreateInfo,
                                                   VkShader *pShader)
{
    VkLayerDispatchTable* pTable = tableMap[(VkBaseLayerObject *)device];
    VkResult res = pTable->CreateShader(device, pCreateInfo, pShader);

    shader_map[(VkBaseLayerObject *) *pShader] = new shader_source(pCreateInfo);
    return res;
}


static void
validate_interface_between_stages(shader_source const *producer, char const *producer_name,
                                  shader_source const *consumer, char const *consumer_name)
{
    std::map<uint32_t, interface_var> outputs;
    std::map<uint32_t, interface_var> inputs;

    std::map<uint32_t, interface_var> builtin_outputs;
    std::map<uint32_t, interface_var> builtin_inputs;

    printf("Begin validate_interface_between_stages %s -> %s\n",
           producer_name, consumer_name);

    collect_interface_by_location(producer, spv::StorageOutput, outputs, builtin_outputs);
    collect_interface_by_location(consumer, spv::StorageInput, inputs, builtin_inputs);

    auto a_it = outputs.begin();
    auto b_it = inputs.begin();

    /* maps sorted by key (location); walk them together to find mismatches */
    while (a_it != outputs.end() || b_it != inputs.end()) {
        if (b_it == inputs.end() || a_it->first < b_it->first) {
            printf("  WARN: %s writes to output location %d which is not consumed by %s\n",
                   producer_name, a_it->first, consumer_name);
            a_it++;
        }
        else if (a_it == outputs.end() || a_it->first > b_it->first) {
            printf("  ERR: %s consumes input location %d which is not written by %s\n",
                   consumer_name, b_it->first, producer_name);
            b_it++;
        }
        else {
            printf("  OK: match on location %d\n",
                   a_it->first);
            /* TODO: typecheck */
            a_it++;
            b_it++;
        }
    }

    printf("End validate_interface_between_stages\n");
}


VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device,
                                                             const VkGraphicsPipelineCreateInfo *pCreateInfo,
                                                             VkPipeline *pPipeline)
{
    /* TODO: run cross-stage validation */
    /* - Validate vertex fetch -> VS interface */
    /* - Validate FS output -> CB */
    /* - Support GS, TCS, TES stages */

    /* We seem to allow pipeline stages to be specified out of order, so collect and identify them
     * before trying to do anything more: */

    shader_source const *vs_source = 0;
    shader_source const *fs_source = 0;
    VkPipelineCbStateCreateInfo const *cb = 0;
    VkPipelineVertexInputCreateInfo const *vi = 0;

    for (auto stage = pCreateInfo; stage; stage = (decltype(stage))stage->pNext) {
        if (stage->sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO) {
            auto shader_stage = (VkPipelineShaderStageCreateInfo const *)stage;

            if (shader_stage->shader.stage == VK_SHADER_STAGE_VERTEX)
                vs_source = shader_map[(void *)(shader_stage->shader.shader)];
            else if (shader_stage->shader.stage == VK_SHADER_STAGE_FRAGMENT)
                fs_source = shader_map[(void *)(shader_stage->shader.shader)];
            else
                printf("Unknown shader stage %d\n", shader_stage->shader.stage);
        }
        else if (stage->sType == VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO) {
            cb = (VkPipelineCbStateCreateInfo const *)stage;
        }
        else if (stage->sType == VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO) {
            vi = (VkPipelineVertexInputCreateInfo const *)stage;
        }
    }

    printf("Pipeline: vi=%p vs=%p fs=%p cb=%p\n", vi, vs_source, fs_source, cb);

    if (vs_source && fs_source) {
        validate_interface_between_stages(vs_source, "vertex shader",
                                          fs_source, "fragment shader");
    }

    VkLayerDispatchTable *pTable = tableMap[(VkBaseLayerObject *)device];
    VkResult res = pTable->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    return res;
}


VK_LAYER_EXPORT void * VKAPI vkGetProcAddr(VkPhysicalGpu gpu, const char* pName)
{
    if (gpu == NULL)
        return NULL;

    initLayerTable((const VkBaseLayerObject *) gpu);

#define ADD_HOOK(fn)    \
    if (!strncmp(#fn, pName, sizeof(#fn))) \
        return (void *) fn

    ADD_HOOK(vkGetProcAddr);
    ADD_HOOK(vkEnumerateLayers);
    ADD_HOOK(vkCreateDevice);
    ADD_HOOK(vkCreateShader);
    ADD_HOOK(vkCreateGraphicsPipeline);

    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    if (gpuw->pGPA == NULL)
        return NULL;
    return gpuw->pGPA((VkPhysicalGpu) gpuw->nextObject, pName);
}
