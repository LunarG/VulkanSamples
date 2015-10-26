/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 * Copyright (C) 2015 Valve Corporation
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
#pragma once
#include <unordered_map>
#include "vk_layer.h"
#include "vktrace_platform.h"
#include "vktrace_vk_vk.h"
#include "vk_struct_size_helper.h"
#include "vk_debug_marker_layer.h"

// Support for shadowing CPU mapped memory
//TODO better handling of multiple range rather than fixed array
typedef struct _VKAllocInfo {
    VkDeviceSize   totalSize;
    VkDeviceSize   rangeSize;
    VkDeviceSize   rangeOffset;
    BOOL           didFlush;
    VkDeviceMemory handle;
    uint8_t        *pData;
    BOOL           valid;
} VKAllocInfo;

typedef struct _VKMemInfo {
    unsigned int numEntrys;
    VKAllocInfo *pEntrys;
    VKAllocInfo *pLastMapped;
    unsigned int capacity;
} VKMemInfo;

typedef struct _layer_device_data {
    VkLayerDispatchTable devTable;
    bool LunargDebugMarkerEnabled;
    VkLayerDebugMarkerDispatchTable debugMarkerTable;
    bool KHRDeviceSwapchainEnabled;
} layer_device_data;
typedef struct _layer_instance_data {
    VkLayerInstanceDispatchTable instTable;
    bool LunargDebugReportEnabled;
    bool KHRSwapchainEnabled;
} layer_instance_data;

// defined in manually written file: vktrace_lib_trace.c
extern VKMemInfo g_memInfo;
extern VKTRACE_CRITICAL_SECTION g_memInfoLock;
extern std::unordered_map<void *, layer_device_data *> g_deviceDataMap;
extern std::unordered_map<void *, layer_instance_data *> g_instanceDataMap;

typedef void *dispatch_key;
inline dispatch_key get_dispatch_key(const void* object)
{
    return (dispatch_key) *(VkLayerDispatchTable **) object;
}

layer_instance_data *mid(void *object);
layer_device_data *mdd(void* object);

static void init_mem_info_entrys(VKAllocInfo *ptr, const unsigned int num)
{
    unsigned int i;
    for (i = 0; i < num; i++)
    {
        VKAllocInfo *entry = ptr + i;
        entry->pData = NULL;
        entry->totalSize = 0;
        entry->rangeSize = 0;
        entry->rangeOffset = 0;
        entry->didFlush = FALSE;
        memset(&entry->handle, 0, sizeof(VkDeviceMemory));
        entry->valid = FALSE;
    }
}

// caller must hold the g_memInfoLock
static void init_mem_info()
{
    g_memInfo.numEntrys = 0;
    g_memInfo.capacity = 4096;
    g_memInfo.pLastMapped = NULL;

    g_memInfo.pEntrys = VKTRACE_NEW_ARRAY(VKAllocInfo, g_memInfo.capacity);

    if (g_memInfo.pEntrys == NULL)
        vktrace_LogError("init_mem_info()  malloc failed.");
    else
        init_mem_info_entrys(g_memInfo.pEntrys, g_memInfo.capacity);
}

// caller must hold the g_memInfoLock
static void delete_mem_info()
{
    VKTRACE_DELETE(g_memInfo.pEntrys);
    g_memInfo.pEntrys = NULL;
    g_memInfo.numEntrys = 0;
    g_memInfo.capacity = 0;
    g_memInfo.pLastMapped = NULL;
}

// caller must hold the g_memInfoLock
static VKAllocInfo * get_mem_info_entry()
{
    unsigned int i;
    VKAllocInfo *entry;
    if (g_memInfo.numEntrys > g_memInfo.capacity)
    {
        vktrace_LogError("get_mem_info_entry() bad internal state numEntrys %u.", g_memInfo.numEntrys);
        return NULL;
    }

    entry = g_memInfo.pEntrys;
    for (i = 0; i < g_memInfo.numEntrys; i++)
    {
        if ((entry + i)->valid == FALSE)
            return entry + i;
    }
    if (g_memInfo.numEntrys == g_memInfo.capacity)
    {  // grow the array 2x
        g_memInfo.capacity *= 2;
        g_memInfo.pEntrys = (VKAllocInfo *) VKTRACE_REALLOC(g_memInfo.pEntrys, g_memInfo.capacity * sizeof(VKAllocInfo));
        if (g_memInfo.pEntrys == NULL)
            vktrace_LogError("get_mem_info_entry() realloc failed.");
        //vktrace_LogDebug("realloc memInfo from %u to %", g_memInfo.capacity /2, g_memInfo.capacity);
        //init the newly added entrys
        init_mem_info_entrys(g_memInfo.pEntrys + g_memInfo.capacity / 2, g_memInfo.capacity / 2);
    }

    assert(g_memInfo.numEntrys < g_memInfo.capacity);
    entry = g_memInfo.pEntrys + g_memInfo.numEntrys;
    g_memInfo.numEntrys++;
    assert(entry->valid == FALSE);
    return entry;
}

