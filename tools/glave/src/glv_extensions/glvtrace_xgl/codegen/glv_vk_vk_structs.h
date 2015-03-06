/* THIS FILE IS GENERATED.  DO NOT EDIT. */

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
 */

#pragma once

#include "xgl.h"
#include "glv_trace_packet_utils.h"


//=============================================================================
static void add_XGL_APPLICATION_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_APPLICATION_INFO** ppStruct, const XGL_APPLICATION_INFO *pInStruct)
{
    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_APPLICATION_INFO), pInStruct);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pAppName), strlen(pInStruct->pAppName) + 1, pInStruct->pAppName);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pEngineName), strlen(pInStruct->pEngineName) + 1, pInStruct->pEngineName);
    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pAppName));
    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pEngineName));
    glv_finalize_buffer_address(pHeader, (void**)&*ppStruct);
};

//=============================================================================

static void add_XGL_DEVICE_CREATE_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_DEVICE_CREATE_INFO** ppStruct, const XGL_DEVICE_CREATE_INFO *pInStruct)
{
    uint32_t i;
    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_DEVICE_CREATE_INFO), pInStruct);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pRequestedQueues, pInStruct->queueRecordCount*sizeof(XGL_DEVICE_QUEUE_CREATE_INFO), pInStruct->pRequestedQueues);
    glv_finalize_buffer_address(pHeader, (void**)&(*ppStruct)->pRequestedQueues);
    if (pInStruct->extensionCount > 0) 
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledExtensionNames), pInStruct->extensionCount * sizeof(char *), pInStruct->ppEnabledExtensionNames);
        for (i = 0; i < pInStruct->extensionCount; i++)
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])), strlen(pInStruct->ppEnabledExtensionNames[i]) + 1, pInStruct->ppEnabledExtensionNames[i]);
            glv_finalize_buffer_address(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])));
        }
        glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames);
    }
    XGL_LAYER_CREATE_INFO *pNext = ( XGL_LAYER_CREATE_INFO *) pInStruct->pNext;
    while (pNext != NULL)
    {
        if ((pNext->sType == XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pNext->layerCount > 0)
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppStruct)->pNext)), sizeof(XGL_LAYER_CREATE_INFO), pNext);
            glv_finalize_buffer_address(pHeader, (void**)(&((*ppStruct)->pNext)));
            XGL_LAYER_CREATE_INFO **ppOutStruct = (XGL_LAYER_CREATE_INFO **) &((*ppStruct)->pNext);
            glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppOutStruct)->ppActiveLayerNames), pNext->layerCount * sizeof(char *), pNext->ppActiveLayerNames);
            for (i = 0; i < pNext->layerCount; i++)
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppOutStruct)->ppActiveLayerNames[i])), strlen(pNext->ppActiveLayerNames[i]) + 1, pNext->ppActiveLayerNames[i]);
                glv_finalize_buffer_address(pHeader, (void**)(&((*ppOutStruct)->ppActiveLayerNames[i])));
            }
            glv_finalize_buffer_address(pHeader, (void **)&(*ppOutStruct)->ppActiveLayerNames);
        }
        pNext = ( XGL_LAYER_CREATE_INFO *) pNext->pNext;
    }
    glv_finalize_buffer_address(pHeader, (void**)ppStruct);
}

static XGL_DEVICE_CREATE_INFO* interpret_XGL_DEVICE_CREATE_INFO(glv_trace_packet_header*  pHeader, intptr_t ptr_variable)
{
    XGL_DEVICE_CREATE_INFO* pXGL_DEVICE_CREATE_INFO = (XGL_DEVICE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)ptr_variable);

    if (pXGL_DEVICE_CREATE_INFO != NULL)
    {
            uint32_t i;
            const char** pNames;
        pXGL_DEVICE_CREATE_INFO->pRequestedQueues = (const XGL_DEVICE_QUEUE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->pRequestedQueues);

        if (pXGL_DEVICE_CREATE_INFO->extensionCount > 0)
        {
            pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames = (const char *const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames);
            pNames = (const char**)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames;
            for (i = 0; i < pXGL_DEVICE_CREATE_INFO->extensionCount; i++)
            {
                pNames[i] = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames[i]));
            }
        }
        XGL_LAYER_CREATE_INFO *pNext = ( XGL_LAYER_CREATE_INFO *) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->pNext);
        while (pNext != NULL)
        {
            if ((pNext->sType == XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pNext->layerCount > 0)
            {
                pNext->ppActiveLayerNames = (const char**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pNext->ppActiveLayerNames));
                pNames = (const char**)pNext->ppActiveLayerNames;
                for (i = 0; i < pNext->layerCount; i++)
                {
                    pNames[i] = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pNext->ppActiveLayerNames[i]));
                }
            }
            pNext = ( XGL_LAYER_CREATE_INFO *) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
        }
    }

    return pXGL_DEVICE_CREATE_INFO;
}

static void interpret_pipeline_shader(glv_trace_packet_header*  pHeader, XGL_PIPELINE_SHADER* pShader)
{
    if (pShader != NULL)
    {
        // constant buffers
        if (pShader->linkConstBufferCount > 0)
        {
            uint32_t i;
            pShader->pLinkConstBufferInfo = (const XGL_LINK_CONST_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo);
            for (i = 0; i < pShader->linkConstBufferCount; i++)
            {
                XGL_LINK_CONST_BUFFER* pBuffer = (XGL_LINK_CONST_BUFFER*)pShader->pLinkConstBufferInfo;
                pBuffer[i].pBufferData = (const void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo[i].pBufferData);
            }
        }
    }
}

//=============================================================================
typedef struct struct_xglApiVersion {
    glv_trace_packet_header* header;
    uint32_t version;
} struct_xglApiVersion;

static struct_xglApiVersion* interpret_body_as_xglApiVersion(glv_trace_packet_header* pHeader, BOOL check_version)
{
    struct_xglApiVersion* pPacket = (struct_xglApiVersion*)pHeader->pBody;
    pPacket->header = pHeader;
    if (check_version && pPacket->version != XGL_API_VERSION)
        glv_LogError("Trace file from older XGL version 0x%x, xgl replayer built from version 0x%x, replayer may fail\n", pPacket->version, XGL_API_VERSION);
    return pPacket;
}

typedef struct struct_xglCreateInstance {
    glv_trace_packet_header* header;
    const XGL_APPLICATION_INFO* pAppInfo;
    const XGL_ALLOC_CALLBACKS* pAllocCb;
    XGL_INSTANCE* pInstance;
    XGL_RESULT result;
} struct_xglCreateInstance;

static struct_xglCreateInstance* interpret_body_as_xglCreateInstance(glv_trace_packet_header* pHeader)
{
    struct_xglCreateInstance* pPacket = (struct_xglCreateInstance*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pAppInfo = (const XGL_APPLICATION_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo);
    if (pPacket->pAppInfo != NULL)
    {
        XGL_APPLICATION_INFO* pInfo = (XGL_APPLICATION_INFO*)pPacket->pAppInfo;
        pInfo->pAppName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pAppName);
        pInfo->pEngineName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pEngineName);
    }
    pPacket->pAllocCb = (const XGL_ALLOC_CALLBACKS*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocCb);
    pPacket->pInstance = (XGL_INSTANCE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pInstance);
    return pPacket;
}

typedef struct struct_xglDestroyInstance {
    glv_trace_packet_header* header;
    XGL_INSTANCE instance;
    XGL_RESULT result;
} struct_xglDestroyInstance;

