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

#include "glv_platform.h"
#include "glvtrace_vk_vk.h"
#include "vk_struct_size_helper.h"

// defined in generated file: glvtrace_vk_vk.c
extern BOOL isHooked;

// Support for shadowing CPU mapped memory
typedef struct _VKAllocInfo {
    VkGpuSize   size;
    VkGpuMemory handle;
    void           *pData;
    BOOL           valid;
} VKAllocInfo;

typedef struct _VKMemInfo {
    unsigned int numEntrys;
    VKAllocInfo *pEntrys;
    VKAllocInfo *pLastMapped;
    unsigned int capacity;
} VKMemInfo;

// defined in manually written file: glvtrace_vk_trace.c
extern VKMemInfo g_memInfo;
extern GLV_CRITICAL_SECTION g_memInfoLock;

static void init_mem_info_entrys(VKAllocInfo *ptr, const unsigned int num)
{
    unsigned int i;
    for (i = 0; i < num; i++)
    {
        VKAllocInfo *entry = ptr + i;
        entry->pData = NULL;
        entry->size  = 0;
        entry->handle = NULL;
        entry->valid = FALSE;
    }
}

// caller must hold the g_memInfoLock
static void init_mem_info()
{
    g_memInfo.numEntrys = 0;
    g_memInfo.capacity = 4096;
    g_memInfo.pLastMapped = NULL;

    g_memInfo.pEntrys = GLV_NEW_ARRAY(VKAllocInfo, g_memInfo.capacity);

    if (g_memInfo.pEntrys == NULL)
        glv_LogError("init_mem_info()  malloc failed\n");
    else
        init_mem_info_entrys(g_memInfo.pEntrys, g_memInfo.capacity);
}

// caller must hold the g_memInfoLock
static void delete_mem_info()
{
    GLV_DELETE(g_memInfo.pEntrys);
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
        glv_LogError("get_mem_info_entry() bad internal state numEntrys %u\n", g_memInfo.numEntrys);
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
        g_memInfo.pEntrys = (VKAllocInfo *) GLV_REALLOC(g_memInfo.pEntrys, g_memInfo.capacity * sizeof(VKAllocInfo));
        if (g_memInfo.pEntrys == NULL)
            glv_LogError("get_mem_info_entry() realloc failed\n");
        //glv_LogInfo("realloc memInfo from %u to %u\n", g_memInfo.capacity /2, g_memInfo.capacity);
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
static VKAllocInfo * find_mem_info_entry(const VkGpuMemory handle)
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

static VKAllocInfo * find_mem_info_entry_lock(const VkGpuMemory handle)
{
    VKAllocInfo *res;
    glv_enter_critical_section(&g_memInfoLock);
    res = find_mem_info_entry(handle);
    glv_leave_critical_section(&g_memInfoLock);
    return res;
}

static void add_new_handle_to_mem_info(const VkGpuMemory handle, VkGpuSize size, void *pData)
{
    VKAllocInfo *entry;

    glv_enter_critical_section(&g_memInfoLock);
    if (g_memInfo.capacity == 0)
        init_mem_info();

    entry = get_mem_info_entry();
    if (entry)
    {
        entry->valid = TRUE;
        entry->handle = handle;
        entry->size = size;
        entry->pData = pData;   // NOTE: VKFreeMemory will free this mem, so no malloc()
    }
    glv_leave_critical_section(&g_memInfoLock);
}

static void add_data_to_mem_info(const VkGpuMemory handle, void *pData)
{
    VKAllocInfo *entry;

    glv_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(handle);
    if (entry)
    {
        entry->pData = pData;
    }
    g_memInfo.pLastMapped = entry;
    glv_leave_critical_section(&g_memInfoLock);
}

static void rm_handle_from_mem_info(const VkGpuMemory handle)
{
    VKAllocInfo *entry;

    glv_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(handle);
    if (entry)
    {
        entry->valid = FALSE;
        entry->pData = NULL;
        entry->size = 0;
        entry->handle = NULL;

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
    glv_leave_critical_section(&g_memInfoLock);
}


static void add_begin_cmdbuf_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    const VkCmdBufferBeginInfo* pInNow = pIn;
    VkCmdBufferBeginInfo** ppOutNext = (VkCmdBufferBeginInfo**)ppOut;
    while (pInNow != NULL)
    {
        VkCmdBufferBeginInfo** ppOutNow = ppOutNext;
        ppOutNext = NULL;

        switch (pInNow->sType)
        {
            case VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkCmdBufferGraphicsBeginInfo), pInNow);
                ppOutNext = (VkCmdBufferBeginInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            default:
                assert(!"Encountered an unexpected type in cmdbuffer_begin_info list");
        }
        pInNow = (VkCmdBufferBeginInfo*)pInNow->pNext;
    }
    return;
}