// caller must hold the g_memInfoLock
static VKAllocInfo * find_mem_info_entry(const VkDeviceMemory handle)
{
    VKAllocInfo *entry;
    unsigned int i;
    entry = g_memInfo.pEntrys;
    if (g_memInfo.pLastMapped && g_memInfo.pLastMapped->handle == handle && g_memInfo.pLastMapped->valid)
    {
        return g_memInfo.pLastMapped;
    }
    for (i = 0; i < g_memInfo.numEntrys; i++)
    {
        if ((entry + i)->valid && (handle == (entry + i)->handle))
        {
            return entry + i;
        }
    }

    return NULL;
}

static VKAllocInfo * find_mem_info_entry_lock(const VkDeviceMemory handle)
{
    VKAllocInfo *res;
    vktrace_enter_critical_section(&g_memInfoLock);
    res = find_mem_info_entry(handle);
    vktrace_leave_critical_section(&g_memInfoLock);
    return res;
}

static void add_new_handle_to_mem_info(const VkDeviceMemory handle, VkDeviceSize size, void *pData)
{
    VKAllocInfo *entry;

    vktrace_enter_critical_section(&g_memInfoLock);
    if (g_memInfo.capacity == 0)
        init_mem_info();

    entry = get_mem_info_entry();
    if (entry)
    {
        entry->valid = TRUE;
        entry->handle = handle;
        entry->totalSize = size;
        entry->rangeSize = 0;
        entry->rangeOffset = 0;
        entry->didFlush = FALSE;
        entry->pData = (uint8_t *) pData;   // NOTE: VKFreeMemory will free this mem, so no malloc()
    }
    vktrace_leave_critical_section(&g_memInfoLock);
}

static void add_data_to_mem_info(const VkDeviceMemory handle, VkDeviceSize rangeSize, VkDeviceSize rangeOffset, void *pData)
{
    VKAllocInfo *entry;

    vktrace_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(handle);
    if (entry)
    {
        entry->pData = (uint8_t *) pData;
        if (rangeSize == 0)
            entry->rangeSize = entry->totalSize - rangeOffset;
        else
            entry->rangeSize = rangeSize;
        entry->rangeOffset = entry->rangeOffset;
        assert(entry->totalSize >= rangeSize + rangeOffset);
    }
    g_memInfo.pLastMapped = entry;
    vktrace_leave_critical_section(&g_memInfoLock);
}

static void rm_handle_from_mem_info(const VkDeviceMemory handle)
{
    VKAllocInfo *entry;

    vktrace_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(handle);
    if (entry)
    {
        entry->valid = FALSE;
        entry->pData = NULL;
        entry->totalSize = 0;
        entry->rangeSize = 0;
        entry->rangeOffset = 0;
        entry->didFlush = FALSE;
        memset(&entry->handle, 0, sizeof(VkDeviceMemory));

        if (entry == g_memInfo.pLastMapped)
            g_memInfo.pLastMapped = NULL;
        // adjust numEntrys to be last valid entry in list
        do {
            entry =  g_memInfo.pEntrys + g_memInfo.numEntrys - 1;
            if (entry->valid == FALSE)
                g_memInfo.numEntrys--;
        } while ((entry->valid == FALSE) && (g_memInfo.numEntrys > 0));
        if (g_memInfo.numEntrys == 0)
            delete_mem_info();
    }
    vktrace_leave_critical_section(&g_memInfoLock);
}

static void add_alloc_memory_to_trace_packet(vktrace_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    return;
}

