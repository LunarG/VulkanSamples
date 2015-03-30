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
#include "glvtrace_xgl_xgl.h"
#include "xgl_struct_size_helper.h"

// defined in generated file: glvtrace_xgl_xgl.c
extern BOOL isHooked;

// Support for shadowing CPU mapped memory
typedef struct _XGLAllocInfo {
    XGL_GPU_SIZE   size;
    XGL_GPU_MEMORY handle;
    void           *pData;
    BOOL           valid;
} XGLAllocInfo;

typedef struct _XGLMemInfo {
    unsigned int numEntrys;
    XGLAllocInfo *pEntrys;
    XGLAllocInfo *pLastMapped;
    unsigned int capacity;
} XGLMemInfo;

// defined in manually written file: glvtrace_xgl_trace.c
extern XGLMemInfo g_memInfo;
extern GLV_CRITICAL_SECTION g_memInfoLock;

static void init_mem_info_entrys(XGLAllocInfo *ptr, const unsigned int num)
{
    unsigned int i;
    for (i = 0; i < num; i++)
    {
        XGLAllocInfo *entry = ptr + i;
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

    g_memInfo.pEntrys = GLV_NEW_ARRAY(XGLAllocInfo, g_memInfo.capacity);

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
static XGLAllocInfo * get_mem_info_entry()
{
    unsigned int i;
    XGLAllocInfo *entry;
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
        g_memInfo.pEntrys = (XGLAllocInfo *) GLV_REALLOC(g_memInfo.pEntrys, g_memInfo.capacity * sizeof(XGLAllocInfo));
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
static XGLAllocInfo * find_mem_info_entry(const XGL_GPU_MEMORY handle)
{
    XGLAllocInfo *entry;
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

static XGLAllocInfo * find_mem_info_entry_lock(const XGL_GPU_MEMORY handle)
{
    XGLAllocInfo *res;
    glv_enter_critical_section(&g_memInfoLock);
    res = find_mem_info_entry(handle);
    glv_leave_critical_section(&g_memInfoLock);
    return res;
}

static void add_new_handle_to_mem_info(const XGL_GPU_MEMORY handle, XGL_GPU_SIZE size, void *pData)
{
    XGLAllocInfo *entry;

    glv_enter_critical_section(&g_memInfoLock);
    if (g_memInfo.capacity == 0)
        init_mem_info();

    entry = get_mem_info_entry();
    if (entry)
    {
        entry->valid = TRUE;
        entry->handle = handle;
        entry->size = size;
        entry->pData = pData;   // NOTE: xglFreeMemory will free this mem, so no malloc()
    }
    glv_leave_critical_section(&g_memInfoLock);
}

static void add_data_to_mem_info(const XGL_GPU_MEMORY handle, void *pData)
{
    XGLAllocInfo *entry;

    glv_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(handle);
    if (entry)
    {
        entry->pData = pData;
    }
    g_memInfo.pLastMapped = entry;
    glv_leave_critical_section(&g_memInfoLock);
}

static void rm_handle_from_mem_info(const XGL_GPU_MEMORY handle)
{
    XGLAllocInfo *entry;

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
    const XGL_CMD_BUFFER_BEGIN_INFO* pInNow = pIn;
    XGL_CMD_BUFFER_BEGIN_INFO** ppOutNext = (XGL_CMD_BUFFER_BEGIN_INFO**)ppOut;
    while (pInNow != NULL)
    {
        XGL_CMD_BUFFER_BEGIN_INFO** ppOutNow = ppOutNext;
        ppOutNext = NULL;

        switch (pInNow->sType)
        {
            case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO), pInNow);
                ppOutNext = (XGL_CMD_BUFFER_BEGIN_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            default:
                assert(!"Encountered an unexpected type in cmdbuffer_begin_info list");
        }
        pInNow = (XGL_CMD_BUFFER_BEGIN_INFO*)pInNow->pNext;
    }
    return;
}

static void add_alloc_memory_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    const XGL_MEMORY_ALLOC_INFO* pInNow = pIn;
    XGL_MEMORY_ALLOC_INFO** ppOutNext = (XGL_MEMORY_ALLOC_INFO**)ppOut;
    while (pInNow != NULL)
    {
        XGL_MEMORY_ALLOC_INFO** ppOutNow = ppOutNext;
        ppOutNext = NULL;

        switch (pInNow->sType)
        {
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO), pInNow);
            ppOutNext = (XGL_MEMORY_ALLOC_INFO**)&(*ppOutNow)->pNext;
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO), pInNow);
            ppOutNext = (XGL_MEMORY_ALLOC_INFO**)&(*ppOutNow)->pNext;
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        default:
            assert(!"Encountered an unexpected type in memory_alloc_info list");
        }
        pInNow = (XGL_MEMORY_ALLOC_INFO*)pInNow->pNext;
    }
    return;
}