static void add_alloc_memory_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    const VkMemoryAllocInfo* pInNow = pIn;
    while (pInNow != NULL)
    {

        switch (pInNow->sType)
        {
        default:
            assert(!"Encountered an unexpected type in memory_alloc_info list");
        }
        pInNow = (VkMemoryAllocInfo*)pInNow->pNext;
    }
    return;
}

static size_t calculate_memory_barrier_size(uint32_t mbCount, const void** ppMemBarriers)
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

static void add_pipeline_shader_to_trace_packet(glv_trace_packet_header* pHeader, VkPipelineShader* packetShader, const VkPipelineShader* paramShader)
{
    uint32_t i;
    // constant buffers
    if (paramShader->linkConstBufferCount > 0 && paramShader->pLinkConstBufferInfo != NULL)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo), sizeof(VkLinkConstBuffer) * paramShader->linkConstBufferCount, paramShader->pLinkConstBufferInfo);
        for (i = 0; i < paramShader->linkConstBufferCount; i++)
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo[i].pBufferData), packetShader->pLinkConstBufferInfo[i].bufferSize, paramShader->pLinkConstBufferInfo[i].pBufferData);
        }
    }
}

static void finalize_pipeline_shader_address(glv_trace_packet_header* pHeader, const VkPipelineShader* packetShader)
{
    uint32_t i;
    // constant buffers
    if (packetShader->linkConstBufferCount > 0 && packetShader->pLinkConstBufferInfo != NULL)
    {
        for (i = 0; i < packetShader->linkConstBufferCount; i++)
        {
            glv_finalize_buffer_address(pHeader, (void**)&(packetShader->pLinkConstBufferInfo[i].pBufferData));
        }
        glv_finalize_buffer_address(pHeader, (void**)&(packetShader->pLinkConstBufferInfo));
    }
}

static void add_create_ds_layout_to_trace_packet(glv_trace_packet_header* pHeader, const VkDescriptorSetLayoutCreateInfo** ppOut, const VkDescriptorSetLayoutCreateInfo* pIn)
{
    const VkDescriptorSetLayoutCreateInfo* pInNow = pIn;
    VkDescriptorSetLayoutCreateInfo** ppOutNext = (VkDescriptorSetLayoutCreateInfo**)ppOut;
    while (pInNow != NULL)
    {
        VkDescriptorSetLayoutCreateInfo** ppOutNow = ppOutNext;
        size_t i;
        ppOutNext = NULL;
        glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkDescriptorSetLayoutCreateInfo), pInNow);
        ppOutNext = (VkDescriptorSetLayoutCreateInfo**)&(*ppOutNow)->pNext;
        glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
        for (i = 0; i < pInNow->count; i++)
        {
            VkDescriptorSetLayoutBinding *pLayoutBinding =  (VkDescriptorSetLayoutBinding *) pInNow->pBinding + i;
            VkDescriptorSetLayoutBinding *pOutLayoutBinding =  (VkDescriptorSetLayoutBinding *) (*ppOutNow)->pBinding + i;
            glv_add_buffer_to_trace_packet(pHeader, (void**) &pOutLayoutBinding->pImmutableSamplers, sizeof(VkSampler) * pLayoutBinding->count, pLayoutBinding->pImmutableSamplers);
            glv_finalize_buffer_address(pHeader, (void**) &pOutLayoutBinding->pImmutableSamplers);
        }
        glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppOutNow)->pBinding), sizeof(VkDescriptorSetLayoutBinding) * pInNow->count, pInNow->pBinding);
        glv_finalize_buffer_address(pHeader, (void**)&((*ppOutNow)->pBinding));
        ppOutNext = (VkDescriptorSetLayoutCreateInfo**)&(*ppOutNow)->pNext;
        pInNow = (VkDescriptorSetLayoutCreateInfo*)pInNow->pNext;
    }
    return;
}

static void add_update_descriptors_to_trace_packet(glv_trace_packet_header* pHeader, const uint32_t count, void*** pppUpdateArrayOut, const void** ppUpdateArrayIn)
{
    uint32_t i;
    for (i = 0; i < count; i++)
    {
        const VkUpdateSamplers* pInNow = (const VkUpdateSamplers*)ppUpdateArrayIn[i];
        VkUpdateSamplers** ppOut = (VkUpdateSamplers**)*pppUpdateArrayOut;
        VkUpdateSamplers** ppOutNow = &(ppOut[i]);
        switch (pInNow->sType)
        {
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkUpdateSamplers), pInNow);
            VkUpdateSamplers* pPacket = (VkUpdateSamplers*)*ppOutNow;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplers, ((VkUpdateSamplers*)pInNow)->count * sizeof(VkSampler), pInNow->pSamplers);
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplers));
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
        {
//            totalUpdateSize += sizeof(VkUpdateSamplerTextures) + ((VkUpdateSamplerTextures*)pNext)->count * (sizeof(VkSamplerImageViewInfo) + sizeof(VkImageViewAttachInfo));
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkUpdateSamplerTextures), pInNow);
            VkUpdateSamplerTextures* pPacket = (VkUpdateSamplerTextures*)*ppOutNow;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplerImageViews, ((VkUpdateSamplerTextures*)pInNow)->count * sizeof(VkSamplerImageViewInfo), ((VkUpdateSamplerTextures*)pInNow)->pSamplerImageViews);