static size_t calculate_memory_barrier_size(uint32_t mbCount, const void* const* ppMemBarriers)
{
    uint32_t i, siz=0;
    for (i = 0; i < mbCount; i++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) ppMemBarriers[i];
        switch (pNext->sType) {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz += sizeof(VkMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz += sizeof(VkBufferMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz += sizeof(VkImageMemoryBarrier);
                break;
            default:
                assert(0);
                break;
        }
    }
    return siz;
}

static void add_VkPipelineShaderStageCreateInfo_to_trace_packet(vktrace_trace_packet_header* pHeader, VkPipelineShaderStageCreateInfo* packetShader, const VkPipelineShaderStageCreateInfo* paramShader)
{
    // Specialization info
    if (packetShader->pSpecializationInfo != NULL)
    {
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&packetShader->pSpecializationInfo, sizeof(VkPipelineShaderStageCreateInfo), paramShader->pSpecializationInfo);
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&packetShader->pSpecializationInfo->pMap, sizeof(VkSpecializationMapEntry) * paramShader->pSpecializationInfo->mapEntryCount, paramShader->pSpecializationInfo->pMap);
        // TODO: packetShader->pSpecializationInfo->pData is not yet supported because we don't know what size it is.
        //   We now have dataSize so fix this
        vktrace_LogError("VkSpecializationInfo is not yet supported because we don't know how many bytes it is.");
        vktrace_finalize_buffer_address(pHeader, (void**)&packetShader->pSpecializationInfo->pMap);
        vktrace_finalize_buffer_address(pHeader, (void**)&packetShader->pSpecializationInfo);
    }
}

static void add_create_ds_layout_to_trace_packet(vktrace_trace_packet_header* pHeader, const VkDescriptorSetLayoutCreateInfo** ppOut, const VkDescriptorSetLayoutCreateInfo* pIn)
{
    const VkDescriptorSetLayoutCreateInfo* pInNow = pIn;
    VkDescriptorSetLayoutCreateInfo** ppOutNext = (VkDescriptorSetLayoutCreateInfo**)ppOut;
    while (pInNow != NULL)
    {
        VkDescriptorSetLayoutCreateInfo** ppOutNow = ppOutNext;
        ppOutNext = NULL;
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkDescriptorSetLayoutCreateInfo), pInNow);
        ppOutNext = (VkDescriptorSetLayoutCreateInfo**)&(*ppOutNow)->pNext;
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&((*ppOutNow)->pBinding), sizeof(VkDescriptorSetLayoutBinding) * pInNow->count, pInNow->pBinding);
        vktrace_finalize_buffer_address(pHeader, (void**)&((*ppOutNow)->pBinding));
        ppOutNext = (VkDescriptorSetLayoutCreateInfo**)&(*ppOutNow)->pNext;
        pInNow = (VkDescriptorSetLayoutCreateInfo*)pInNow->pNext;
        vktrace_finalize_buffer_address(pHeader, (void**)(ppOutNow));
    }
    return;
}

static void add_VkGraphicsPipelineCreateInfos_to_trace_packet(vktrace_trace_packet_header* pHeader, VkGraphicsPipelineCreateInfo* pPacket, const VkGraphicsPipelineCreateInfo* pParam, uint32_t count)
{
    if (pParam != NULL)
    {
        uint32_t i;
        uint32_t j;

        for (i = 0; i < count; i++) {
            // shader stages array
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pStages), sizeof(VkPipelineShaderStageCreateInfo) * pParam[i].stageCount, pParam[i].pStages);
            for (j = 0; j < pParam[i].stageCount; j++)
            {
                add_VkPipelineShaderStageCreateInfo_to_trace_packet(pHeader, (VkPipelineShaderStageCreateInfo*)&pPacket->pStages[j], &pParam->pStages[j]);
            }
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pStages));

            // Vertex Input State
            if (pParam[i].pVertexInputState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pVertexInputState), sizeof(VkPipelineVertexInputStateCreateInfo), pParam[i].pVertexInputState);
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pVertexInputState->pVertexBindingDescriptions), pParam[i].pVertexInputState->bindingCount * sizeof(VkVertexInputBindingDescription), pParam[i].pVertexInputState->pVertexBindingDescriptions);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexInputState->pVertexBindingDescriptions));
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pVertexInputState->pVertexAttributeDescriptions), pParam[i].pVertexInputState->attributeCount * sizeof(VkVertexInputAttributeDescription), pParam[i].pVertexInputState->pVertexAttributeDescriptions);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexInputState->pVertexAttributeDescriptions));
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexInputState));
            }
            // Input Assembly State
            if (pParam[i].pInputAssemblyState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInputAssemblyState), sizeof(VkPipelineInputAssemblyStateCreateInfo), pParam[i].pInputAssemblyState);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pInputAssemblyState));
            }
            // Tesselation State
            if (pParam[i].pTessellationState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pTessellationState), sizeof(VkPipelineTessellationStateCreateInfo), pParam[i].pTessellationState);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pTessellationState));
            }
            // Viewport State
            if (pParam[i].pViewportState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pViewportState), sizeof(VkPipelineViewportStateCreateInfo), pParam[i].pViewportState);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pViewportState));
            }

            // Raster State
            if (pParam[i].pRasterState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRasterState), sizeof(VkPipelineRasterStateCreateInfo), pParam[i].pRasterState);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRasterState));
            }
            // MultiSample State
            if (pParam[i].pMultisampleState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMultisampleState), sizeof(VkPipelineMultisampleStateCreateInfo), pParam[i].pMultisampleState);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMultisampleState));
            }

            // DepthStencil State
            if (pParam[i].pDepthStencilState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDepthStencilState), sizeof(VkPipelineDepthStencilStateCreateInfo), pParam[i].pDepthStencilState);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDepthStencilState));
            }

            // ColorBlend State
            if (pParam[i].pColorBlendState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pColorBlendState), sizeof(VkPipelineColorBlendStateCreateInfo), pParam[i].pColorBlendState);
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pColorBlendState->pAttachments), pParam[i].pColorBlendState->attachmentCount * sizeof(VkPipelineColorBlendAttachmentState), pParam[i].pColorBlendState->pAttachments);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pColorBlendState->pAttachments));
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pColorBlendState));
            }

            // DynamicState
            if (pParam[i].pDynamicState) {
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDynamicState), sizeof(VkPipelineDynamicStateCreateInfo), pParam[i].pDynamicState);
                vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDynamicState->pDynamicStates), pParam[i].pDynamicState->dynamicStateCount * sizeof(VkDynamicState), pParam[i].pDynamicState->pDynamicStates);
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDynamicState->pDynamicStates));
                vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDynamicState));
            }
        }
    }
    return;
}