static size_t calculate_memory_barrier_size(uint32_t mbCount, const void** ppMemBarriers)
{
    uint32_t i, siz=0;
    for (i = 0; i < mbCount; i++) {
        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) ppMemBarriers[i];
        switch (pNext->sType) {
            case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz += sizeof(XGL_MEMORY_BARRIER);
                break;
            case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz += sizeof(XGL_BUFFER_MEMORY_BARRIER);
                break;
            case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz += sizeof(XGL_IMAGE_MEMORY_BARRIER);
                break;
            default:
                assert(0);
                break;
        }
    }
    return siz;
}

static void add_pipeline_shader_to_trace_packet(glv_trace_packet_header* pHeader, XGL_PIPELINE_SHADER* packetShader, const XGL_PIPELINE_SHADER* paramShader)
{
    uint32_t i;
    // constant buffers
    if (paramShader->linkConstBufferCount > 0 && paramShader->pLinkConstBufferInfo != NULL)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo), sizeof(XGL_LINK_CONST_BUFFER) * paramShader->linkConstBufferCount, paramShader->pLinkConstBufferInfo);
        for (i = 0; i < paramShader->linkConstBufferCount; i++)
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo[i].pBufferData), packetShader->pLinkConstBufferInfo[i].bufferSize, paramShader->pLinkConstBufferInfo[i].pBufferData);
        }
    }
}

static void finalize_pipeline_shader_address(glv_trace_packet_header* pHeader, const XGL_PIPELINE_SHADER* packetShader)
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

static void add_create_ds_layout_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pInNow = pIn;
    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO** ppOutNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO**)ppOut;
    while (pInNow != NULL)
    {
        XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO** ppOutNow = ppOutNext;
        size_t i;
        ppOutNext = NULL;
        glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO), pInNow);
        ppOutNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO**)&(*ppOutNow)->pNext;
        glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
        for (i = 0; i < pInNow->count; i++)
        {
            XGL_DESCRIPTOR_SET_LAYOUT_BINDING *pLayoutBinding =  (XGL_DESCRIPTOR_SET_LAYOUT_BINDING *) pInNow->pBinding + i;
            XGL_DESCRIPTOR_SET_LAYOUT_BINDING *pOutLayoutBinding =  (XGL_DESCRIPTOR_SET_LAYOUT_BINDING *) (*ppOutNow)->pBinding + i;
            glv_add_buffer_to_trace_packet(pHeader, (void**) &pOutLayoutBinding, sizeof(XGL_SAMPLER) * pLayoutBinding->count, pLayoutBinding->pImmutableSamplers);
            glv_finalize_buffer_address(pHeader, (void**) &pOutLayoutBinding);
        }
        glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppOutNow)->pBinding), sizeof(XGL_DESCRIPTOR_SET_LAYOUT_BINDING) * pInNow->count, pInNow->pBinding);
        glv_finalize_buffer_address(pHeader, (void**)&((*ppOutNow)->pBinding));
        pInNow = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pInNow->pNext;
    }
    return;
}

static void add_update_descriptors_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    const XGL_UPDATE_SAMPLERS* pInNow = pIn;
    XGL_UPDATE_SAMPLERS** ppOutNext = (XGL_UPDATE_SAMPLERS**)ppOut;
    while (pInNow != NULL)
    {
        XGL_UPDATE_SAMPLERS** ppOutNow = ppOutNext;
        ppOutNext = NULL;
        switch (pInNow->sType)
        {
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_SAMPLERS), pInNow);
            XGL_UPDATE_SAMPLERS* pPacket = (XGL_UPDATE_SAMPLERS*)*ppOutNow;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplers, ((XGL_UPDATE_SAMPLERS*)pInNow)->count * sizeof(XGL_SAMPLER), ((XGL_UPDATE_SAMPLERS*)pInNow)->pSamplers);
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplers));
            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
        {
//            totalUpdateSize += sizeof(XGL_UPDATE_SAMPLER_TEXTURES) + ((XGL_UPDATE_SAMPLER_TEXTURES*)pNext)->count * (sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO) + sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_SAMPLER_TEXTURES), pInNow);
            XGL_UPDATE_SAMPLER_TEXTURES* pPacket = (XGL_UPDATE_SAMPLER_TEXTURES*)*ppOutNow;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplerImageViews, ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->count * sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO), ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->pSamplerImageViews);