// TODO : is the below correct? is pImageView a pointer to a single struct or not?
            uint32_t j;
            for (j = 0; j < ((VkUpdateSamplerTextures*)pInNow)->count; j++) {
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplerImageViews[j].pImageView, sizeof(VkImageViewAttachInfo), ((VkUpdateSamplerTextures*)pInNow)->pSamplerImageViews[j].pImageView);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplerImageViews[j].pImageView));
            }
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplerImageViews));
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkUpdateImages), pInNow);
            VkUpdateImages* pPacket = (VkUpdateImages*)*ppOutNow;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pImageViews, ((VkUpdateImages*)pInNow)->count * sizeof(VkImageViewAttachInfo), ((VkUpdateImages*)pInNow)->pImageViews);
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageViews));
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkUpdateBuffers), pInNow);
            VkUpdateBuffers* pPacket = (VkUpdateBuffers*)*ppOutNow;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pBufferViews, ((VkUpdateBuffers*)pInNow)->count * sizeof(VkBufferViewAttachInfo), ((VkUpdateBuffers*)pInNow)->pBufferViews);
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBufferViews));
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkUpdateAsCopy), pInNow);
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        default:
        {
            assert(0);
        }
        }
        pInNow = (VkUpdateSamplers*)pInNow->pNext;
    }
    return;
}

static void add_pipeline_state_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    const VkGraphicsPipelineCreateInfo* pInNow = pIn;
    VkGraphicsPipelineCreateInfo** ppOutNext = (VkGraphicsPipelineCreateInfo**)ppOut;
    while (pInNow != NULL)
    {
        VkGraphicsPipelineCreateInfo** ppOutNow = ppOutNext;
        ppOutNext = NULL;

        switch (pInNow->sType)
        {
            case VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO: {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineIaStateCreateInfo), pInNow);
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO: {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineTessStateCreateInfo), pInNow);
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO: {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineRsStateCreateInfo), pInNow);
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO: {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineDsStateCreateInfo), pInNow);
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO: {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineVpStateCreateInfo), pInNow);
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO: {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineMsStateCreateInfo), pInNow);
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            {
                VkPipelineCbStateCreateInfo *pPacket = NULL;
                VkPipelineCbStateCreateInfo *pIn = NULL;
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineCbStateCreateInfo), pInNow);
                pPacket = (VkPipelineCbStateCreateInfo*) *ppOutNow;
                pIn = (VkPipelineCbStateCreateInfo*) pInNow;
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pAttachments, pIn->attachmentCount * sizeof(VkPipelineCbAttachmentState), pIn->pAttachments);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAttachments));
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            {
                VkPipelineShaderStageCreateInfo* pPacket = NULL;
                VkPipelineShaderStageCreateInfo* pInPacket = NULL;
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineShaderStageCreateInfo), pInNow);
                pPacket = (VkPipelineShaderStageCreateInfo*) *ppOutNow;
                pInPacket = (VkPipelineShaderStageCreateInfo*) pInNow;
                add_pipeline_shader_to_trace_packet(pHeader, &pPacket->shader, &pInPacket->shader);
                finalize_pipeline_shader_address(pHeader, &pPacket->shader);
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
            {
                VkPipelineVertexInputCreateInfo *pPacket = NULL;
                VkPipelineVertexInputCreateInfo *pIn = NULL;
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(VkPipelineVertexInputCreateInfo), pInNow);
                pPacket = (VkPipelineVertexInputCreateInfo*) *ppOutNow;
                pIn = (VkPipelineVertexInputCreateInfo*) pInNow;
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pVertexBindingDescriptions, pIn->bindingCount * sizeof(VkVertexInputBindingDescription), pIn->pVertexBindingDescriptions);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexBindingDescriptions));
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pVertexAttributeDescriptions, pIn->attributeCount * sizeof(VkVertexInputAttributeDescription), pIn->pVertexAttributeDescriptions);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexAttributeDescriptions));
                ppOutNext = (VkGraphicsPipelineCreateInfo**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            default:
                assert(!"Encountered an unexpected type in pipeline state list");
        }
        pInNow = (VkGraphicsPipelineCreateInfo*)pInNow->pNext;
    }
    return;
}