static void add_VkComputePipelineCreateInfos_to_trace_packet(vktrace_trace_packet_header* pHeader, VkComputePipelineCreateInfo* pPacket, const VkComputePipelineCreateInfo* pParam, uint32_t count)
{
    if (pParam != NULL)
    {
        uint32_t i;

        for (i = 0; i < count; i++) {

            // shader stage
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->stage), sizeof(VkPipelineShaderStageCreateInfo), &pParam[i].stage);
            add_VkPipelineShaderStageCreateInfo_to_trace_packet(pHeader, (VkPipelineShaderStageCreateInfo*)&pPacket->stage, &pParam[i].stage);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->stage));

/*
            // Vertex Input State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pVertexInputState), sizeof(VkPipelineVertexInputStateCreateInfo), pParam[i]->pVertexInputState);
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pVertexInputState->pVertexBindingDescriptions), pParam[i]->pVertexInputState->bindingCount * sizeof(VkVertexInputBindingDescription), pParam[i]->pVertexInputState->pVertexBindingDescriptions);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexInputState->pVertexBindingDescriptions));
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pVertexInputState->pVertexAttributeDescriptions), pParam[i]->pVertexInputState->attributeCount * sizeof(VkVertexInputAttributeDescription), pParam[i]->pVertexInputState->pVertexAttributeDescriptions);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexInputState->pVertexAttributeDescriptions));
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexInputState));

            // Input Assembly State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInputAssemblyState), sizeof(VkPipelineInputAssemblyStateCreateInfo), pParam[i]->pInputAssemblyState);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pInputAssemblyState));

            // Tesselation State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pTessellationState), sizeof(VkPipelineTessellationStateCreateInfo), pParam[i]->pTessellationState);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pTessellationState));

            // Viewport State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pViewportState), sizeof(VkPipelineViewportStateCreateInfo), pParam[i]->pViewportState);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pViewportState));

            // Raster State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRasterState), sizeof(VkPipelineRasterStateCreateInfo), pParam[i]->pRasterState);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRasterState));

            // MultiSample State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMultisampleState), sizeof(VkPipelineMultisampleStateCreateInfo), pParam[i]->pMultisampleState);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMultisampleState));

            // DepthStencil State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDepthStencilState), sizeof(VkPipelineDepthStencilStateCreateInfo), pParam[i]->pDepthStencilState);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDepthStencilState));

            // ColorBlend State
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pColorBlendState), sizeof(VkPipelineColorBlendStateCreateInfo), pParam[i]->pColorBlendState);
            vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pColorBlendState->pAttachments), pParam[i]->pColorBlendState->attachmentCount * sizeof(VkPipelineColorBlendAttachmentState), pParam[i]->pColorBlendState->pAttachments);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pColorBlendState->pAttachments));
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pColorBlendState));
*/
        }
    }
    return;
}