// TODO : This is still broken. How to update the original XGL_SAMPLER_IMAGE_VIEW_INFO struct ptrs to have correct address for newly added XGL_IMAGE_VIEW_ATTACH_INFO blocks below?
            uint32_t i;
            for (i = 0; i < ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->count; i++) {
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pSamplerImageViews[i].pImageView, sizeof(XGL_IMAGE_VIEW_ATTACH_INFO), ((XGL_UPDATE_SAMPLER_TEXTURES*)pInNow)->pSamplerImageViews[i].pImageView);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplerImageViews[i].pImageView));
            }
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplerImageViews));
            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_IMAGES), pInNow);
            XGL_UPDATE_IMAGES* pPacket = (XGL_UPDATE_IMAGES*)*ppOutNow;
            uint32_t i;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pImageViews, ((XGL_UPDATE_IMAGES*)pInNow)->count * sizeof(XGL_IMAGE_VIEW_ATTACH_INFO *), ((XGL_UPDATE_IMAGES*)pInNow)->pImageViews);
            for (i = 0; i < ((XGL_UPDATE_IMAGES*)pInNow)->count; i++) {
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pImageViews[i], sizeof(XGL_IMAGE_VIEW_ATTACH_INFO), ((XGL_UPDATE_IMAGES*)pInNow)->pImageViews[i]);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageViews[i]));
            }
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageViews));
            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_BUFFERS), pInNow);
            XGL_UPDATE_BUFFERS* pPacket = (XGL_UPDATE_BUFFERS*)*ppOutNow;
            glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pBufferViews, ((XGL_UPDATE_BUFFERS*)pInNow)->count * sizeof(XGL_BUFFER_VIEW_ATTACH_INFO *), ((XGL_UPDATE_BUFFERS*)pInNow)->pBufferViews);
            uint32_t i;
            for (i = 0; i < ((XGL_UPDATE_BUFFERS*)pInNow)->count; i++) {
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pBufferViews[i], sizeof(XGL_BUFFER_VIEW_ATTACH_INFO), ((XGL_UPDATE_BUFFERS*)pInNow)->pBufferViews[i]);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBufferViews[i]));
            }
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBufferViews));
            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_UPDATE_AS_COPY), pInNow);
            ppOutNext = (XGL_UPDATE_SAMPLERS**)&(*ppOutNow)->pNext;
            glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
            break;
        }
            default:
                assert(0);
        }
        pInNow = (XGL_UPDATE_SAMPLERS*)pInNow->pNext;
    }
    return;
}

#define CASE_XGL_STRUCTURE_TYPE_PIPELINE(type) \
    case XGL_STRUCTURE_TYPE_PIPELINE_##type: {\
        glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_##type), pInNow);\
        ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;\
        glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));\
        break;\
    }


static void add_pipeline_state_to_trace_packet(glv_trace_packet_header* pHeader, void** ppOut, const void* pIn)
{
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pInNow = pIn;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO** ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)ppOut;
    while (pInNow != NULL)
    {
        XGL_GRAPHICS_PIPELINE_CREATE_INFO** ppOutNow = ppOutNext;
        ppOutNext = NULL;

        switch (pInNow->sType)
        {
            CASE_XGL_STRUCTURE_TYPE_PIPELINE(IA_STATE_CREATE_INFO)
            CASE_XGL_STRUCTURE_TYPE_PIPELINE(TESS_STATE_CREATE_INFO)
            CASE_XGL_STRUCTURE_TYPE_PIPELINE(RS_STATE_CREATE_INFO)
            CASE_XGL_STRUCTURE_TYPE_PIPELINE(DS_STATE_CREATE_INFO)
            CASE_XGL_STRUCTURE_TYPE_PIPELINE(VP_STATE_CREATE_INFO)
            CASE_XGL_STRUCTURE_TYPE_PIPELINE(MS_STATE_CREATE_INFO)
            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            {
                XGL_PIPELINE_CB_STATE_CREATE_INFO *pPacket = NULL;
                XGL_PIPELINE_CB_STATE_CREATE_INFO *pIn = NULL;
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_CB_STATE_CREATE_INFO), pInNow);
                pPacket = (XGL_PIPELINE_CB_STATE_CREATE_INFO*) *ppOutNow;
                pIn = (XGL_PIPELINE_CB_STATE_CREATE_INFO*) pInNow;
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pAttachments, pIn->attachmentCount * sizeof(XGL_PIPELINE_CB_ATTACHMENT_STATE), pIn->pAttachments);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAttachments));
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            {
                XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pPacket = NULL;
                XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pInPacket = NULL;
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO), pInNow);
                pPacket = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) *ppOutNow;
                pInPacket = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) pInNow;
                add_pipeline_shader_to_trace_packet(pHeader, &pPacket->shader, &pInPacket->shader);
                finalize_pipeline_shader_address(pHeader, &pPacket->shader);
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
            {
                XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pPacket = NULL;
                XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pIn = NULL;
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO), pInNow);
                pPacket = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*) *ppOutNow;
                pIn = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*) pInNow;
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pVertexBindingDescriptions, pIn->bindingCount * sizeof(XGL_VERTEX_INPUT_BINDING_DESCRIPTION), pIn->pVertexBindingDescriptions);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexBindingDescriptions));
                glv_add_buffer_to_trace_packet(pHeader, (void **) &pPacket->pVertexAttributeDescriptions, pIn->attributeCount * sizeof(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION), pIn->pVertexAttributeDescriptions);
                glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pVertexAttributeDescriptions));
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            default:
                assert(!"Encountered an unexpected type in pipeline state list");
        }
        pInNow = (XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pInNow->pNext;
    }
    return;
}