static struct_xglDestroyInstance* interpret_body_as_xglDestroyInstance(glv_trace_packet_header* pHeader)
{
    struct_xglDestroyInstance* pPacket = (struct_xglDestroyInstance*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglEnumerateGpus {
    glv_trace_packet_header* header;
    XGL_INSTANCE instance;
    uint32_t maxGpus;
    uint32_t* pGpuCount;
    XGL_PHYSICAL_GPU* pGpus;
    XGL_RESULT result;
} struct_xglEnumerateGpus;

static struct_xglEnumerateGpus* interpret_body_as_xglEnumerateGpus(glv_trace_packet_header* pHeader)
{
    struct_xglEnumerateGpus* pPacket = (struct_xglEnumerateGpus*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pGpuCount = (uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pGpuCount);
    pPacket->pGpus = (XGL_PHYSICAL_GPU*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pGpus);
    return pPacket;
}

typedef struct struct_xglGetGpuInfo {
    glv_trace_packet_header* header;
    XGL_PHYSICAL_GPU gpu;
    XGL_PHYSICAL_GPU_INFO_TYPE infoType;
    size_t* pDataSize;
    void* pData;
    XGL_RESULT result;
} struct_xglGetGpuInfo;

static struct_xglGetGpuInfo* interpret_body_as_xglGetGpuInfo(glv_trace_packet_header* pHeader)
{
    struct_xglGetGpuInfo* pPacket = (struct_xglGetGpuInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (size_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglGetProcAddr {
    glv_trace_packet_header* header;
    XGL_PHYSICAL_GPU gpu;
    const char* pName;
    void* result;
} struct_xglGetProcAddr;

static struct_xglGetProcAddr* interpret_body_as_xglGetProcAddr(glv_trace_packet_header* pHeader)
{
    struct_xglGetProcAddr* pPacket = (struct_xglGetProcAddr*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pName);
    return pPacket;
}

typedef struct struct_xglCreateDevice {
    glv_trace_packet_header* header;
    XGL_PHYSICAL_GPU gpu;
    const XGL_DEVICE_CREATE_INFO* pCreateInfo;
    XGL_DEVICE* pDevice;
    XGL_RESULT result;
} struct_xglCreateDevice;

static struct_xglCreateDevice* interpret_body_as_xglCreateDevice(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDevice* pPacket = (struct_xglCreateDevice*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = interpret_XGL_DEVICE_CREATE_INFO(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pDevice = (XGL_DEVICE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDevice);
    return pPacket;
}

typedef struct struct_xglDestroyDevice {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_RESULT result;
} struct_xglDestroyDevice;

static struct_xglDestroyDevice* interpret_body_as_xglDestroyDevice(glv_trace_packet_header* pHeader)
{
    struct_xglDestroyDevice* pPacket = (struct_xglDestroyDevice*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglGetExtensionSupport {
    glv_trace_packet_header* header;
    XGL_PHYSICAL_GPU gpu;
    const char* pExtName;
    XGL_RESULT result;
} struct_xglGetExtensionSupport;

static struct_xglGetExtensionSupport* interpret_body_as_xglGetExtensionSupport(glv_trace_packet_header* pHeader)
{
    struct_xglGetExtensionSupport* pPacket = (struct_xglGetExtensionSupport*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pExtName = (const char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pExtName);
    return pPacket;
}

typedef struct struct_xglEnumerateLayers {
    glv_trace_packet_header* header;
    XGL_PHYSICAL_GPU gpu;
    size_t maxLayerCount;
    size_t maxStringSize;
    size_t* pOutLayerCount;
    char* const* pOutLayers;
    void* pReserved;
    XGL_RESULT result;
} struct_xglEnumerateLayers;

static struct_xglEnumerateLayers* interpret_body_as_xglEnumerateLayers(glv_trace_packet_header* pHeader)
{
    struct_xglEnumerateLayers* pPacket = (struct_xglEnumerateLayers*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOutLayerCount = (size_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOutLayerCount);
    pPacket->pOutLayers = (char* const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOutLayers);
    pPacket->pReserved = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pReserved);
    return pPacket;
}

typedef struct struct_xglGetDeviceQueue {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    uint32_t queueNodeIndex;
    uint32_t queueIndex;
    XGL_QUEUE* pQueue;
    XGL_RESULT result;
} struct_xglGetDeviceQueue;

static struct_xglGetDeviceQueue* interpret_body_as_xglGetDeviceQueue(glv_trace_packet_header* pHeader)
{
    struct_xglGetDeviceQueue* pPacket = (struct_xglGetDeviceQueue*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pQueue = (XGL_QUEUE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pQueue);
    return pPacket;
}

typedef struct struct_xglQueueSubmit {
    glv_trace_packet_header* header;
    XGL_QUEUE queue;
    uint32_t cmdBufferCount;
    const XGL_CMD_BUFFER* pCmdBuffers;
    uint32_t memRefCount;
    const XGL_MEMORY_REF* pMemRefs;
    XGL_FENCE fence;
    XGL_RESULT result;
} struct_xglQueueSubmit;

static struct_xglQueueSubmit* interpret_body_as_xglQueueSubmit(glv_trace_packet_header* pHeader)
{
    struct_xglQueueSubmit* pPacket = (struct_xglQueueSubmit*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCmdBuffers = (const XGL_CMD_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCmdBuffers);
    pPacket->pMemRefs = (const XGL_MEMORY_REF*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMemRefs);
    return pPacket;
}

typedef struct struct_xglQueueSetGlobalMemReferences {
    glv_trace_packet_header* header;
    XGL_QUEUE queue;
    uint32_t memRefCount;
    const XGL_MEMORY_REF* pMemRefs;
    XGL_RESULT result;
} struct_xglQueueSetGlobalMemReferences;

static struct_xglQueueSetGlobalMemReferences* interpret_body_as_xglQueueSetGlobalMemReferences(glv_trace_packet_header* pHeader)
{
    struct_xglQueueSetGlobalMemReferences* pPacket = (struct_xglQueueSetGlobalMemReferences*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pMemRefs = (const XGL_MEMORY_REF*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMemRefs);
    return pPacket;
}

typedef struct struct_xglQueueWaitIdle {
    glv_trace_packet_header* header;
    XGL_QUEUE queue;
    XGL_RESULT result;
} struct_xglQueueWaitIdle;

static struct_xglQueueWaitIdle* interpret_body_as_xglQueueWaitIdle(glv_trace_packet_header* pHeader)
{
    struct_xglQueueWaitIdle* pPacket = (struct_xglQueueWaitIdle*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglDeviceWaitIdle {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_RESULT result;
} struct_xglDeviceWaitIdle;

static struct_xglDeviceWaitIdle* interpret_body_as_xglDeviceWaitIdle(glv_trace_packet_header* pHeader)
{
    struct_xglDeviceWaitIdle* pPacket = (struct_xglDeviceWaitIdle*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglAllocMemory {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_MEMORY_ALLOC_INFO* pAllocInfo;
    XGL_GPU_MEMORY* pMem;
    XGL_RESULT result;
} struct_xglAllocMemory;

static struct_xglAllocMemory* interpret_body_as_xglAllocMemory(glv_trace_packet_header* pHeader)
{
    struct_xglAllocMemory* pPacket = (struct_xglAllocMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pAllocInfo = (const XGL_MEMORY_ALLOC_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocInfo);
    if (pPacket->pAllocInfo != NULL)
    {
        if (pPacket->pAllocInfo->sType == XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO) {
            XGL_MEMORY_ALLOC_INFO** ppNext = (XGL_MEMORY_ALLOC_INFO**) &(pPacket->pAllocInfo->pNext);
            *ppNext = (XGL_MEMORY_ALLOC_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocInfo->pNext);
            XGL_MEMORY_ALLOC_INFO* pNext = (XGL_MEMORY_ALLOC_INFO*) *ppNext;
            while (NULL != pNext)
            {
                switch(pNext->sType)
                {
                    case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
                    case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
                    {
                        ppNext = (XGL_MEMORY_ALLOC_INFO **) &(pNext->pNext);
                        *ppNext = (XGL_MEMORY_ALLOC_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                        break;
                    }
                    default:
                    {
                       glv_LogError("Encountered an unexpected type alloc memory list.\n");
                       pPacket->header = NULL;
                       pNext->pNext = NULL;
                    }
                }
                pNext = (XGL_MEMORY_ALLOC_INFO*)pNext->pNext;
            }
        } else {
            // This is unexpected.
            glv_LogError("AllocMemory must have AllocInfo stype of XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO.\n");
            pPacket->header = NULL;
        }
    }
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglFreeMemory {
    glv_trace_packet_header* header;
    XGL_GPU_MEMORY mem;
    XGL_RESULT result;
} struct_xglFreeMemory;

static struct_xglFreeMemory* interpret_body_as_xglFreeMemory(glv_trace_packet_header* pHeader)
{
    struct_xglFreeMemory* pPacket = (struct_xglFreeMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglSetMemoryPriority {
    glv_trace_packet_header* header;
    XGL_GPU_MEMORY mem;
    XGL_MEMORY_PRIORITY priority;
    XGL_RESULT result;
} struct_xglSetMemoryPriority;

static struct_xglSetMemoryPriority* interpret_body_as_xglSetMemoryPriority(glv_trace_packet_header* pHeader)
{
    struct_xglSetMemoryPriority* pPacket = (struct_xglSetMemoryPriority*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglMapMemory {
    glv_trace_packet_header* header;
    XGL_GPU_MEMORY mem;
    XGL_FLAGS flags;
    void** ppData;
    XGL_RESULT result;
} struct_xglMapMemory;

static struct_xglMapMemory* interpret_body_as_xglMapMemory(glv_trace_packet_header* pHeader)
{
    struct_xglMapMemory* pPacket = (struct_xglMapMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->ppData = (void**)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->ppData);
    return pPacket;
}

typedef struct struct_xglUnmapMemory {
    glv_trace_packet_header* header;
    XGL_GPU_MEMORY mem;
    void* pData;
    XGL_RESULT result;
} struct_xglUnmapMemory;

static struct_xglUnmapMemory* interpret_body_as_xglUnmapMemory(glv_trace_packet_header* pHeader)
{
    struct_xglUnmapMemory* pPacket = (struct_xglUnmapMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglPinSystemMemory {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const void* pSysMem;
    size_t memSize;
    XGL_GPU_MEMORY* pMem;
    XGL_RESULT result;
} struct_xglPinSystemMemory;

static struct_xglPinSystemMemory* interpret_body_as_xglPinSystemMemory(glv_trace_packet_header* pHeader)
{
    struct_xglPinSystemMemory* pPacket = (struct_xglPinSystemMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pSysMem = (const void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSysMem);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglGetMultiGpuCompatibility {
    glv_trace_packet_header* header;
    XGL_PHYSICAL_GPU gpu0;
    XGL_PHYSICAL_GPU gpu1;
    XGL_GPU_COMPATIBILITY_INFO* pInfo;
    XGL_RESULT result;
} struct_xglGetMultiGpuCompatibility;

static struct_xglGetMultiGpuCompatibility* interpret_body_as_xglGetMultiGpuCompatibility(glv_trace_packet_header* pHeader)
{
    struct_xglGetMultiGpuCompatibility* pPacket = (struct_xglGetMultiGpuCompatibility*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pInfo = (XGL_GPU_COMPATIBILITY_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pInfo);
    return pPacket;
}

typedef struct struct_xglOpenSharedMemory {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_MEMORY_OPEN_INFO* pOpenInfo;
    XGL_GPU_MEMORY* pMem;
    XGL_RESULT result;
} struct_xglOpenSharedMemory;

static struct_xglOpenSharedMemory* interpret_body_as_xglOpenSharedMemory(glv_trace_packet_header* pHeader)
{
    struct_xglOpenSharedMemory* pPacket = (struct_xglOpenSharedMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_MEMORY_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglOpenSharedQueueSemaphore {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo;
    XGL_QUEUE_SEMAPHORE* pSemaphore;
    XGL_RESULT result;
} struct_xglOpenSharedQueueSemaphore;

static struct_xglOpenSharedQueueSemaphore* interpret_body_as_xglOpenSharedQueueSemaphore(glv_trace_packet_header* pHeader)
{
    struct_xglOpenSharedQueueSemaphore* pPacket = (struct_xglOpenSharedQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_QUEUE_SEMAPHORE_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    pPacket->pSemaphore = (XGL_QUEUE_SEMAPHORE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSemaphore);
    return pPacket;
}

typedef struct struct_xglOpenPeerMemory {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo;
    XGL_GPU_MEMORY* pMem;
    XGL_RESULT result;
} struct_xglOpenPeerMemory;

static struct_xglOpenPeerMemory* interpret_body_as_xglOpenPeerMemory(glv_trace_packet_header* pHeader)
{
    struct_xglOpenPeerMemory* pPacket = (struct_xglOpenPeerMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_PEER_MEMORY_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglOpenPeerImage {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo;
    XGL_IMAGE* pImage;
    XGL_GPU_MEMORY* pMem;
    XGL_RESULT result;
} struct_xglOpenPeerImage;

static struct_xglOpenPeerImage* interpret_body_as_xglOpenPeerImage(glv_trace_packet_header* pHeader)
{
    struct_xglOpenPeerImage* pPacket = (struct_xglOpenPeerImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_PEER_IMAGE_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    pPacket->pImage = (XGL_IMAGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pImage);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglDestroyObject {
    glv_trace_packet_header* header;
    XGL_OBJECT object;
    XGL_RESULT result;
} struct_xglDestroyObject;

static struct_xglDestroyObject* interpret_body_as_xglDestroyObject(glv_trace_packet_header* pHeader)
{
    struct_xglDestroyObject* pPacket = (struct_xglDestroyObject*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglGetObjectInfo {
    glv_trace_packet_header* header;
    XGL_BASE_OBJECT object;
    XGL_OBJECT_INFO_TYPE infoType;
    size_t* pDataSize;
    void* pData;
    XGL_RESULT result;
} struct_xglGetObjectInfo;

static struct_xglGetObjectInfo* interpret_body_as_xglGetObjectInfo(glv_trace_packet_header* pHeader)
{
    struct_xglGetObjectInfo* pPacket = (struct_xglGetObjectInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (size_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglBindObjectMemory {
    glv_trace_packet_header* header;
    XGL_OBJECT object;
    uint32_t allocationIdx;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE offset;
    XGL_RESULT result;
} struct_xglBindObjectMemory;

static struct_xglBindObjectMemory* interpret_body_as_xglBindObjectMemory(glv_trace_packet_header* pHeader)
{
    struct_xglBindObjectMemory* pPacket = (struct_xglBindObjectMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglBindObjectMemoryRange {
    glv_trace_packet_header* header;
    XGL_OBJECT object;
    uint32_t allocationIdx;
    XGL_GPU_SIZE rangeOffset;
    XGL_GPU_SIZE rangeSize;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE memOffset;
    XGL_RESULT result;
} struct_xglBindObjectMemoryRange;

static struct_xglBindObjectMemoryRange* interpret_body_as_xglBindObjectMemoryRange(glv_trace_packet_header* pHeader)
{
    struct_xglBindObjectMemoryRange* pPacket = (struct_xglBindObjectMemoryRange*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglBindImageMemoryRange {
    glv_trace_packet_header* header;
    XGL_IMAGE image;
    uint32_t allocationIdx;
    const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE memOffset;
    XGL_RESULT result;
} struct_xglBindImageMemoryRange;

static struct_xglBindImageMemoryRange* interpret_body_as_xglBindImageMemoryRange(glv_trace_packet_header* pHeader)
{
    struct_xglBindImageMemoryRange* pPacket = (struct_xglBindImageMemoryRange*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->bindInfo = (const XGL_IMAGE_MEMORY_BIND_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->bindInfo);
    return pPacket;
}

typedef struct struct_xglCreateFence {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_FENCE_CREATE_INFO* pCreateInfo;
    XGL_FENCE* pFence;
    XGL_RESULT result;
} struct_xglCreateFence;

static struct_xglCreateFence* interpret_body_as_xglCreateFence(glv_trace_packet_header* pHeader)
{
    struct_xglCreateFence* pPacket = (struct_xglCreateFence*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_FENCE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pFence = (XGL_FENCE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pFence);
    return pPacket;
}

typedef struct struct_xglGetFenceStatus {
    glv_trace_packet_header* header;
    XGL_FENCE fence;
    XGL_RESULT result;
} struct_xglGetFenceStatus;

static struct_xglGetFenceStatus* interpret_body_as_xglGetFenceStatus(glv_trace_packet_header* pHeader)
{
    struct_xglGetFenceStatus* pPacket = (struct_xglGetFenceStatus*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglWaitForFences {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    uint32_t fenceCount;
    const XGL_FENCE* pFences;
    bool32_t waitAll;
    uint64_t timeout;
    XGL_RESULT result;
} struct_xglWaitForFences;

static struct_xglWaitForFences* interpret_body_as_xglWaitForFences(glv_trace_packet_header* pHeader)
{
    struct_xglWaitForFences* pPacket = (struct_xglWaitForFences*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pFences = (const XGL_FENCE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pFences);
    return pPacket;
}

typedef struct struct_xglCreateQueueSemaphore {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo;
    XGL_QUEUE_SEMAPHORE* pSemaphore;
    XGL_RESULT result;
} struct_xglCreateQueueSemaphore;

static struct_xglCreateQueueSemaphore* interpret_body_as_xglCreateQueueSemaphore(glv_trace_packet_header* pHeader)
{
    struct_xglCreateQueueSemaphore* pPacket = (struct_xglCreateQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_QUEUE_SEMAPHORE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pSemaphore = (XGL_QUEUE_SEMAPHORE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSemaphore);
    return pPacket;
}

typedef struct struct_xglSignalQueueSemaphore {
    glv_trace_packet_header* header;
    XGL_QUEUE queue;
    XGL_QUEUE_SEMAPHORE semaphore;
    XGL_RESULT result;
} struct_xglSignalQueueSemaphore;

static struct_xglSignalQueueSemaphore* interpret_body_as_xglSignalQueueSemaphore(glv_trace_packet_header* pHeader)
{
    struct_xglSignalQueueSemaphore* pPacket = (struct_xglSignalQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglWaitQueueSemaphore {
    glv_trace_packet_header* header;
    XGL_QUEUE queue;
    XGL_QUEUE_SEMAPHORE semaphore;
    XGL_RESULT result;
} struct_xglWaitQueueSemaphore;

static struct_xglWaitQueueSemaphore* interpret_body_as_xglWaitQueueSemaphore(glv_trace_packet_header* pHeader)
{
    struct_xglWaitQueueSemaphore* pPacket = (struct_xglWaitQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCreateEvent {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_EVENT_CREATE_INFO* pCreateInfo;
    XGL_EVENT* pEvent;
    XGL_RESULT result;
} struct_xglCreateEvent;

static struct_xglCreateEvent* interpret_body_as_xglCreateEvent(glv_trace_packet_header* pHeader)
{
    struct_xglCreateEvent* pPacket = (struct_xglCreateEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_EVENT_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pEvent = (XGL_EVENT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pEvent);
    return pPacket;
}

typedef struct struct_xglGetEventStatus {
    glv_trace_packet_header* header;
    XGL_EVENT event;
    XGL_RESULT result;
} struct_xglGetEventStatus;

static struct_xglGetEventStatus* interpret_body_as_xglGetEventStatus(glv_trace_packet_header* pHeader)
{
    struct_xglGetEventStatus* pPacket = (struct_xglGetEventStatus*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglSetEvent {
    glv_trace_packet_header* header;
    XGL_EVENT event;
    XGL_RESULT result;
} struct_xglSetEvent;

static struct_xglSetEvent* interpret_body_as_xglSetEvent(glv_trace_packet_header* pHeader)
{
    struct_xglSetEvent* pPacket = (struct_xglSetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglResetEvent {
    glv_trace_packet_header* header;
    XGL_EVENT event;
    XGL_RESULT result;
} struct_xglResetEvent;

static struct_xglResetEvent* interpret_body_as_xglResetEvent(glv_trace_packet_header* pHeader)
{
    struct_xglResetEvent* pPacket = (struct_xglResetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCreateQueryPool {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo;
    XGL_QUERY_POOL* pQueryPool;
    XGL_RESULT result;
} struct_xglCreateQueryPool;

static struct_xglCreateQueryPool* interpret_body_as_xglCreateQueryPool(glv_trace_packet_header* pHeader)
{
    struct_xglCreateQueryPool* pPacket = (struct_xglCreateQueryPool*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_QUERY_POOL_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pQueryPool = (XGL_QUERY_POOL*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pQueryPool);
    return pPacket;
}

typedef struct struct_xglGetQueryPoolResults {
    glv_trace_packet_header* header;
    XGL_QUERY_POOL queryPool;
    uint32_t startQuery;
    uint32_t queryCount;
    size_t* pDataSize;
    void* pData;
    XGL_RESULT result;
} struct_xglGetQueryPoolResults;

static struct_xglGetQueryPoolResults* interpret_body_as_xglGetQueryPoolResults(glv_trace_packet_header* pHeader)
{
    struct_xglGetQueryPoolResults* pPacket = (struct_xglGetQueryPoolResults*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (size_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglGetFormatInfo {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_FORMAT format;
    XGL_FORMAT_INFO_TYPE infoType;
    size_t* pDataSize;
    void* pData;
    XGL_RESULT result;
} struct_xglGetFormatInfo;

static struct_xglGetFormatInfo* interpret_body_as_xglGetFormatInfo(glv_trace_packet_header* pHeader)
{
    struct_xglGetFormatInfo* pPacket = (struct_xglGetFormatInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (size_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglCreateBuffer {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_BUFFER_CREATE_INFO* pCreateInfo;
    XGL_BUFFER* pBuffer;
    XGL_RESULT result;
} struct_xglCreateBuffer;

static struct_xglCreateBuffer* interpret_body_as_xglCreateBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCreateBuffer* pPacket = (struct_xglCreateBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_BUFFER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pBuffer = (XGL_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBuffer);
    return pPacket;
}

typedef struct struct_xglCreateBufferView {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo;
    XGL_BUFFER_VIEW* pView;
    XGL_RESULT result;
} struct_xglCreateBufferView;

static struct_xglCreateBufferView* interpret_body_as_xglCreateBufferView(glv_trace_packet_header* pHeader)
{
    struct_xglCreateBufferView* pPacket = (struct_xglCreateBufferView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_BUFFER_VIEW_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pView = (XGL_BUFFER_VIEW*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pView);
    return pPacket;
}

typedef struct struct_xglCreateImage {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_IMAGE_CREATE_INFO* pCreateInfo;
    XGL_IMAGE* pImage;
    XGL_RESULT result;
} struct_xglCreateImage;

static struct_xglCreateImage* interpret_body_as_xglCreateImage(glv_trace_packet_header* pHeader)
{
    struct_xglCreateImage* pPacket = (struct_xglCreateImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_IMAGE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pImage = (XGL_IMAGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pImage);
    return pPacket;
}

typedef struct struct_xglSetFastClearColor {
    glv_trace_packet_header* header;
    XGL_IMAGE image;
    const float color[4];
    XGL_RESULT result;
} struct_xglSetFastClearColor;

static struct_xglSetFastClearColor* interpret_body_as_xglSetFastClearColor(glv_trace_packet_header* pHeader)
{
    struct_xglSetFastClearColor* pPacket = (struct_xglSetFastClearColor*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglSetFastClearDepth {
    glv_trace_packet_header* header;
    XGL_IMAGE image;
    float depth;
    XGL_RESULT result;
} struct_xglSetFastClearDepth;

static struct_xglSetFastClearDepth* interpret_body_as_xglSetFastClearDepth(glv_trace_packet_header* pHeader)
{
    struct_xglSetFastClearDepth* pPacket = (struct_xglSetFastClearDepth*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglGetImageSubresourceInfo {
    glv_trace_packet_header* header;
    XGL_IMAGE image;
    const XGL_IMAGE_SUBRESOURCE* pSubresource;
    XGL_SUBRESOURCE_INFO_TYPE infoType;
    size_t* pDataSize;
    void* pData;
    XGL_RESULT result;
} struct_xglGetImageSubresourceInfo;

static struct_xglGetImageSubresourceInfo* interpret_body_as_xglGetImageSubresourceInfo(glv_trace_packet_header* pHeader)
{
    struct_xglGetImageSubresourceInfo* pPacket = (struct_xglGetImageSubresourceInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pSubresource = (const XGL_IMAGE_SUBRESOURCE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSubresource);
    pPacket->pDataSize = (size_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglCreateImageView {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo;
    XGL_IMAGE_VIEW* pView;
    XGL_RESULT result;
} struct_xglCreateImageView;

static struct_xglCreateImageView* interpret_body_as_xglCreateImageView(glv_trace_packet_header* pHeader)
{
    struct_xglCreateImageView* pPacket = (struct_xglCreateImageView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_IMAGE_VIEW_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pView = (XGL_IMAGE_VIEW*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pView);
    return pPacket;
}

typedef struct struct_xglCreateColorAttachmentView {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo;
    XGL_COLOR_ATTACHMENT_VIEW* pView;
    XGL_RESULT result;
} struct_xglCreateColorAttachmentView;

static struct_xglCreateColorAttachmentView* interpret_body_as_xglCreateColorAttachmentView(glv_trace_packet_header* pHeader)
{
    struct_xglCreateColorAttachmentView* pPacket = (struct_xglCreateColorAttachmentView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pView = (XGL_COLOR_ATTACHMENT_VIEW*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pView);
    return pPacket;
}

typedef struct struct_xglCreateDepthStencilView {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo;
    XGL_DEPTH_STENCIL_VIEW* pView;
    XGL_RESULT result;
} struct_xglCreateDepthStencilView;

static struct_xglCreateDepthStencilView* interpret_body_as_xglCreateDepthStencilView(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDepthStencilView* pPacket = (struct_xglCreateDepthStencilView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pView = (XGL_DEPTH_STENCIL_VIEW*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pView);
    return pPacket;
}

typedef struct struct_xglCreateShader {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_SHADER_CREATE_INFO* pCreateInfo;
    XGL_SHADER* pShader;
    XGL_RESULT result;
} struct_xglCreateShader;

static struct_xglCreateShader* interpret_body_as_xglCreateShader(glv_trace_packet_header* pHeader)
{
    struct_xglCreateShader* pPacket = (struct_xglCreateShader*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_SHADER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    if (pPacket->pCreateInfo != NULL)
    {
        XGL_SHADER_CREATE_INFO* pInfo = (XGL_SHADER_CREATE_INFO*)pPacket->pCreateInfo;
        pInfo->pCode = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pCode);
    }
    pPacket->pShader = (XGL_SHADER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pShader);
    return pPacket;
}

typedef struct struct_xglCreateGraphicsPipeline {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo;
    XGL_PIPELINE* pPipeline;
    XGL_RESULT result;
} struct_xglCreateGraphicsPipeline;

static struct_xglCreateGraphicsPipeline* interpret_body_as_xglCreateGraphicsPipeline(glv_trace_packet_header* pHeader)
{
    struct_xglCreateGraphicsPipeline* pPacket = (struct_xglCreateGraphicsPipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_GRAPHICS_PIPELINE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    if (pPacket->pCreateInfo != NULL)
    {
        if (pPacket->pCreateInfo->sType == XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {
            // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one
            void** ppNextVoidPtr = (void**)&pPacket->pCreateInfo->pNext;
            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pNext);
            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;
            while ((NULL != pNext) && (XGL_NULL_HANDLE != pNext))
        {
                switch(pNext->sType)
            {
                    case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
                    case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
                    case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
                    case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
                    case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
                    case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
                    {
                        void** ppNextVoidPtr = (void**)&pNext->pNext;
                        *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
                    {
                        void** ppNextVoidPtr = (void**)&pNext->pNext;
                        XGL_PIPELINE_CB_STATE_CREATE_INFO *pCb = (XGL_PIPELINE_CB_STATE_CREATE_INFO *) pNext;
                        *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                        pCb->pAttachments = (XGL_PIPELINE_CB_ATTACHMENT_STATE*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pCb->pAttachments);
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
                    {
                        void** ppNextVoidPtr = (void**)&pNext->pNext;
                        *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                        interpret_pipeline_shader(pHeader, &pNext->shader);
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
                    {
                        void** ppNextVoidPtr = (void**)&pNext->pNext;
                        XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pVi = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *) pNext;
                        *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                        pVi->pVertexBindingDescriptions = (XGL_VERTEX_INPUT_BINDING_DESCRIPTION*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexBindingDescriptions);
                        pVi->pVertexAttributeDescriptions = (XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pVi->pVertexAttributeDescriptions);
                        break;
                    }
                    default:
                    {
                       glv_LogError("Encountered an unexpected type in pipeline state list.\n");
                       pPacket->header = NULL;
                       pNext->pNext = NULL;
                    }
                }
                pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;
            }
        } else {
            // This is unexpected.
            glv_LogError("CreateGraphicsPipeline must have CreateInfo stype of XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO.\n");
            pPacket->header = NULL;
        }
    }
    pPacket->pPipeline = (XGL_PIPELINE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPipeline);
    return pPacket;
}

typedef struct struct_xglCreateComputePipeline {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo;
    XGL_PIPELINE* pPipeline;
    XGL_RESULT result;
} struct_xglCreateComputePipeline;

static struct_xglCreateComputePipeline* interpret_body_as_xglCreateComputePipeline(glv_trace_packet_header* pHeader)
{
    struct_xglCreateComputePipeline* pPacket = (struct_xglCreateComputePipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_COMPUTE_PIPELINE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    if (pPacket->pCreateInfo != NULL)
    {
        interpret_pipeline_shader(pHeader, (XGL_PIPELINE_SHADER*)(&pPacket->pCreateInfo->cs));
    }
    pPacket->pPipeline = (XGL_PIPELINE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPipeline);
    return pPacket;
}

typedef struct struct_xglStorePipeline {
    glv_trace_packet_header* header;
    XGL_PIPELINE pipeline;
    size_t* pDataSize;
    void* pData;
    XGL_RESULT result;
} struct_xglStorePipeline;

static struct_xglStorePipeline* interpret_body_as_xglStorePipeline(glv_trace_packet_header* pHeader)
{
    struct_xglStorePipeline* pPacket = (struct_xglStorePipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (size_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglLoadPipeline {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    size_t dataSize;
    const void* pData;
    XGL_PIPELINE* pPipeline;
    XGL_RESULT result;
} struct_xglLoadPipeline;

static struct_xglLoadPipeline* interpret_body_as_xglLoadPipeline(glv_trace_packet_header* pHeader)
{
    struct_xglLoadPipeline* pPacket = (struct_xglLoadPipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (const void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    pPacket->pPipeline = (XGL_PIPELINE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPipeline);
    return pPacket;
}

typedef struct struct_xglCreatePipelineDelta {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_PIPELINE p1;
    XGL_PIPELINE p2;
    XGL_PIPELINE_DELTA* delta;
    XGL_RESULT result;
} struct_xglCreatePipelineDelta;

static struct_xglCreatePipelineDelta* interpret_body_as_xglCreatePipelineDelta(glv_trace_packet_header* pHeader)
{
    struct_xglCreatePipelineDelta* pPacket = (struct_xglCreatePipelineDelta*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->delta = (XGL_PIPELINE_DELTA*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->delta);
    return pPacket;
}

typedef struct struct_xglCreateSampler {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_SAMPLER_CREATE_INFO* pCreateInfo;
    XGL_SAMPLER* pSampler;
    XGL_RESULT result;
} struct_xglCreateSampler;

static struct_xglCreateSampler* interpret_body_as_xglCreateSampler(glv_trace_packet_header* pHeader)
{
    struct_xglCreateSampler* pPacket = (struct_xglCreateSampler*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_SAMPLER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pSampler = (XGL_SAMPLER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSampler);
    return pPacket;
}

typedef struct struct_xglCreateDescriptorSetLayout {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_FLAGS stageFlags;
    const uint32_t* pSetBindPoints;
    XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout;
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList;
    XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout;
    XGL_RESULT result;
} struct_xglCreateDescriptorSetLayout;

static struct_xglCreateDescriptorSetLayout* interpret_body_as_xglCreateDescriptorSetLayout(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDescriptorSetLayout* pPacket = (struct_xglCreateDescriptorSetLayout*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pSetBindPoints = (const uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetBindPoints);
    pPacket->pSetLayoutInfoList = (const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetLayoutInfoList);
    if (pPacket->pSetLayoutInfoList != NULL)
    {
        if (pPacket->pSetLayoutInfoList->sType == XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {
            // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one
            void** ppNextVoidPtr = (void**)&(pPacket->pSetLayoutInfoList->pNext);
            *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetLayoutInfoList->pNext);
            XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pPacket->pSetLayoutInfoList->pNext;
            while (NULL != pNext)
            {
                switch(pNext->sType)
                {
                    case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
                    {
                        void** ppNextVoidPtr = (void**)&pNext->pNext;
                        *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                        break;
                    }
                    default:
                    {
                        glv_LogError("Encountered an unexpected type in descriptor set layout create list.\n");
                        pPacket->header = NULL;
                        pNext->pNext = NULL;
                    }
                }
                pNext = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pNext->pNext;
             }
        } else {
             // This is unexpected.
             glv_LogError("CreateDescriptorSetLayout must have LayoutInfoList stype of XGL_STRCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO\n");
             pPacket->header = NULL;
        }
    }
    pPacket->pSetLayout = (XGL_DESCRIPTOR_SET_LAYOUT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetLayout);
    return pPacket;
}

typedef struct struct_xglBeginDescriptorRegionUpdate {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_DESCRIPTOR_UPDATE_MODE updateMode;
    XGL_RESULT result;
} struct_xglBeginDescriptorRegionUpdate;

static struct_xglBeginDescriptorRegionUpdate* interpret_body_as_xglBeginDescriptorRegionUpdate(glv_trace_packet_header* pHeader)
{
    struct_xglBeginDescriptorRegionUpdate* pPacket = (struct_xglBeginDescriptorRegionUpdate*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglEndDescriptorRegionUpdate {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_CMD_BUFFER cmd;
    XGL_RESULT result;
} struct_xglEndDescriptorRegionUpdate;

static struct_xglEndDescriptorRegionUpdate* interpret_body_as_xglEndDescriptorRegionUpdate(glv_trace_packet_header* pHeader)
{
    struct_xglEndDescriptorRegionUpdate* pPacket = (struct_xglEndDescriptorRegionUpdate*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCreateDescriptorRegion {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    XGL_DESCRIPTOR_REGION_USAGE regionUsage;
    uint32_t maxSets;
    const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo;
    XGL_DESCRIPTOR_REGION* pDescriptorRegion;
    XGL_RESULT result;
} struct_xglCreateDescriptorRegion;

static struct_xglCreateDescriptorRegion* interpret_body_as_xglCreateDescriptorRegion(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDescriptorRegion* pPacket = (struct_xglCreateDescriptorRegion*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DESCRIPTOR_REGION_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    if (pPacket->pCreateInfo != NULL)
    {
        XGL_DESCRIPTOR_REGION_CREATE_INFO* pInfo = (XGL_DESCRIPTOR_REGION_CREATE_INFO*)pPacket->pCreateInfo;
        pInfo->pTypeCount = (XGL_DESCRIPTOR_TYPE_COUNT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pTypeCount);

    }
    pPacket->pDescriptorRegion = (XGL_DESCRIPTOR_REGION*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDescriptorRegion);
    return pPacket;
}

typedef struct struct_xglClearDescriptorRegion {
    glv_trace_packet_header* header;
    XGL_DESCRIPTOR_REGION descriptorRegion;
    XGL_RESULT result;
} struct_xglClearDescriptorRegion;

static struct_xglClearDescriptorRegion* interpret_body_as_xglClearDescriptorRegion(glv_trace_packet_header* pHeader)
{
    struct_xglClearDescriptorRegion* pPacket = (struct_xglClearDescriptorRegion*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglAllocDescriptorSets {
    glv_trace_packet_header* header;
    XGL_DESCRIPTOR_REGION descriptorRegion;
    XGL_DESCRIPTOR_SET_USAGE setUsage;
    uint32_t count;
    const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts;
    XGL_DESCRIPTOR_SET* pDescriptorSets;
    uint32_t* pCount;
    XGL_RESULT result;
} struct_xglAllocDescriptorSets;

static struct_xglAllocDescriptorSets* interpret_body_as_xglAllocDescriptorSets(glv_trace_packet_header* pHeader)
{
    struct_xglAllocDescriptorSets* pPacket = (struct_xglAllocDescriptorSets*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pSetLayouts = (const XGL_DESCRIPTOR_SET_LAYOUT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSetLayouts);
    pPacket->pDescriptorSets = (XGL_DESCRIPTOR_SET*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDescriptorSets);
    pPacket->pCount = (uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCount);
    return pPacket;
}

typedef struct struct_xglClearDescriptorSets {
    glv_trace_packet_header* header;
    XGL_DESCRIPTOR_REGION descriptorRegion;
    uint32_t count;
    const XGL_DESCRIPTOR_SET* pDescriptorSets;
} struct_xglClearDescriptorSets;

static struct_xglClearDescriptorSets* interpret_body_as_xglClearDescriptorSets(glv_trace_packet_header* pHeader)
{
    struct_xglClearDescriptorSets* pPacket = (struct_xglClearDescriptorSets*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDescriptorSets = (const XGL_DESCRIPTOR_SET*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDescriptorSets);
    return pPacket;
}

typedef struct struct_xglUpdateDescriptors {
    glv_trace_packet_header* header;
    XGL_DESCRIPTOR_SET descriptorSet;
    const void* pUpdateChain;
} struct_xglUpdateDescriptors;

static struct_xglUpdateDescriptors* interpret_body_as_xglUpdateDescriptors(glv_trace_packet_header* pHeader)
{
    struct_xglUpdateDescriptors* pPacket = (struct_xglUpdateDescriptors*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pUpdateChain = (const void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pUpdateChain);
    if (pPacket->pUpdateChain != NULL)
    {
        XGL_UPDATE_SAMPLERS* pNext = (XGL_UPDATE_SAMPLERS*)pPacket->pUpdateChain;
        while ((NULL != pNext) && (XGL_NULL_HANDLE != pNext))
        {
            switch(pNext->sType)
            {
                case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
                {
                    void** ppNextVoidPtr = (void**)&pNext->pNext;
                    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                    break;
                }
                case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                {
                    void** ppNextVoidPtr = (void**)&pNext->pNext;
                    XGL_UPDATE_SAMPLERS* pUS = (XGL_UPDATE_SAMPLERS*)pNext;
                    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                    pUS->pSamplers = (XGL_SAMPLER*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUS->pSamplers);
                    break;
                }
                case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                {
                    void** ppNextVoidPtr = (void**)&pNext->pNext;
                    XGL_UPDATE_SAMPLER_TEXTURES* pUST = (XGL_UPDATE_SAMPLER_TEXTURES*)pNext;
                    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                    pUST->pSamplerImageViews = (XGL_SAMPLER_IMAGE_VIEW_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUST->pSamplerImageViews);
                    uint32_t i;
                    for (i = 0; i < pUST->count; i++) {
                        XGL_IMAGE_VIEW_ATTACH_INFO** ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO**)&pUST->pSamplerImageViews[i].pImageView;
                        *ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUST->pSamplerImageViews[i].pImageView);
                    }
                    break;
                }
                case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
                {
                    void** ppNextVoidPtr = (void**)&pNext->pNext;
                    XGL_UPDATE_IMAGES* pUI = (XGL_UPDATE_IMAGES*)pNext;
                    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                    XGL_IMAGE_VIEW_ATTACH_INFO** ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO**)&pUI->pImageViews;
                    *ppLocalImageView = (XGL_IMAGE_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUI->pImageViews);
                    uint32_t i;
                    for (i = 0; i < pUI->count; i++) {
                        XGL_IMAGE_VIEW_ATTACH_INFO** ppLocalImageViews = (XGL_IMAGE_VIEW_ATTACH_INFO**)&pUI->pImageViews[i];
                        *ppLocalImageViews = (XGL_IMAGE_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUI->pImageViews[i]);
                    }
                    break;
                }
                case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
                {
                    void** ppNextVoidPtr = (void**)&pNext->pNext;
                    XGL_UPDATE_BUFFERS* pUB = (XGL_UPDATE_BUFFERS*)pNext;
                    *ppNextVoidPtr = (void*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                    XGL_BUFFER_VIEW_ATTACH_INFO** ppLocalBufferView = (XGL_BUFFER_VIEW_ATTACH_INFO**)&pUB->pBufferViews;
                    *ppLocalBufferView = (XGL_BUFFER_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUB->pBufferViews);
                    uint32_t i;
                    for (i = 0; i < pUB->count; i++) {
                        XGL_BUFFER_VIEW_ATTACH_INFO** ppLocalBufferViews = (XGL_BUFFER_VIEW_ATTACH_INFO**)&pUB->pBufferViews[i];
                        *ppLocalBufferViews = (XGL_BUFFER_VIEW_ATTACH_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pUB->pBufferViews[i]);
                    }
                    break;
                }
                default:
                {
                   glv_LogError("Encountered an unexpected type in update descriptors pUpdateChain.\n");
                   pPacket->header = NULL;
                   pNext->pNext = NULL;
                }
            }
            pNext = (XGL_UPDATE_SAMPLERS*)pNext->pNext;
        }
    }
    return pPacket;
}

typedef struct struct_xglCreateDynamicViewportState {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo;
    XGL_DYNAMIC_VP_STATE_OBJECT* pState;
    XGL_RESULT result;
} struct_xglCreateDynamicViewportState;

static struct_xglCreateDynamicViewportState* interpret_body_as_xglCreateDynamicViewportState(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDynamicViewportState* pPacket = (struct_xglCreateDynamicViewportState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DYNAMIC_VP_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    if (pPacket->pCreateInfo != NULL)
    {
        XGL_DYNAMIC_VP_STATE_CREATE_INFO* pInfo = (XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pPacket->pCreateInfo;
        pInfo->pViewports = (XGL_VIEWPORT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pViewports);
        pInfo->pScissors = (XGL_RECT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pScissors);
    }
    pPacket->pState = (XGL_DYNAMIC_VP_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateDynamicRasterState {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo;
    XGL_DYNAMIC_RS_STATE_OBJECT* pState;
    XGL_RESULT result;
} struct_xglCreateDynamicRasterState;

static struct_xglCreateDynamicRasterState* interpret_body_as_xglCreateDynamicRasterState(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDynamicRasterState* pPacket = (struct_xglCreateDynamicRasterState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DYNAMIC_RS_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_DYNAMIC_RS_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateDynamicColorBlendState {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo;
    XGL_DYNAMIC_CB_STATE_OBJECT* pState;
    XGL_RESULT result;
} struct_xglCreateDynamicColorBlendState;

static struct_xglCreateDynamicColorBlendState* interpret_body_as_xglCreateDynamicColorBlendState(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDynamicColorBlendState* pPacket = (struct_xglCreateDynamicColorBlendState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DYNAMIC_CB_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_DYNAMIC_CB_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateDynamicDepthStencilState {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo;
    XGL_DYNAMIC_DS_STATE_OBJECT* pState;
    XGL_RESULT result;
} struct_xglCreateDynamicDepthStencilState;

static struct_xglCreateDynamicDepthStencilState* interpret_body_as_xglCreateDynamicDepthStencilState(glv_trace_packet_header* pHeader)
{
    struct_xglCreateDynamicDepthStencilState* pPacket = (struct_xglCreateDynamicDepthStencilState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DYNAMIC_DS_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_DYNAMIC_DS_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateCommandBuffer {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo;
    XGL_CMD_BUFFER* pCmdBuffer;
    XGL_RESULT result;
} struct_xglCreateCommandBuffer;

static struct_xglCreateCommandBuffer* interpret_body_as_xglCreateCommandBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCreateCommandBuffer* pPacket = (struct_xglCreateCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_CMD_BUFFER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pCmdBuffer = (XGL_CMD_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCmdBuffer);
    return pPacket;
}

typedef struct struct_xglBeginCommandBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo;
    XGL_RESULT result;
} struct_xglBeginCommandBuffer;

static struct_xglBeginCommandBuffer* interpret_body_as_xglBeginCommandBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglBeginCommandBuffer* pPacket = (struct_xglBeginCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pBeginInfo = (const XGL_CMD_BUFFER_BEGIN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBeginInfo);
    if (pPacket->pBeginInfo != NULL)
    {
        if (pPacket->pBeginInfo->sType == XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO) {
            // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one
            XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO** ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO**)&(pPacket->pBeginInfo->pNext);
            *ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBeginInfo->pNext);
            XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pNext = *ppNext;
            while (NULL != pNext)
            {
                switch(pNext->sType)
                {
                    case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
                    {
                        ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO**) &pNext->pNext;
                        *ppNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                        break;
                    }
                    default:
                    {
                        glv_LogError("Encountered an unexpected type in begin command buffer list.\n");
                        pPacket->header = NULL;
                        pNext->pNext = NULL;
                    }
                }
                pNext = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pNext->pNext;
            }
        } else {
            // This is unexpected.
            glv_LogError("BeginCommandBuffer must have BeginInfo stype of XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO.\n");
            pPacket->header = NULL;
        }
    }
    return pPacket;
}

typedef struct struct_xglEndCommandBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_RESULT result;
} struct_xglEndCommandBuffer;

static struct_xglEndCommandBuffer* interpret_body_as_xglEndCommandBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglEndCommandBuffer* pPacket = (struct_xglEndCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglResetCommandBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_RESULT result;
} struct_xglResetCommandBuffer;

static struct_xglResetCommandBuffer* interpret_body_as_xglResetCommandBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglResetCommandBuffer* pPacket = (struct_xglResetCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindPipeline {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT pipelineBindPoint;
    XGL_PIPELINE pipeline;
} struct_xglCmdBindPipeline;

static struct_xglCmdBindPipeline* interpret_body_as_xglCmdBindPipeline(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBindPipeline* pPacket = (struct_xglCmdBindPipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindPipelineDelta {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT pipelineBindPoint;
    XGL_PIPELINE_DELTA delta;
} struct_xglCmdBindPipelineDelta;

static struct_xglCmdBindPipelineDelta* interpret_body_as_xglCmdBindPipelineDelta(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBindPipelineDelta* pPacket = (struct_xglCmdBindPipelineDelta*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindDynamicStateObject {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_STATE_BIND_POINT stateBindPoint;
    XGL_DYNAMIC_STATE_OBJECT state;
} struct_xglCmdBindDynamicStateObject;

static struct_xglCmdBindDynamicStateObject* interpret_body_as_xglCmdBindDynamicStateObject(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBindDynamicStateObject* pPacket = (struct_xglCmdBindDynamicStateObject*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindDescriptorSet {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT pipelineBindPoint;
    XGL_DESCRIPTOR_SET descriptorSet;
    const uint32_t* pUserData;
} struct_xglCmdBindDescriptorSet;

static struct_xglCmdBindDescriptorSet* interpret_body_as_xglCmdBindDescriptorSet(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBindDescriptorSet* pPacket = (struct_xglCmdBindDescriptorSet*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pUserData = (const uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pUserData);
    return pPacket;
}

typedef struct struct_xglCmdBindVertexBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER buffer;
    XGL_GPU_SIZE offset;
    uint32_t binding;
} struct_xglCmdBindVertexBuffer;

static struct_xglCmdBindVertexBuffer* interpret_body_as_xglCmdBindVertexBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBindVertexBuffer* pPacket = (struct_xglCmdBindVertexBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindIndexBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER buffer;
    XGL_GPU_SIZE offset;
    XGL_INDEX_TYPE indexType;
} struct_xglCmdBindIndexBuffer;

static struct_xglCmdBindIndexBuffer* interpret_body_as_xglCmdBindIndexBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBindIndexBuffer* pPacket = (struct_xglCmdBindIndexBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDraw {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    uint32_t firstVertex;
    uint32_t vertexCount;
    uint32_t firstInstance;
    uint32_t instanceCount;
} struct_xglCmdDraw;

static struct_xglCmdDraw* interpret_body_as_xglCmdDraw(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDraw* pPacket = (struct_xglCmdDraw*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDrawIndexed {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t vertexOffset;
    uint32_t firstInstance;
    uint32_t instanceCount;
} struct_xglCmdDrawIndexed;

static struct_xglCmdDrawIndexed* interpret_body_as_xglCmdDrawIndexed(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDrawIndexed* pPacket = (struct_xglCmdDrawIndexed*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDrawIndirect {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER buffer;
    XGL_GPU_SIZE offset;
    uint32_t count;
    uint32_t stride;
} struct_xglCmdDrawIndirect;

static struct_xglCmdDrawIndirect* interpret_body_as_xglCmdDrawIndirect(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDrawIndirect* pPacket = (struct_xglCmdDrawIndirect*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDrawIndexedIndirect {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER buffer;
    XGL_GPU_SIZE offset;
    uint32_t count;
    uint32_t stride;
} struct_xglCmdDrawIndexedIndirect;

static struct_xglCmdDrawIndexedIndirect* interpret_body_as_xglCmdDrawIndexedIndirect(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDrawIndexedIndirect* pPacket = (struct_xglCmdDrawIndexedIndirect*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDispatch {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    uint32_t x;
    uint32_t y;
    uint32_t z;
} struct_xglCmdDispatch;

static struct_xglCmdDispatch* interpret_body_as_xglCmdDispatch(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDispatch* pPacket = (struct_xglCmdDispatch*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDispatchIndirect {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER buffer;
    XGL_GPU_SIZE offset;
} struct_xglCmdDispatchIndirect;

static struct_xglCmdDispatchIndirect* interpret_body_as_xglCmdDispatchIndirect(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDispatchIndirect* pPacket = (struct_xglCmdDispatchIndirect*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdCopyBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER srcBuffer;
    XGL_BUFFER destBuffer;
    uint32_t regionCount;
    const XGL_BUFFER_COPY* pRegions;
} struct_xglCmdCopyBuffer;

static struct_xglCmdCopyBuffer* interpret_body_as_xglCmdCopyBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCmdCopyBuffer* pPacket = (struct_xglCmdCopyBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_BUFFER_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCopyImage {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_IMAGE srcImage;
    XGL_IMAGE destImage;
    uint32_t regionCount;
    const XGL_IMAGE_COPY* pRegions;
} struct_xglCmdCopyImage;

static struct_xglCmdCopyImage* interpret_body_as_xglCmdCopyImage(glv_trace_packet_header* pHeader)
{
    struct_xglCmdCopyImage* pPacket = (struct_xglCmdCopyImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_IMAGE_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCopyBufferToImage {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER srcBuffer;
    XGL_IMAGE destImage;
    uint32_t regionCount;
    const XGL_BUFFER_IMAGE_COPY* pRegions;
} struct_xglCmdCopyBufferToImage;

static struct_xglCmdCopyBufferToImage* interpret_body_as_xglCmdCopyBufferToImage(glv_trace_packet_header* pHeader)
{
    struct_xglCmdCopyBufferToImage* pPacket = (struct_xglCmdCopyBufferToImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_BUFFER_IMAGE_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCopyImageToBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_IMAGE srcImage;
    XGL_BUFFER destBuffer;
    uint32_t regionCount;
    const XGL_BUFFER_IMAGE_COPY* pRegions;
} struct_xglCmdCopyImageToBuffer;

static struct_xglCmdCopyImageToBuffer* interpret_body_as_xglCmdCopyImageToBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCmdCopyImageToBuffer* pPacket = (struct_xglCmdCopyImageToBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_BUFFER_IMAGE_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCloneImageData {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_IMAGE srcImage;
    XGL_IMAGE_LAYOUT srcImageLayout;
    XGL_IMAGE destImage;
    XGL_IMAGE_LAYOUT destImageLayout;
} struct_xglCmdCloneImageData;

static struct_xglCmdCloneImageData* interpret_body_as_xglCmdCloneImageData(glv_trace_packet_header* pHeader)
{
    struct_xglCmdCloneImageData* pPacket = (struct_xglCmdCloneImageData*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdUpdateBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER destBuffer;
    XGL_GPU_SIZE destOffset;
    XGL_GPU_SIZE dataSize;
    const uint32_t* pData;
} struct_xglCmdUpdateBuffer;

static struct_xglCmdUpdateBuffer* interpret_body_as_xglCmdUpdateBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCmdUpdateBuffer* pPacket = (struct_xglCmdUpdateBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (const uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglCmdFillBuffer {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_BUFFER destBuffer;
    XGL_GPU_SIZE destOffset;
    XGL_GPU_SIZE fillSize;
    uint32_t data;
} struct_xglCmdFillBuffer;

static struct_xglCmdFillBuffer* interpret_body_as_xglCmdFillBuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCmdFillBuffer* pPacket = (struct_xglCmdFillBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdClearColorImage {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_IMAGE image;
    const float color[4];
    uint32_t rangeCount;
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges;
} struct_xglCmdClearColorImage;

static struct_xglCmdClearColorImage* interpret_body_as_xglCmdClearColorImage(glv_trace_packet_header* pHeader)
{
    struct_xglCmdClearColorImage* pPacket = (struct_xglCmdClearColorImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRanges = (const XGL_IMAGE_SUBRESOURCE_RANGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRanges);
    return pPacket;
}

typedef struct struct_xglCmdClearColorImageRaw {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_IMAGE image;
    const uint32_t color[4];
    uint32_t rangeCount;
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges;
} struct_xglCmdClearColorImageRaw;

static struct_xglCmdClearColorImageRaw* interpret_body_as_xglCmdClearColorImageRaw(glv_trace_packet_header* pHeader)
{
    struct_xglCmdClearColorImageRaw* pPacket = (struct_xglCmdClearColorImageRaw*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRanges = (const XGL_IMAGE_SUBRESOURCE_RANGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRanges);
    return pPacket;
}

typedef struct struct_xglCmdClearDepthStencil {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_IMAGE image;
    float depth;
    uint32_t stencil;
    uint32_t rangeCount;
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges;
} struct_xglCmdClearDepthStencil;

static struct_xglCmdClearDepthStencil* interpret_body_as_xglCmdClearDepthStencil(glv_trace_packet_header* pHeader)
{
    struct_xglCmdClearDepthStencil* pPacket = (struct_xglCmdClearDepthStencil*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRanges = (const XGL_IMAGE_SUBRESOURCE_RANGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRanges);
    return pPacket;
}

typedef struct struct_xglCmdResolveImage {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_IMAGE srcImage;
    XGL_IMAGE destImage;
    uint32_t rectCount;
    const XGL_IMAGE_RESOLVE* pRects;
} struct_xglCmdResolveImage;

static struct_xglCmdResolveImage* interpret_body_as_xglCmdResolveImage(glv_trace_packet_header* pHeader)
{
    struct_xglCmdResolveImage* pPacket = (struct_xglCmdResolveImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRects = (const XGL_IMAGE_RESOLVE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRects);
    return pPacket;
}

typedef struct struct_xglCmdSetEvent {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_EVENT event;
    XGL_SET_EVENT pipeEvent;
} struct_xglCmdSetEvent;

static struct_xglCmdSetEvent* interpret_body_as_xglCmdSetEvent(glv_trace_packet_header* pHeader)
{
    struct_xglCmdSetEvent* pPacket = (struct_xglCmdSetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdResetEvent {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_EVENT event;
} struct_xglCmdResetEvent;

static struct_xglCmdResetEvent* interpret_body_as_xglCmdResetEvent(glv_trace_packet_header* pHeader)
{
    struct_xglCmdResetEvent* pPacket = (struct_xglCmdResetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdWaitEvents {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    const XGL_EVENT_WAIT_INFO* pWaitInfo;
} struct_xglCmdWaitEvents;

static struct_xglCmdWaitEvents* interpret_body_as_xglCmdWaitEvents(glv_trace_packet_header* pHeader)
{
    struct_xglCmdWaitEvents* pPacket = (struct_xglCmdWaitEvents*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pWaitInfo = (const XGL_EVENT_WAIT_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pWaitInfo);
    if (pPacket->pWaitInfo != NULL)
    {
        XGL_EVENT_WAIT_INFO* pInfo = (XGL_EVENT_WAIT_INFO*)pPacket->pWaitInfo;
        pInfo->pEvents = (XGL_EVENT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pWaitInfo->pEvents);
        pInfo->ppMemBarriers = (const void**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pWaitInfo->ppMemBarriers);
        uint32_t i;
        for (i = 0; i < pInfo->memBarrierCount; i++) {
            void** ppLocalMemBarriers = (void**)&pInfo->ppMemBarriers[i];
            *ppLocalMemBarriers = (void*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pInfo->ppMemBarriers[i]);
        }
    }
    return pPacket;
}

typedef struct struct_xglCmdPipelineBarrier {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    const XGL_PIPELINE_BARRIER* pBarrier;
} struct_xglCmdPipelineBarrier;

static struct_xglCmdPipelineBarrier* interpret_body_as_xglCmdPipelineBarrier(glv_trace_packet_header* pHeader)
{
    struct_xglCmdPipelineBarrier* pPacket = (struct_xglCmdPipelineBarrier*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pBarrier = (const XGL_PIPELINE_BARRIER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBarrier);
    if (pPacket->pBarrier != NULL)
    {
        XGL_PIPELINE_BARRIER* pBarrier = (XGL_PIPELINE_BARRIER*)pPacket->pBarrier;
        pBarrier->pEvents = (XGL_SET_EVENT*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBarrier->pEvents);
        pBarrier->ppMemBarriers = (const void**) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pBarrier->ppMemBarriers);
        uint32_t i;
        for (i = 0; i < pBarrier->memBarrierCount; i++) {
            void** ppLocalMemBarriers = (void**)&pBarrier->ppMemBarriers[i];
            *ppLocalMemBarriers = (void*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pBarrier->ppMemBarriers[i]);
        }
    }
    return pPacket;
}

typedef struct struct_xglCmdBeginQuery {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_QUERY_POOL queryPool;
    uint32_t slot;
    XGL_FLAGS flags;
} struct_xglCmdBeginQuery;

static struct_xglCmdBeginQuery* interpret_body_as_xglCmdBeginQuery(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBeginQuery* pPacket = (struct_xglCmdBeginQuery*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdEndQuery {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_QUERY_POOL queryPool;
    uint32_t slot;
} struct_xglCmdEndQuery;

static struct_xglCmdEndQuery* interpret_body_as_xglCmdEndQuery(glv_trace_packet_header* pHeader)
{
    struct_xglCmdEndQuery* pPacket = (struct_xglCmdEndQuery*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdResetQueryPool {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_QUERY_POOL queryPool;
    uint32_t startQuery;
    uint32_t queryCount;
} struct_xglCmdResetQueryPool;

static struct_xglCmdResetQueryPool* interpret_body_as_xglCmdResetQueryPool(glv_trace_packet_header* pHeader)
{
    struct_xglCmdResetQueryPool* pPacket = (struct_xglCmdResetQueryPool*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdWriteTimestamp {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_TIMESTAMP_TYPE timestampType;
    XGL_BUFFER destBuffer;
    XGL_GPU_SIZE destOffset;
} struct_xglCmdWriteTimestamp;

static struct_xglCmdWriteTimestamp* interpret_body_as_xglCmdWriteTimestamp(glv_trace_packet_header* pHeader)
{
    struct_xglCmdWriteTimestamp* pPacket = (struct_xglCmdWriteTimestamp*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdInitAtomicCounters {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT pipelineBindPoint;
    uint32_t startCounter;
    uint32_t counterCount;
    const uint32_t* pData;
} struct_xglCmdInitAtomicCounters;

static struct_xglCmdInitAtomicCounters* interpret_body_as_xglCmdInitAtomicCounters(glv_trace_packet_header* pHeader)
{
    struct_xglCmdInitAtomicCounters* pPacket = (struct_xglCmdInitAtomicCounters*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (const uint32_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglCmdLoadAtomicCounters {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT pipelineBindPoint;
    uint32_t startCounter;
    uint32_t counterCount;
    XGL_BUFFER srcBuffer;
    XGL_GPU_SIZE srcOffset;
} struct_xglCmdLoadAtomicCounters;

static struct_xglCmdLoadAtomicCounters* interpret_body_as_xglCmdLoadAtomicCounters(glv_trace_packet_header* pHeader)
{
    struct_xglCmdLoadAtomicCounters* pPacket = (struct_xglCmdLoadAtomicCounters*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdSaveAtomicCounters {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT pipelineBindPoint;
    uint32_t startCounter;
    uint32_t counterCount;
    XGL_BUFFER destBuffer;
    XGL_GPU_SIZE destOffset;
} struct_xglCmdSaveAtomicCounters;

static struct_xglCmdSaveAtomicCounters* interpret_body_as_xglCmdSaveAtomicCounters(glv_trace_packet_header* pHeader)
{
    struct_xglCmdSaveAtomicCounters* pPacket = (struct_xglCmdSaveAtomicCounters*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCreateFramebuffer {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo;
    XGL_FRAMEBUFFER* pFramebuffer;
    XGL_RESULT result;
} struct_xglCreateFramebuffer;

static struct_xglCreateFramebuffer* interpret_body_as_xglCreateFramebuffer(glv_trace_packet_header* pHeader)
{
    struct_xglCreateFramebuffer* pPacket = (struct_xglCreateFramebuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_FRAMEBUFFER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    if (pPacket->pCreateInfo != NULL)
    {
        XGL_FRAMEBUFFER_CREATE_INFO* pInfo = (XGL_FRAMEBUFFER_CREATE_INFO*)pPacket->pCreateInfo;
        pInfo->pColorAttachments = (XGL_COLOR_ATTACHMENT_BIND_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorAttachments);
        pInfo->pDepthStencilAttachment = (XGL_DEPTH_STENCIL_BIND_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pDepthStencilAttachment);

    }
    pPacket->pFramebuffer = (XGL_FRAMEBUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pFramebuffer);
    return pPacket;
}

typedef struct struct_xglCreateRenderPass {
    glv_trace_packet_header* header;
    XGL_DEVICE device;
    const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo;
    XGL_RENDER_PASS* pRenderPass;
    XGL_RESULT result;
} struct_xglCreateRenderPass;

static struct_xglCreateRenderPass* interpret_body_as_xglCreateRenderPass(glv_trace_packet_header* pHeader)
{
    struct_xglCreateRenderPass* pPacket = (struct_xglCreateRenderPass*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_RENDER_PASS_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    if (pPacket->pCreateInfo != NULL)
    {
        XGL_RENDER_PASS_CREATE_INFO* pInfo = (XGL_RENDER_PASS_CREATE_INFO*)pPacket->pCreateInfo;
        pInfo->pColorLoadOps = (XGL_ATTACHMENT_LOAD_OP*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorLoadOps);
        pInfo->pColorStoreOps = (XGL_ATTACHMENT_STORE_OP*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorStoreOps);
        pInfo->pColorLoadClearValues = (XGL_CLEAR_COLOR*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pColorLoadClearValues);

    }
    pPacket->pRenderPass = (XGL_RENDER_PASS*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRenderPass);
    return pPacket;
}

typedef struct struct_xglCmdBeginRenderPass {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_RENDER_PASS renderPass;
} struct_xglCmdBeginRenderPass;

static struct_xglCmdBeginRenderPass* interpret_body_as_xglCmdBeginRenderPass(glv_trace_packet_header* pHeader)
{
    struct_xglCmdBeginRenderPass* pPacket = (struct_xglCmdBeginRenderPass*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdEndRenderPass {
    glv_trace_packet_header* header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_RENDER_PASS renderPass;
} struct_xglCmdEndRenderPass;

static struct_xglCmdEndRenderPass* interpret_body_as_xglCmdEndRenderPass(glv_trace_packet_header* pHeader)
{
    struct_xglCmdEndRenderPass* pPacket = (struct_xglCmdEndRenderPass*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

