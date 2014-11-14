/**************************************************************************
 *
 * Copyright 2014 Valve Software
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#pragma once

#include "xgl.h"
#include "glv_trace_packet_utils.h"

static const char* string_XGL_RESULT(XGL_RESULT result)
{
    switch ((XGL_RESULT)result)
    {
    // Return codes for successful operation execution
    case XGL_SUCCESS:
        return "XGL_SUCCESS";
    case XGL_UNSUPPORTED:
        return "XGL_UNSUPPORTED";
    case XGL_NOT_READY:
        return "XGL_NOT_READY";
    case XGL_TIMEOUT:
        return "XGL_TIMEOUT";
    case XGL_EVENT_SET:
        return "XGL_EVENT_SET";
    case XGL_EVENT_RESET:
        return "XGL_EVENT_RESET";
    // Error codes
    case XGL_ERROR_UNKNOWN:
        return "XGL_ERROR_UNKNOWN";
    case XGL_ERROR_UNAVAILABLE:
        return "XGL_ERROR_UNAVAILABLE";
    case XGL_ERROR_INITIALIZATION_FAILED:
        return "XGL_ERROR_INITIALIZATION_FAILED";
    case XGL_ERROR_OUT_OF_MEMORY:
        return "XGL_ERROR_OUT_OF_MEMORY";
    case XGL_ERROR_OUT_OF_GPU_MEMORY:
        return "XGL_ERROR_OUT_OF_GPU_MEMORY";
    case XGL_ERROR_DEVICE_ALREADY_CREATED:
        return "XGL_ERROR_DEVICE_ALREADY_CREATED";
    case XGL_ERROR_DEVICE_LOST:
        return "XGL_ERROR_DEVICE_LOST";
    case XGL_ERROR_INVALID_POINTER:
        return "XGL_ERROR_INVALID_POINTER";
    case XGL_ERROR_INVALID_VALUE:
        return "XGL_ERROR_INVALID_VALUE";
    case XGL_ERROR_INVALID_HANDLE:
        return "XGL_ERROR_INVALID_HANDLE";
    case XGL_ERROR_INVALID_ORDINAL:
        return "XGL_ERROR_INVALID_ORDINAL";
    case XGL_ERROR_INVALID_MEMORY_SIZE:
        return "XGL_ERROR_INVALID_MEMORY_SIZE";
    case XGL_ERROR_INVALID_EXTENSION:
        return "XGL_ERROR_INVALID_EXTENSION";
    case XGL_ERROR_INVALID_FLAGS:
        return "XGL_ERROR_INVALID_FLAGS";
    case XGL_ERROR_INVALID_ALIGNMENT:
        return "XGL_ERROR_INVALID_ALIGNMENT";
    case XGL_ERROR_INVALID_FORMAT:
        return "XGL_ERROR_INVALID_FORMAT";
    case XGL_ERROR_INVALID_IMAGE:
        return "XGL_ERROR_INVALID_IMAGE";
    case XGL_ERROR_INVALID_DESCRIPTOR_SET_DATA:
        return "XGL_ERROR_INVALID_DESCRIPTOR_SET_DATA";
    case XGL_ERROR_INVALID_QUEUE_TYPE:
        return "XGL_ERROR_INVALID_QUEUE_TYPE";
    case XGL_ERROR_INVALID_OBJECT_TYPE:
        return "XGL_ERROR_INVALID_OBJECT_TYPE";
    case XGL_ERROR_UNSUPPORTED_SHADER_IL_VERSION:
        return "XGL_ERROR_UNSUPPORTED_SHADER_IL_VERSION";
    case XGL_ERROR_BAD_SHADER_CODE:
        return "XGL_ERROR_BAD_SHADER_CODE";
    case XGL_ERROR_BAD_PIPELINE_DATA:
        return "XGL_ERROR_BAD_PIPELINE_DATA";
    case XGL_ERROR_TOO_MANY_MEMORY_REFERENCES:
        return "XGL_ERROR_TOO_MANY_MEMORY_REFERENCES";
    case XGL_ERROR_NOT_MAPPABLE:
        return "XGL_ERROR_NOT_MAPPABLE";
    case XGL_ERROR_MEMORY_MAP_FAILED:
        return "XGL_ERROR_MEMORY_MAP_FAILED";
    case XGL_ERROR_MEMORY_UNMAP_FAILED:
        return "XGL_ERROR_MEMORY_UNMAP_FAILED";
    case XGL_ERROR_INCOMPATIBLE_DEVICE:
        return "XGL_ERROR_INCOMPATIBLE_DEVICE";
    case XGL_ERROR_INCOMPATIBLE_DRIVER:
        return "XGL_ERROR_INCOMPATIBLE_DRIVER";
    case XGL_ERROR_INCOMPLETE_COMMAND_BUFFER:
        return "XGL_ERROR_INCOMPLETE_COMMAND_BUFFER";
    case XGL_ERROR_BUILDING_COMMAND_BUFFER:
        return "XGL_ERROR_BUILDING_COMMAND_BUFFER";
    case XGL_ERROR_MEMORY_NOT_BOUND:
        return "XGL_ERROR_MEMORY_NOT_BOUND";
    case XGL_ERROR_INCOMPATIBLE_QUEUE:
        return "XGL_ERROR_INCOMPATIBLE_QUEUE";
    case XGL_ERROR_NOT_SHAREABLE:
        return "XGL_ERROR_NOT_SHAREABLE";
    }
    return "Unhandled XGL_RESULT";
}

//=============================================================================
static uint64_t calc_size_XGL_APPLICATION_INFO(const XGL_APPLICATION_INFO* pStruct)
{
    return ((pStruct == NULL) ? 0 : sizeof(XGL_APPLICATION_INFO)) + strlen((const char *)pStruct->pAppName) + 1 + strlen((const char *)pStruct->pEngineName) + 1;
}

static void add_XGL_APPLICATION_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_APPLICATION_INFO** ppStruct, const XGL_APPLICATION_INFO *pInStruct)
{
    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_APPLICATION_INFO), pInStruct);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pAppName), strlen((const char *)pInStruct->pAppName) + 1, (const char *)pInStruct->pAppName);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&((*ppStruct)->pEngineName), strlen((const char *)pInStruct->pEngineName) + 1, (const char *)pInStruct->pEngineName);
    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pAppName));
    glv_finalize_buffer_address(pHeader, (void**)&((*ppStruct)->pEngineName));
    glv_finalize_buffer_address(pHeader, (void**)&*ppStruct);
};

//=============================================================================

static uint64_t calc_size_XGL_DEVICE_CREATE_INFO(const XGL_DEVICE_CREATE_INFO* pStruct)
{
    uint64_t total_size_ppEnabledExtensionNames = pStruct->extensionCount * sizeof(XGL_CHAR *);
    uint32_t i;
    for (i = 0; i < pStruct->extensionCount; i++)
    {
        total_size_ppEnabledExtensionNames += strlen((const char *)pStruct->ppEnabledExtensionNames[i]) + 1;
    }

    return sizeof(XGL_DEVICE_CREATE_INFO) + (pStruct->queueRecordCount*sizeof(XGL_DEVICE_CREATE_INFO)) + total_size_ppEnabledExtensionNames;
}

static void add_XGL_DEVICE_CREATE_INFO_to_packet(glv_trace_packet_header*  pHeader, XGL_DEVICE_CREATE_INFO** ppStruct, const XGL_DEVICE_CREATE_INFO *pInStruct)
{
    uint32_t i;
    glv_add_buffer_to_trace_packet(pHeader, (void**)ppStruct, sizeof(XGL_DEVICE_CREATE_INFO), pInStruct);

    glv_add_buffer_to_trace_packet(pHeader, (void**)&(*ppStruct)->pRequestedQueues, pInStruct->queueRecordCount*sizeof(XGL_DEVICE_CREATE_INFO), pInStruct->pRequestedQueues);
    glv_finalize_buffer_address(pHeader, (void**)&(*ppStruct)->pRequestedQueues);

    if (pInStruct->extensionCount > 0) 
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)(&(*ppStruct)->ppEnabledExtensionNames), pInStruct->extensionCount * sizeof(XGL_CHAR *), pInStruct->ppEnabledExtensionNames);
        for (i = 0; i < pInStruct->extensionCount; i++)
        {

            glv_add_buffer_to_trace_packet(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])), strlen((const char *)pInStruct->ppEnabledExtensionNames[i]) + 1, (const char *)pInStruct->ppEnabledExtensionNames[i]);
            glv_finalize_buffer_address(pHeader, (void**)(&((*ppStruct)->ppEnabledExtensionNames[i])));
        }
        glv_finalize_buffer_address(pHeader, (void **)&(*ppStruct)->ppEnabledExtensionNames);
    }
    glv_finalize_buffer_address(pHeader, (void**)ppStruct);
}

static XGL_DEVICE_CREATE_INFO* intepret_XGL_DEVICE_CREATE_INFO(glv_trace_packet_header*  pHeader, intptr_t ptr_variable)
{
    XGL_DEVICE_CREATE_INFO* pXGL_DEVICE_CREATE_INFO = (XGL_DEVICE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)ptr_variable);

    if (pXGL_DEVICE_CREATE_INFO != NULL)
    {
        pXGL_DEVICE_CREATE_INFO->pRequestedQueues = (const XGL_DEVICE_QUEUE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->pRequestedQueues);

        if (pXGL_DEVICE_CREATE_INFO->extensionCount > 0)
        {
            const XGL_CHAR** pNames;
            uint32_t i;
            pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames = (const XGL_CHAR *const*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames);
            pNames = (const XGL_CHAR**)pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames;
            for (i = 0; i < pXGL_DEVICE_CREATE_INFO->extensionCount; i++)
            {
                pNames[i] = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)(pXGL_DEVICE_CREATE_INFO->ppEnabledExtensionNames[i]));
            }
        }
    }

    return pXGL_DEVICE_CREATE_INFO;
}

//=============================================================================
// entrypoints

typedef struct struct_xglInitAndEnumerateGpus {
    glv_trace_packet_header*    header;
    const XGL_APPLICATION_INFO* pAppInfo;
    const XGL_ALLOC_CALLBACKS*  pAllocCb;
    XGL_UINT                    maxGpus;
    XGL_UINT*                   pGpuCount;
    XGL_PHYSICAL_GPU*           pGpus;
    XGL_RESULT                  result;
} struct_xglInitAndEnumerateGpus;

static struct_xglInitAndEnumerateGpus* interpret_body_as_xglInitAndEnumerateGpus(glv_trace_packet_header*  pHeader)
{
    struct_xglInitAndEnumerateGpus* pPacket = (struct_xglInitAndEnumerateGpus*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pAppInfo = (const XGL_APPLICATION_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo);
    if (pPacket->pAppInfo != NULL)
    {
        XGL_APPLICATION_INFO* pInfo = (XGL_APPLICATION_INFO*)pPacket->pAppInfo;
        pInfo->pAppName = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pAppName);
        pInfo->pEngineName = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAppInfo->pEngineName);
    }
    pPacket->pAllocCb = (const XGL_ALLOC_CALLBACKS*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocCb);
    pPacket->pGpuCount = (XGL_UINT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pGpuCount);
    pPacket->pGpus = (XGL_PHYSICAL_GPU*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pGpus);
    return pPacket;
}

typedef struct struct_xglGetGpuInfo {
    glv_trace_packet_header*           header;
    XGL_PHYSICAL_GPU                   gpu;
    XGL_PHYSICAL_GPU_INFO_TYPE         infoType;
    XGL_SIZE*                          pDataSize;
    XGL_VOID*                          pData;
    XGL_RESULT                         result;
} struct_xglGetGpuInfo;

static struct_xglGetGpuInfo* interpret_body_as_xglGetGpuInfo(glv_trace_packet_header*  pHeader)
{
    struct_xglGetGpuInfo* pPacket = (struct_xglGetGpuInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (XGL_SIZE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

// Device functions

typedef struct struct_xglCreateDevice {
    glv_trace_packet_header*      header;
    XGL_PHYSICAL_GPU              gpu;
    const XGL_DEVICE_CREATE_INFO* pCreateInfo;
    XGL_DEVICE*                   pDevice;
    XGL_RESULT                    result;
} struct_xglCreateDevice;

static struct_xglCreateDevice* interpret_body_as_xglCreateDevice(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateDevice* pPacket = (struct_xglCreateDevice*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = intepret_XGL_DEVICE_CREATE_INFO(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pDevice = (XGL_DEVICE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDevice);
    return pPacket;
}

typedef struct struct_xglDestroyDevice {
    glv_trace_packet_header*  header;
    XGL_DEVICE                device;
    XGL_RESULT                result;
} struct_xglDestroyDevice;

static struct_xglDestroyDevice* interpret_body_as_xglDestroyDevice(glv_trace_packet_header*  pHeader)
{
    struct_xglDestroyDevice* pPacket = (struct_xglDestroyDevice*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

// Extension discovery functions

typedef struct struct_xglGetExtensionSupport {
    glv_trace_packet_header*  header;
    XGL_PHYSICAL_GPU          gpu;
    const XGL_CHAR*           pExtName;
    XGL_RESULT                result;
} struct_xglGetExtensionSupport;

static struct_xglGetExtensionSupport* interpret_body_as_xglGetExtensionSupport(glv_trace_packet_header*  pHeader)
{
    struct_xglGetExtensionSupport* pPacket = (struct_xglGetExtensionSupport*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pExtName = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pExtName);
    return pPacket;
}

// Queue functions

typedef struct struct_xglGetDeviceQueue {
    glv_trace_packet_header*  header;
    XGL_DEVICE                device;
    XGL_QUEUE_TYPE            queueType;
    XGL_UINT                  queueIndex;
    XGL_QUEUE*                pQueue;
    XGL_RESULT                result;
} struct_xglGetDeviceQueue;

static struct_xglGetDeviceQueue* interpret_body_as_xglGetDeviceQueue(glv_trace_packet_header*  pHeader)
{
    struct_xglGetDeviceQueue* pPacket = (struct_xglGetDeviceQueue*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pQueue = (XGL_QUEUE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pQueue);
    return pPacket;
}

typedef struct struct_xglQueueSubmit {
    glv_trace_packet_header* header;
    XGL_QUEUE                queue;
    XGL_UINT                 cmdBufferCount;
    const XGL_CMD_BUFFER*    pCmdBuffers;
    XGL_UINT                 memRefCount;
    const XGL_MEMORY_REF*    pMemRefs;
    XGL_FENCE                fence;
    XGL_RESULT               result;
} struct_xglQueueSubmit;

static struct_xglQueueSubmit* interpret_body_as_xglQueueSubmit(glv_trace_packet_header*  pHeader)
{
    struct_xglQueueSubmit* pPacket = (struct_xglQueueSubmit*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCmdBuffers = (const XGL_CMD_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCmdBuffers);
    pPacket->pMemRefs = (const XGL_MEMORY_REF*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMemRefs);
    return pPacket;
}

typedef struct struct_xglQueueSetGlobalMemReferences {
    glv_trace_packet_header*  header;
    XGL_QUEUE                 queue;
    XGL_UINT                  memRefCount;
    const XGL_MEMORY_REF*     pMemRefs;
    XGL_RESULT                result;
} struct_xglQueueSetGlobalMemReferences;

static struct_xglQueueSetGlobalMemReferences* interpret_body_as_xglQueueSetGlobalMemReferences(glv_trace_packet_header*  pHeader)
{
    struct_xglQueueSetGlobalMemReferences* pPacket = (struct_xglQueueSetGlobalMemReferences*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pMemRefs = (const XGL_MEMORY_REF*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMemRefs);
    return pPacket;
}

typedef struct struct_xglQueueWaitIdle {
    glv_trace_packet_header*  header;
    XGL_QUEUE queue;
    XGL_RESULT result;
} struct_xglQueueWaitIdle;

static struct_xglQueueWaitIdle* interpret_body_as_xglQueueWaitIdle(glv_trace_packet_header*  pHeader)
{
    struct_xglQueueWaitIdle* pPacket = (struct_xglQueueWaitIdle*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglDeviceWaitIdle {
    glv_trace_packet_header*  header;
    XGL_DEVICE device;
    XGL_RESULT result;
} struct_xglDeviceWaitIdle;

static struct_xglDeviceWaitIdle* interpret_body_as_xglDeviceWaitIdle(glv_trace_packet_header*  pHeader)
{
    struct_xglDeviceWaitIdle* pPacket = (struct_xglDeviceWaitIdle*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

// Memory functions

typedef struct struct_xglGetMemoryHeapCount {
    glv_trace_packet_header*  header;
    XGL_DEVICE  device;
    XGL_UINT*   pCount;
    XGL_RESULT result;
} struct_xglGetMemoryHeapCount;

static struct_xglGetMemoryHeapCount* interpret_body_as_xglGetMemoryHeapCount(glv_trace_packet_header*  pHeader)
{
    struct_xglGetMemoryHeapCount* pPacket = (struct_xglGetMemoryHeapCount*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCount = (XGL_UINT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCount);
    return pPacket;
}

typedef struct struct_xglGetMemoryHeapInfo {
    glv_trace_packet_header*  header;
    XGL_DEVICE device;
    XGL_UINT   heapId;
    XGL_MEMORY_HEAP_INFO_TYPE   infoType;
    XGL_SIZE*  pDataSize;
    XGL_VOID*  pData;
    XGL_RESULT result;
} struct_xglGetMemoryHeapInfo;

static struct_xglGetMemoryHeapInfo* interpret_body_as_xglGetMemoryHeapInfo(glv_trace_packet_header*  pHeader)
{
    struct_xglGetMemoryHeapInfo* pPacket = (struct_xglGetMemoryHeapInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (XGL_SIZE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglAllocMemory {
    glv_trace_packet_header*  header;
    XGL_DEVICE                   device;
    const XGL_MEMORY_ALLOC_INFO* pAllocInfo;
    XGL_GPU_MEMORY*              pMem;
    XGL_RESULT result;
} struct_xglAllocMemory;

static struct_xglAllocMemory* interpret_body_as_xglAllocMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglAllocMemory* pPacket = (struct_xglAllocMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pAllocInfo = (const XGL_MEMORY_ALLOC_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pAllocInfo);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglFreeMemory {
    glv_trace_packet_header*  header;
    XGL_GPU_MEMORY mem;
    XGL_RESULT result;
} struct_xglFreeMemory;

static struct_xglFreeMemory* interpret_body_as_xglFreeMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglFreeMemory* pPacket = (struct_xglFreeMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglSetMemoryPriority {
    glv_trace_packet_header*  header;
    XGL_GPU_MEMORY mem;
    XGL_MEMORY_PRIORITY       priority;
    XGL_RESULT result;
} struct_xglSetMemoryPriority;

static struct_xglSetMemoryPriority* interpret_body_as_xglSetMemoryPriority(glv_trace_packet_header*  pHeader)
{
    struct_xglSetMemoryPriority* pPacket = (struct_xglSetMemoryPriority*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglMapMemory {
    glv_trace_packet_header*  header;
    XGL_GPU_MEMORY mem;
    XGL_FLAGS      flags;                // Reserved
    XGL_VOID**     ppData;
    XGL_RESULT result;
} struct_xglMapMemory;

static struct_xglMapMemory* interpret_body_as_xglMapMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglMapMemory* pPacket = (struct_xglMapMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->ppData = (XGL_VOID**)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->ppData);
    return pPacket;
}

typedef struct struct_xglUnmapMemory {
    glv_trace_packet_header*  header;
    XGL_GPU_MEMORY mem;
    XGL_VOID *pData;  //Data CPU potentially wrote into
    XGL_RESULT result;
} struct_xglUnmapMemory;

static struct_xglUnmapMemory* interpret_body_as_xglUnmapMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglUnmapMemory* pPacket = (struct_xglUnmapMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (XGL_VOID *)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglPinSystemMemory {
    glv_trace_packet_header*  header;
    XGL_DEVICE      device;
    const XGL_VOID* pSysMem;
    XGL_SIZE        memSize;
    XGL_GPU_MEMORY* pMem;
    XGL_RESULT result;
} struct_xglPinSystemMemory;

static struct_xglPinSystemMemory* interpret_body_as_xglPinSystemMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglPinSystemMemory* pPacket = (struct_xglPinSystemMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pSysMem = (const XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSysMem);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglRemapVirtualMemoryPages {
    glv_trace_packet_header*  header;
    XGL_DEVICE                            device;
    XGL_UINT                              rangeCount;
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges;
    XGL_UINT                              preWaitSemaphoreCount;
    const XGL_QUEUE_SEMAPHORE*            pPreWaitSemaphores;
    XGL_UINT                              postSignalSemaphoreCount;
    const XGL_QUEUE_SEMAPHORE*            pPostSignalSemaphores;
    XGL_RESULT result;
} struct_xglRemapVirtualMemoryPages;

static struct_xglRemapVirtualMemoryPages* interpret_body_as_xglRemapVirtualMemoryPages(glv_trace_packet_header*  pHeader)
{
    struct_xglRemapVirtualMemoryPages* pPacket = (struct_xglRemapVirtualMemoryPages*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRanges = (const XGL_VIRTUAL_MEMORY_REMAP_RANGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRanges);
    pPacket->pPreWaitSemaphores = (const XGL_QUEUE_SEMAPHORE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPreWaitSemaphores);
    pPacket->pPostSignalSemaphores = (const XGL_QUEUE_SEMAPHORE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPostSignalSemaphores);
    return pPacket;
}

// Multi-device functions

typedef struct struct_xglGetMultiGpuCompatibility {
    glv_trace_packet_header*  header;
    XGL_PHYSICAL_GPU            gpu0;
    XGL_PHYSICAL_GPU            gpu1;
    XGL_GPU_COMPATIBILITY_INFO* pInfo;
    XGL_RESULT result;
} struct_xglGetMultiGpuCompatibility;

static struct_xglGetMultiGpuCompatibility* interpret_body_as_xglGetMultiGpuCompatibility(glv_trace_packet_header*  pHeader)
{
    struct_xglGetMultiGpuCompatibility* pPacket = (struct_xglGetMultiGpuCompatibility*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pInfo = (XGL_GPU_COMPATIBILITY_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pInfo);
    return pPacket;
}

typedef struct struct_xglOpenSharedMemory {
    glv_trace_packet_header*  header;
    XGL_DEVICE                  device;
    const XGL_MEMORY_OPEN_INFO* pOpenInfo;
    XGL_GPU_MEMORY*             pMem;
    XGL_RESULT result;
} struct_xglOpenSharedMemory;

static struct_xglOpenSharedMemory* interpret_body_as_xglOpenSharedMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglOpenSharedMemory* pPacket = (struct_xglOpenSharedMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_MEMORY_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglOpenSharedQueueSemaphore {
    glv_trace_packet_header*  header;
    XGL_DEVICE                           device;
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo;
    XGL_QUEUE_SEMAPHORE*                 pSemaphore;
    XGL_RESULT result;
} struct_xglOpenSharedQueueSemaphore;

static struct_xglOpenSharedQueueSemaphore* interpret_body_as_xglOpenSharedQueueSemaphore(glv_trace_packet_header*  pHeader)
{
    struct_xglOpenSharedQueueSemaphore* pPacket = (struct_xglOpenSharedQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_QUEUE_SEMAPHORE_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    pPacket->pSemaphore = (XGL_QUEUE_SEMAPHORE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSemaphore);
    return pPacket;
}

typedef struct struct_xglOpenPeerMemory {
    glv_trace_packet_header*  header;
    XGL_DEVICE                       device;
    const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo;
    XGL_GPU_MEMORY*                  pMem;
    XGL_RESULT result;
} struct_xglOpenPeerMemory;

static struct_xglOpenPeerMemory* interpret_body_as_xglOpenPeerMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglOpenPeerMemory* pPacket = (struct_xglOpenPeerMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_PEER_MEMORY_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    return pPacket;
}

typedef struct struct_xglOpenPeerImage {
    glv_trace_packet_header*  header;
    XGL_DEVICE                      device;
    const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo;
    XGL_IMAGE*                      pImage;
    XGL_GPU_MEMORY*                 pMem;
    XGL_RESULT result;
} struct_xglOpenPeerImage;

static struct_xglOpenPeerImage* interpret_body_as_xglOpenPeerImage(glv_trace_packet_header*  pHeader)
{
    struct_xglOpenPeerImage* pPacket = (struct_xglOpenPeerImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pOpenInfo = (const XGL_PEER_IMAGE_OPEN_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pOpenInfo);
    return pPacket;
}

// Generic API object functions

typedef struct struct_xglDestroyObject {
    glv_trace_packet_header*  header;
    XGL_OBJECT object;
    XGL_RESULT result;
} struct_xglDestroyObject;

static struct_xglDestroyObject* interpret_body_as_xglDestroyObject(glv_trace_packet_header*  pHeader)
{
    struct_xglDestroyObject* pPacket = (struct_xglDestroyObject*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglGetObjectInfo {
    glv_trace_packet_header*  header;
    XGL_BASE_OBJECT object;
    XGL_OBJECT_INFO_TYPE        infoType;
    XGL_SIZE*       pDataSize;
    XGL_VOID*       pData;
    XGL_RESULT result;
} struct_xglGetObjectInfo;

static struct_xglGetObjectInfo* interpret_body_as_xglGetObjectInfo(glv_trace_packet_header*  pHeader)
{
    struct_xglGetObjectInfo* pPacket = (struct_xglGetObjectInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (XGL_SIZE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglBindObjectMemory {
    glv_trace_packet_header*  header;
    XGL_OBJECT     object;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE   offset;
    XGL_RESULT result;
} struct_xglBindObjectMemory;

static struct_xglBindObjectMemory* interpret_body_as_xglBindObjectMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglBindObjectMemory* pPacket = (struct_xglBindObjectMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

// Fence functions

typedef struct struct_xglCreateFence {
    glv_trace_packet_header*  header;
    XGL_DEVICE                   device;
    const XGL_FENCE_CREATE_INFO* pCreateInfo;
    XGL_FENCE*                   pFence;
    XGL_RESULT result;
} struct_xglCreateFence;

static struct_xglCreateFence* interpret_body_as_xglCreateFence(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateFence* pPacket = (struct_xglCreateFence*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (XGL_FENCE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pFence = (XGL_FENCE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pFence);
    return pPacket;
}

typedef struct struct_xglGetFenceStatus {
    glv_trace_packet_header*  header;
    XGL_FENCE fence;
    XGL_RESULT result;
} struct_xglGetFenceStatus;

static struct_xglGetFenceStatus* interpret_body_as_xglGetFenceStatus(glv_trace_packet_header*  pHeader)
{
    struct_xglGetFenceStatus* pPacket = (struct_xglGetFenceStatus*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglWaitForFences {
    glv_trace_packet_header*  header;
    XGL_DEVICE       device;
    XGL_UINT         fenceCount;
    const XGL_FENCE* pFences;
    XGL_BOOL         waitAll;
    XGL_UINT64       timeout;
    XGL_RESULT result;
} struct_xglWaitForFences;

static struct_xglWaitForFences* interpret_body_as_xglWaitForFences(glv_trace_packet_header*  pHeader)
{
    struct_xglWaitForFences* pPacket = (struct_xglWaitForFences*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pFences = (const XGL_FENCE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pFences);
    return pPacket;
}

// Queue semaphore functions

typedef struct struct_xglCreateQueueSemaphore {
    glv_trace_packet_header*  header;
    XGL_DEVICE                             device;
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo;
    XGL_QUEUE_SEMAPHORE*                   pSemaphore;
    XGL_RESULT result;
} struct_xglCreateQueueSemaphore;

static struct_xglCreateQueueSemaphore* interpret_body_as_xglCreateQueueSemaphore(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateQueueSemaphore* pPacket = (struct_xglCreateQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_QUEUE_SEMAPHORE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pSemaphore = (XGL_QUEUE_SEMAPHORE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSemaphore);
    return pPacket;
}

typedef struct struct_xglSignalQueueSemaphore {
    glv_trace_packet_header*  header;
    XGL_QUEUE queue;
    XGL_QUEUE_SEMAPHORE semaphore;
    XGL_RESULT result;
} struct_xglSignalQueueSemaphore;

static struct_xglSignalQueueSemaphore* interpret_body_as_xglSignalQueueSemaphore(glv_trace_packet_header*  pHeader)
{
    struct_xglSignalQueueSemaphore* pPacket = (struct_xglSignalQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglWaitQueueSemaphore {
    glv_trace_packet_header*  header;
    XGL_QUEUE queue;
    XGL_QUEUE_SEMAPHORE semaphore;
    XGL_RESULT result;
} struct_xglWaitQueueSemaphore;

static struct_xglWaitQueueSemaphore* interpret_body_as_xglWaitQueueSemaphore(glv_trace_packet_header*  pHeader)
{
    struct_xglWaitQueueSemaphore* pPacket = (struct_xglWaitQueueSemaphore*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

// Event functions

typedef struct struct_xglCreateEvent {
    glv_trace_packet_header*  header;
    XGL_DEVICE                   device;
    const XGL_EVENT_CREATE_INFO* pCreateInfo;
    XGL_EVENT*                   pEvent;
    XGL_RESULT result;
} struct_xglCreateEvent;

static struct_xglCreateEvent* interpret_body_as_xglCreateEvent(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateEvent* pPacket = (struct_xglCreateEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_EVENT_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pEvent = (XGL_EVENT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pEvent);
    return pPacket;
}

typedef struct struct_xglGetEventStatus {
    glv_trace_packet_header*  header;
    XGL_EVENT event;
    XGL_RESULT result;
} struct_xglGetEventStatus;

static struct_xglGetEventStatus* interpret_body_as_xglGetEventStatus(glv_trace_packet_header*  pHeader)
{
    struct_xglGetEventStatus* pPacket = (struct_xglGetEventStatus*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglSetEvent {
    glv_trace_packet_header*  header;
    XGL_EVENT event;
    XGL_RESULT result;
} struct_xglSetEvent;

static struct_xglSetEvent* interpret_body_as_xglSetEvent(glv_trace_packet_header*  pHeader)
{
    struct_xglSetEvent* pPacket = (struct_xglSetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglResetEvent {
    glv_trace_packet_header*  header;
    XGL_EVENT event;
    XGL_RESULT result;
} struct_xglResetEvent;

static struct_xglResetEvent* interpret_body_as_xglResetEvent(glv_trace_packet_header*  pHeader)
{
    struct_xglResetEvent* pPacket = (struct_xglResetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

// Query functions

typedef struct struct_xglCreateQueryPool {
    glv_trace_packet_header*  header;
    XGL_DEVICE                        device;
    const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo;
    XGL_QUERY_POOL*                   pQueryPool;
    XGL_RESULT result;
} struct_xglCreateQueryPool;

static struct_xglCreateQueryPool* interpret_body_as_xglCreateQueryPool(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateQueryPool* pPacket = (struct_xglCreateQueryPool*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_QUERY_POOL_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pQueryPool = (XGL_QUERY_POOL*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pQueryPool);
    return pPacket;
}

typedef struct struct_xglGetQueryPoolResults {
    glv_trace_packet_header*  header;
    XGL_QUERY_POOL queryPool;
    XGL_UINT       startQuery;
    XGL_UINT       queryCount;
    XGL_SIZE*      pDataSize;
    XGL_VOID*      pData;
    XGL_RESULT result;
} struct_xglGetQueryPoolResults;

static struct_xglGetQueryPoolResults* interpret_body_as_xglGetQueryPoolResults(glv_trace_packet_header*  pHeader)
{
    struct_xglGetQueryPoolResults* pPacket = (struct_xglGetQueryPoolResults*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (XGL_SIZE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

// Format capabilities

typedef struct struct_xglGetFormatInfo {
    glv_trace_packet_header*  header;
    XGL_DEVICE device;
    XGL_FORMAT format;
    XGL_FORMAT_INFO_TYPE   infoType;
    XGL_SIZE*  pDataSize;
    XGL_VOID*  pData;
    XGL_RESULT result;
} struct_xglGetFormatInfo;

static struct_xglGetFormatInfo* interpret_body_as_xglGetFormatInfo(glv_trace_packet_header*  pHeader)
{
    struct_xglGetFormatInfo* pPacket = (struct_xglGetFormatInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (XGL_SIZE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

// Image functions

typedef struct struct_xglCreateImage {
    glv_trace_packet_header*  header;
    XGL_DEVICE                   device;
    const XGL_IMAGE_CREATE_INFO* pCreateInfo;
    XGL_IMAGE*                   pImage;
    XGL_RESULT result;
} struct_xglCreateImage;

static struct_xglCreateImage* interpret_body_as_xglCreateImage(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateImage* pPacket = (struct_xglCreateImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_IMAGE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pImage = (XGL_IMAGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pImage);
    return pPacket;
}

typedef struct struct_xglGetImageSubresourceInfo {
    glv_trace_packet_header*  header;
    XGL_IMAGE                    image;
    const XGL_IMAGE_SUBRESOURCE* pSubresource;
    XGL_SUBRESOURCE_INFO_TYPE    infoType;
    XGL_SIZE*                    pDataSize;
    XGL_VOID*                    pData;
    XGL_RESULT result;
} struct_xglGetImageSubresourceInfo;

static struct_xglGetImageSubresourceInfo* interpret_body_as_xglGetImageSubresourceInfo(glv_trace_packet_header*  pHeader)
{
    struct_xglGetImageSubresourceInfo* pPacket = (struct_xglGetImageSubresourceInfo*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pSubresource = (const XGL_IMAGE_SUBRESOURCE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSubresource);
    pPacket->pDataSize = (XGL_SIZE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

// Image view functions

typedef struct struct_xglCreateImageView {
    glv_trace_packet_header*  header;
    XGL_DEVICE                        device;
    const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo;
    XGL_IMAGE_VIEW*                   pView;
    XGL_RESULT result;
} struct_xglCreateImageView;

static struct_xglCreateImageView* interpret_body_as_xglCreateImageView(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateImageView* pPacket = (struct_xglCreateImageView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_IMAGE_VIEW_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pView = (XGL_IMAGE_VIEW*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pView);
    return pPacket;
}

typedef struct struct_xglCreateColorAttachmentView {
    glv_trace_packet_header*  header;
    XGL_DEVICE                               device;
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo;
    XGL_COLOR_ATTACHMENT_VIEW*                   pView;
    XGL_RESULT result;
} struct_xglCreateColorAttachmentView;

static struct_xglCreateColorAttachmentView* interpret_body_as_xglCreateColorAttachmentView(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateColorAttachmentView* pPacket = (struct_xglCreateColorAttachmentView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pView = (XGL_COLOR_ATTACHMENT_VIEW*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pView);
    return pPacket;
}

typedef struct struct_xglCreateDepthStencilView {
    glv_trace_packet_header*  header;
    XGL_DEVICE                                device;
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo;
    XGL_DEPTH_STENCIL_VIEW*                   pView;
    XGL_RESULT result;
} struct_xglCreateDepthStencilView;

static struct_xglCreateDepthStencilView* interpret_body_as_xglCreateDepthStencilView(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateDepthStencilView* pPacket = (struct_xglCreateDepthStencilView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pView = (XGL_DEPTH_STENCIL_VIEW*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pView);
    return pPacket;
}

// Shader functions

typedef struct struct_xglCreateShader {
    glv_trace_packet_header*  header;
    XGL_DEVICE                    device;
    const XGL_SHADER_CREATE_INFO* pCreateInfo;
    XGL_SHADER*                   pShader;
    XGL_RESULT result;
} struct_xglCreateShader;

static struct_xglCreateShader* interpret_body_as_xglCreateShader(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateShader* pPacket = (struct_xglCreateShader*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_SHADER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pShader = (XGL_SHADER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pShader);
    if (pPacket->pCreateInfo != NULL)
    {
        XGL_SHADER_CREATE_INFO* pInfo = (XGL_SHADER_CREATE_INFO*)pPacket->pCreateInfo;
        pInfo->pCode = glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pCode);
    }
    return pPacket;
}

// Pipeline functions

typedef struct struct_xglCreateGraphicsPipeline {
    glv_trace_packet_header*  header;
    XGL_DEVICE                               device;
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo;
    XGL_PIPELINE*                            pPipeline;
    XGL_RESULT result;
} struct_xglCreateGraphicsPipeline;

static void interpret_pipeline_shader(glv_trace_packet_header*  pHeader, XGL_PIPELINE_SHADER* pShader)
{
    XGL_UINT i, j;
    if (pShader != NULL)
    {
        // descriptor sets
        // TODO: need to ensure XGL_MAX_DESCRIPTOR_SETS is equal in replay as it was at trace time - meta data
        for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)
        {
            pShader->descriptorSetMapping[i].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->descriptorSetMapping[i].pDescriptorInfo);
            for (j = 0; j < pShader->descriptorSetMapping[i].descriptorCount; j++)
            {
                if (pShader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)
                {
                    XGL_DESCRIPTOR_SLOT_INFO* pInfo = (XGL_DESCRIPTOR_SLOT_INFO*)pShader->descriptorSetMapping[i].pDescriptorInfo;
                    pInfo[j].pNextLevelSet = (const XGL_DESCRIPTOR_SET_MAPPING*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet);
                }
            }
        }

        // constant buffers
        if (pShader->linkConstBufferCount > 0)
        {
            XGL_UINT i;
            pShader->pLinkConstBufferInfo = (const XGL_LINK_CONST_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo);
            for (i = 0; i < pShader->linkConstBufferCount; i++)
            {
                XGL_LINK_CONST_BUFFER* pBuffer = (XGL_LINK_CONST_BUFFER*)pShader->pLinkConstBufferInfo;
                pBuffer[i].pBufferData = (const XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pShader->pLinkConstBufferInfo[i].pBufferData);
            }
        }
    }
}

static struct_xglCreateGraphicsPipeline* interpret_body_as_xglCreateGraphicsPipeline(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateGraphicsPipeline* pPacket = (struct_xglCreateGraphicsPipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pPipeline = (XGL_PIPELINE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPipeline);
    pPacket->pCreateInfo = (const XGL_GRAPHICS_PIPELINE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);

    if (pPacket->pCreateInfo != NULL)
    {
        assert(pPacket->pCreateInfo->sType == XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

        // need to make a non-const pointer to the pointer so that we can properly change the original pointer to the interpretted one
        XGL_VOID** ppNextVoidPtr = (XGL_VOID**)&pPacket->pCreateInfo->pNext;
        *ppNextVoidPtr = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo->pNext);

        XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;
        while ((NULL != pNext) && (XGL_NULL_HANDLE != pNext))
        {
            switch(pNext->sType)
            {
                case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
                case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
                case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
                case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
                case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
                {
                    XGL_VOID** ppNextVoidPtr = (XGL_VOID**)&pNext->pNext;
                    *ppNextVoidPtr = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);
                    break;
                }
                case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
                {
                    XGL_VOID** ppNextVoidPtr = (XGL_VOID**)&pNext->pNext;
                    *ppNextVoidPtr = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pNext->pNext);

                    interpret_pipeline_shader(pHeader, &pNext->shader);

                    break;
                }
                default:
                    assert(!"Encountered an unexpected type in pipeline state list");
            }

            pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;
        }
    }
    return pPacket;
}

typedef struct struct_xglCreateComputePipeline {
    glv_trace_packet_header*  header;
    XGL_DEVICE                              device;
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo;
    XGL_PIPELINE*                           pPipeline;
    XGL_RESULT result;
} struct_xglCreateComputePipeline;

static struct_xglCreateComputePipeline* interpret_body_as_xglCreateComputePipeline(glv_trace_packet_header*  pHeader)
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
    glv_trace_packet_header*  header;
    XGL_PIPELINE pipeline;
    XGL_SIZE*    pDataSize;
    XGL_VOID*    pData;
    XGL_RESULT result;
} struct_xglStorePipeline;

static struct_xglStorePipeline* interpret_body_as_xglStorePipeline(glv_trace_packet_header*  pHeader)
{
    struct_xglStorePipeline* pPacket = (struct_xglStorePipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pDataSize = (XGL_SIZE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDataSize);
    pPacket->pData = (XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglLoadPipeline {
    glv_trace_packet_header*  header;
    XGL_DEVICE      device;
    XGL_SIZE        dataSize;
    const XGL_VOID* pData;
    XGL_PIPELINE*   pPipeline;
    XGL_RESULT result;
} struct_xglLoadPipeline;

static struct_xglLoadPipeline* interpret_body_as_xglLoadPipeline(glv_trace_packet_header*  pHeader)
{
    struct_xglLoadPipeline* pPacket = (struct_xglLoadPipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (const XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    pPacket->pPipeline = (XGL_PIPELINE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPipeline);
    return pPacket;
}
typedef struct struct_xglCreatePipelineDelta {
    glv_trace_packet_header*  header;
    XGL_DEVICE   device;
    XGL_PIPELINE p1;
    XGL_PIPELINE p2;
    XGL_PIPELINE_DELTA* delta;
    XGL_RESULT result;
} struct_xglCreatePipelineDelta;

static struct_xglCreatePipelineDelta* interpret_body_as_xglCreatePipelineDelta(glv_trace_packet_header*  pHeader)
{
    struct_xglCreatePipelineDelta* pPacket = (struct_xglCreatePipelineDelta*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->delta = (XGL_PIPELINE_DELTA*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->delta);
    return pPacket;
}

// Sampler functions

typedef struct struct_xglCreateSampler {
    glv_trace_packet_header*  header;
    XGL_DEVICE                     device;
    const XGL_SAMPLER_CREATE_INFO* pCreateInfo;
    XGL_SAMPLER*                   pSampler;
    XGL_RESULT result;
} struct_xglCreateSampler;

static struct_xglCreateSampler* interpret_body_as_xglCreateSampler(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateSampler* pPacket = (struct_xglCreateSampler*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_SAMPLER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pSampler = (XGL_SAMPLER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSampler);
    return pPacket;
}

// Descriptor set functions

typedef struct struct_xglCreateDescriptorSet {
    glv_trace_packet_header*  header;
    XGL_DEVICE                            device;
    const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo;
    XGL_DESCRIPTOR_SET*                   pDescriptorSet;
    XGL_RESULT result;
} struct_xglCreateDescriptorSet;

static struct_xglCreateDescriptorSet* interpret_body_as_xglCreateDescriptorSet(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateDescriptorSet* pPacket = (struct_xglCreateDescriptorSet*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DESCRIPTOR_SET_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pDescriptorSet = (XGL_DESCRIPTOR_SET*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDescriptorSet);
    return pPacket;
}

typedef struct struct_xglBeginDescriptorSetUpdate {
    glv_trace_packet_header*  header;
    XGL_DESCRIPTOR_SET descriptorSet;
} struct_xglBeginDescriptorSetUpdate;

static struct_xglBeginDescriptorSetUpdate* interpret_body_as_xglBeginDescriptorSetUpdate(glv_trace_packet_header*  pHeader)
{
    struct_xglBeginDescriptorSetUpdate* pPacket = (struct_xglBeginDescriptorSetUpdate*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglEndDescriptorSetUpdate {
    glv_trace_packet_header*  header;
    XGL_DESCRIPTOR_SET descriptorSet;
} struct_xglEndDescriptorSetUpdate;

static struct_xglEndDescriptorSetUpdate* interpret_body_as_xglEndDescriptorSetUpdate(glv_trace_packet_header*  pHeader)
{
    struct_xglEndDescriptorSetUpdate* pPacket = (struct_xglEndDescriptorSetUpdate*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglAttachSamplerDescriptors {
    glv_trace_packet_header*  header;
    XGL_DESCRIPTOR_SET descriptorSet;
    XGL_UINT           startSlot;
    XGL_UINT           slotCount;
    const XGL_SAMPLER* pSamplers;
} struct_xglAttachSamplerDescriptors;

static struct_xglAttachSamplerDescriptors* interpret_body_as_xglAttachSamplerDescriptors(glv_trace_packet_header*  pHeader)
{
    struct_xglAttachSamplerDescriptors* pPacket = (struct_xglAttachSamplerDescriptors*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pSamplers = (const XGL_SAMPLER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pSamplers);
    return pPacket;
}

typedef struct struct_xglAttachImageViewDescriptors {
    glv_trace_packet_header*  header;
    XGL_DESCRIPTOR_SET                descriptorSet;
    XGL_UINT                          startSlot;
    XGL_UINT                          slotCount;
    const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews;
} struct_xglAttachImageViewDescriptors;

static struct_xglAttachImageViewDescriptors* interpret_body_as_xglAttachImageViewDescriptors(glv_trace_packet_header*  pHeader)
{
    struct_xglAttachImageViewDescriptors* pPacket = (struct_xglAttachImageViewDescriptors*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pImageViews = (const XGL_IMAGE_VIEW_ATTACH_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pImageViews);
    return pPacket;
}

typedef struct struct_xglAttachMemoryViewDescriptors {
    glv_trace_packet_header*  header;
    XGL_DESCRIPTOR_SET                 descriptorSet;
    XGL_UINT                           startSlot;
    XGL_UINT                           slotCount;
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews;
} struct_xglAttachMemoryViewDescriptors;

static struct_xglAttachMemoryViewDescriptors* interpret_body_as_xglAttachMemoryViewDescriptors(glv_trace_packet_header*  pHeader)
{
    struct_xglAttachMemoryViewDescriptors* pPacket = (struct_xglAttachMemoryViewDescriptors*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pMemViews = (const XGL_MEMORY_VIEW_ATTACH_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMemViews);
    return pPacket;
}

typedef struct struct_xglAttachNestedDescriptors {
    glv_trace_packet_header*  header;
    XGL_DESCRIPTOR_SET                    descriptorSet;
    XGL_UINT                              startSlot;
    XGL_UINT                              slotCount;
    const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets;
} struct_xglAttachNestedDescriptors;

static struct_xglAttachNestedDescriptors* interpret_body_as_xglAttachNestedDescriptors(glv_trace_packet_header*  pHeader)
{
    struct_xglAttachNestedDescriptors* pPacket = (struct_xglAttachNestedDescriptors*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pNestedDescriptorSets = (const XGL_DESCRIPTOR_SET_ATTACH_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pNestedDescriptorSets);
    return pPacket;
}

typedef struct struct_xglClearDescriptorSetSlots {
    glv_trace_packet_header*  header;
    XGL_DESCRIPTOR_SET descriptorSet;
    XGL_UINT           startSlot;
    XGL_UINT           slotCount;
} struct_xglClearDescriptorSetSlots;

static struct_xglClearDescriptorSetSlots* interpret_body_as_xglClearDescriptorSetSlots(glv_trace_packet_header*  pHeader)
{
    struct_xglClearDescriptorSetSlots* pPacket = (struct_xglClearDescriptorSetSlots*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

// State object functions

typedef struct struct_xglCreateViewportState {
    glv_trace_packet_header*  header;
    XGL_DEVICE                            device;
    const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo;
    XGL_VIEWPORT_STATE_OBJECT*            pState;
    XGL_RESULT result;
} struct_xglCreateViewportState;

static struct_xglCreateViewportState* interpret_body_as_xglCreateViewportState(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateViewportState* pPacket = (struct_xglCreateViewportState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_VIEWPORT_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_VIEWPORT_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateRasterState {
    glv_trace_packet_header*  header;
    XGL_DEVICE                          device;
    const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo;
    XGL_RASTER_STATE_OBJECT*            pState;
    XGL_RESULT result;
} struct_xglCreateRasterState;

static struct_xglCreateRasterState* interpret_body_as_xglCreateRasterState(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateRasterState* pPacket = (struct_xglCreateRasterState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_RASTER_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_RASTER_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateMsaaState {
    glv_trace_packet_header*  header;
    XGL_DEVICE                        device;
    const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo;
    XGL_MSAA_STATE_OBJECT*            pState;
    XGL_RESULT result;
} struct_xglCreateMsaaState;

static struct_xglCreateMsaaState* interpret_body_as_xglCreateMsaaState(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateMsaaState* pPacket = (struct_xglCreateMsaaState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_MSAA_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_MSAA_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateColorBlendState {
    glv_trace_packet_header*  header;
    XGL_DEVICE                               device;
    const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo;
    XGL_COLOR_BLEND_STATE_OBJECT*            pState;
    XGL_RESULT result;
} struct_xglCreateColorBlendState;

static struct_xglCreateColorBlendState* interpret_body_as_xglCreateColorBlendState(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateColorBlendState* pPacket = (struct_xglCreateColorBlendState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_COLOR_BLEND_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_COLOR_BLEND_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

typedef struct struct_xglCreateDepthStencilState {
    glv_trace_packet_header*  header;
    XGL_DEVICE                                 device;
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo;
    XGL_DEPTH_STENCIL_STATE_OBJECT*            pState;
    XGL_RESULT result;
} struct_xglCreateDepthStencilState;

static struct_xglCreateDepthStencilState* interpret_body_as_xglCreateDepthStencilState(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateDepthStencilState* pPacket = (struct_xglCreateDepthStencilState*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pState = (XGL_DEPTH_STENCIL_STATE_OBJECT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pState);
    return pPacket;
}

// Command buffer functions

typedef struct struct_xglCreateCommandBuffer {
    glv_trace_packet_header*  header;
    XGL_DEVICE                        device;
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo;
    XGL_CMD_BUFFER*                   pCmdBuffer;
    XGL_RESULT result;
} struct_xglCreateCommandBuffer;

static struct_xglCreateCommandBuffer* interpret_body_as_xglCreateCommandBuffer(glv_trace_packet_header*  pHeader)
{
    struct_xglCreateCommandBuffer* pPacket = (struct_xglCreateCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pCreateInfo = (const XGL_CMD_BUFFER_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pCmdBuffer = (XGL_CMD_BUFFER*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCmdBuffer);
    return pPacket;
}

typedef struct struct_xglBeginCommandBuffer {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_FLAGS      flags;               // XGL_CMD_BUFFER_BUILD_FLAG
    XGL_RESULT result;
} struct_xglBeginCommandBuffer;

static struct_xglBeginCommandBuffer* interpret_body_as_xglBeginCommandBuffer(glv_trace_packet_header*  pHeader)
{
    struct_xglBeginCommandBuffer* pPacket = (struct_xglBeginCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglEndCommandBuffer {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_RESULT result;
} struct_xglEndCommandBuffer;

static struct_xglEndCommandBuffer* interpret_body_as_xglEndCommandBuffer(glv_trace_packet_header*  pHeader)
{
    struct_xglEndCommandBuffer* pPacket = (struct_xglEndCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglResetCommandBuffer {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_RESULT result;
} struct_xglResetCommandBuffer;

static struct_xglResetCommandBuffer* interpret_body_as_xglResetCommandBuffer(glv_trace_packet_header*  pHeader)
{
    struct_xglResetCommandBuffer* pPacket = (struct_xglResetCommandBuffer*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

// Command buffer building functions

typedef struct struct_xglCmdBindPipeline {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint;
    XGL_PIPELINE   pipeline;
} struct_xglCmdBindPipeline;

static struct_xglCmdBindPipeline* interpret_body_as_xglCmdBindPipeline(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBindPipeline* pPacket = (struct_xglCmdBindPipeline*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindPipelineDelta {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint;
    XGL_PIPELINE_DELTA   delta;
} struct_xglCmdBindPipelineDelta;

static struct_xglCmdBindPipelineDelta* interpret_body_as_xglCmdBindPipelineDelta(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBindPipelineDelta* pPacket = (struct_xglCmdBindPipelineDelta*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindStateObject {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER   cmdBuffer;
    XGL_STATE_BIND_POINT         stateBindPoint;
    XGL_STATE_OBJECT state;
} struct_xglCmdBindStateObject;

static struct_xglCmdBindStateObject* interpret_body_as_xglCmdBindStateObject(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBindStateObject* pPacket = (struct_xglCmdBindStateObject*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindDescriptorSet {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER     cmdBuffer;
    XGL_PIPELINE_BIND_POINT           pipelineBindPoint;
    XGL_UINT           index;
    XGL_DESCRIPTOR_SET descriptorSet;
    XGL_UINT           slotOffset;
} struct_xglCmdBindDescriptorSet;

static struct_xglCmdBindDescriptorSet* interpret_body_as_xglCmdBindDescriptorSet(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBindDescriptorSet* pPacket = (struct_xglCmdBindDescriptorSet*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindDynamicMemoryView {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER                     cmdBuffer;
    XGL_PIPELINE_BIND_POINT                           pipelineBindPoint;
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView;
} struct_xglCmdBindDynamicMemoryView;

static struct_xglCmdBindDynamicMemoryView* interpret_body_as_xglCmdBindDynamicMemoryView(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBindDynamicMemoryView* pPacket = (struct_xglCmdBindDynamicMemoryView*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pMemView = (const XGL_MEMORY_VIEW_ATTACH_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMemView);
    return pPacket;
}

typedef struct struct_xglCmdBindIndexData {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE   offset;
    XGL_INDEX_TYPE       indexType;
} struct_xglCmdBindIndexData;

static struct_xglCmdBindIndexData* interpret_body_as_xglCmdBindIndexData(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBindIndexData* pPacket = (struct_xglCmdBindIndexData*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBindAttachments {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER                     cmdBuffer;
    XGL_UINT                           colorAttachmentCount;
    const XGL_COLOR_ATTACHMENT_BIND_INFO*  pColorAttachments;
    const XGL_DEPTH_STENCIL_BIND_INFO* pDepthAttachment;
} struct_xglCmdBindAttachments;

static struct_xglCmdBindAttachments* interpret_body_as_xglCmdBindAttachments(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBindAttachments* pPacket = (struct_xglCmdBindAttachments*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pColorAttachments = (const XGL_COLOR_ATTACHMENT_BIND_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pColorAttachments);
    pPacket->pDepthAttachment = (const XGL_DEPTH_STENCIL_BIND_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pDepthAttachment);
    return pPacket;
}

typedef struct struct_xglCmdPrepareMemoryRegions {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER                     cmdBuffer;
    XGL_UINT                           transitionCount;
    const XGL_MEMORY_STATE_TRANSITION* pStateTransitions;
} struct_xglCmdPrepareMemoryRegions;

static struct_xglCmdPrepareMemoryRegions* interpret_body_as_xglCmdPrepareMemoryRegions(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdPrepareMemoryRegions* pPacket = (struct_xglCmdPrepareMemoryRegions*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pStateTransitions = (const XGL_MEMORY_STATE_TRANSITION*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pStateTransitions);
    return pPacket;
}

typedef struct struct_xglCmdPrepareImages {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER                    cmdBuffer;
    XGL_UINT                          transitionCount;
    const XGL_IMAGE_STATE_TRANSITION* pStateTransitions;
} struct_xglCmdPrepareImages;

static struct_xglCmdPrepareImages* interpret_body_as_xglCmdPrepareImages(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdPrepareImages* pPacket = (struct_xglCmdPrepareImages*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pStateTransitions = (const XGL_IMAGE_STATE_TRANSITION*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pStateTransitions);
    return pPacket;
}

typedef struct struct_xglCmdDraw {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_UINT       firstVertex;
    XGL_UINT       vertexCount;
    XGL_UINT       firstInstance;
    XGL_UINT       instanceCount;
} struct_xglCmdDraw;

static struct_xglCmdDraw* interpret_body_as_xglCmdDraw(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdDraw* pPacket = (struct_xglCmdDraw*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDrawIndexed {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_UINT       firstIndex;
    XGL_UINT       indexCount;
    XGL_INT        vertexOffset;
    XGL_UINT       firstInstance;
    XGL_UINT       instanceCount;
} struct_xglCmdDrawIndexed;

static struct_xglCmdDrawIndexed* interpret_body_as_xglCmdDrawIndexed(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdDrawIndexed* pPacket = (struct_xglCmdDrawIndexed*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDrawIndirect {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE   offset;
    XGL_UINT32     count;
    XGL_UINT32     stride;
} struct_xglCmdDrawIndirect;

static struct_xglCmdDrawIndirect* interpret_body_as_xglCmdDrawIndirect(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdDrawIndirect* pPacket = (struct_xglCmdDrawIndirect*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDrawIndexedIndirect {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE   offset;
    XGL_UINT32     count;
    XGL_UINT32     stride;
} struct_xglCmdDrawIndexedIndirect;

static struct_xglCmdDrawIndexedIndirect* interpret_body_as_xglCmdDrawIndexedIndirect(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdDrawIndexedIndirect* pPacket = (struct_xglCmdDrawIndexedIndirect*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDispatch {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_UINT       x;
    XGL_UINT       y;
    XGL_UINT       z;
} struct_xglCmdDispatch;

static struct_xglCmdDispatch* interpret_body_as_xglCmdDispatch(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdDispatch* pPacket = (struct_xglCmdDispatch*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdDispatchIndirect {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_GPU_MEMORY mem;
    XGL_GPU_SIZE   offset;
} struct_xglCmdDispatchIndirect;

static struct_xglCmdDispatchIndirect* interpret_body_as_xglCmdDispatchIndirect(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdDispatchIndirect* pPacket = (struct_xglCmdDispatchIndirect*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdCopyMemory {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER         cmdBuffer;
    XGL_GPU_MEMORY         srcMem;
    XGL_GPU_MEMORY         destMem;
    XGL_UINT               regionCount;
    const XGL_MEMORY_COPY* pRegions;
} struct_xglCmdCopyMemory;

static struct_xglCmdCopyMemory* interpret_body_as_xglCmdCopyMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdCopyMemory* pPacket = (struct_xglCmdCopyMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_MEMORY_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCopyImage {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER        cmdBuffer;
    XGL_IMAGE             srcImage;
    XGL_IMAGE             destImage;
    XGL_UINT              regionCount;
    const XGL_IMAGE_COPY* pRegions;
} struct_xglCmdCopyImage;

static struct_xglCmdCopyImage* interpret_body_as_xglCmdCopyImage(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdCopyImage* pPacket = (struct_xglCmdCopyImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_IMAGE_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCopyMemoryToImage {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER               cmdBuffer;
    XGL_GPU_MEMORY               srcMem;
    XGL_IMAGE                    destImage;
    XGL_UINT                     regionCount;
    const XGL_MEMORY_IMAGE_COPY* pRegions;
} struct_xglCmdCopyMemoryToImage;

static struct_xglCmdCopyMemoryToImage* interpret_body_as_xglCmdCopyMemoryToImage(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdCopyMemoryToImage* pPacket = (struct_xglCmdCopyMemoryToImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_MEMORY_IMAGE_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCopyImageToMemory {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER               cmdBuffer;
    XGL_IMAGE                    srcImage;
    XGL_GPU_MEMORY               destMem;
    XGL_UINT                     regionCount;
    const XGL_MEMORY_IMAGE_COPY* pRegions;
} struct_xglCmdCopyImageToMemory;

static struct_xglCmdCopyImageToMemory* interpret_body_as_xglCmdCopyImageToMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdCopyImageToMemory* pPacket = (struct_xglCmdCopyImageToMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRegions = (const XGL_MEMORY_IMAGE_COPY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRegions);
    return pPacket;
}

typedef struct struct_xglCmdCloneImageData {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER  cmdBuffer;
    XGL_IMAGE       srcImage;
    XGL_IMAGE_STATE        srcImageState;
    XGL_IMAGE       destImage;
    XGL_IMAGE_STATE        destImageState;
} struct_xglCmdCloneImageData;

static struct_xglCmdCloneImageData* interpret_body_as_xglCmdCloneImageData(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdCloneImageData* pPacket = (struct_xglCmdCloneImageData*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdUpdateMemory {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER    cmdBuffer;
    XGL_GPU_MEMORY    destMem;
    XGL_GPU_SIZE      destOffset;
    XGL_GPU_SIZE      dataSize;
    const XGL_UINT32* pData;
} struct_xglCmdUpdateMemory;

static struct_xglCmdUpdateMemory* interpret_body_as_xglCmdUpdateMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdUpdateMemory* pPacket = (struct_xglCmdUpdateMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (const XGL_UINT32*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglCmdFillMemory {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_GPU_MEMORY destMem;
    XGL_GPU_SIZE   destOffset;
    XGL_GPU_SIZE   fillSize;
    XGL_UINT32     data;
} struct_xglCmdFillMemory;

static struct_xglCmdFillMemory* interpret_body_as_xglCmdFillMemory(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdFillMemory* pPacket = (struct_xglCmdFillMemory*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdClearColorImage {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER                     cmdBuffer;
    XGL_IMAGE                          image;
    const XGL_FLOAT                    color[4];
    XGL_UINT                           rangeCount;
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges;
} struct_xglCmdClearColorImage;

static struct_xglCmdClearColorImage* interpret_body_as_xglCmdClearColorImage(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdClearColorImage* pPacket = (struct_xglCmdClearColorImage*)pHeader->pBody;
    pPacket->header = pHeader;
//    pPacket->color = (const XGL_FLOAT*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->color);
    pPacket->pRanges = (const XGL_IMAGE_SUBRESOURCE_RANGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRanges);
    return pPacket;
}

typedef struct struct_xglCmdClearColorImageRaw {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER                     cmdBuffer;
    XGL_IMAGE                          image;
    const XGL_UINT32                   color[4];
    XGL_UINT                           rangeCount;
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges;
} struct_xglCmdClearColorImageRaw;

static struct_xglCmdClearColorImageRaw* interpret_body_as_xglCmdClearColorImageRaw(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdClearColorImageRaw* pPacket = (struct_xglCmdClearColorImageRaw*)pHeader->pBody;
    pPacket->header = pHeader;
//    pPacket->color = (const XGL_UINT32*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->color);
    pPacket->pRanges = (const XGL_IMAGE_SUBRESOURCE_RANGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRanges);
    return pPacket;
}

typedef struct struct_xglCmdClearDepthStencil {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER                     cmdBuffer;
    XGL_IMAGE                          image;
    XGL_FLOAT                          depth;
    XGL_UINT8                          stencil;
    XGL_UINT                           rangeCount;
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges;
} struct_xglCmdClearDepthStencil;

static struct_xglCmdClearDepthStencil* interpret_body_as_xglCmdClearDepthStencil(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdClearDepthStencil* pPacket = (struct_xglCmdClearDepthStencil*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRanges = (const XGL_IMAGE_SUBRESOURCE_RANGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRanges);
    return pPacket;
}

typedef struct struct_xglCmdResolveImage {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER           cmdBuffer;
    XGL_IMAGE                srcImage;
    XGL_IMAGE                destImage;
    XGL_UINT                 rectCount;
    const XGL_IMAGE_RESOLVE* pRects;
} struct_xglCmdResolveImage;

static struct_xglCmdResolveImage* interpret_body_as_xglCmdResolveImage(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdResolveImage* pPacket = (struct_xglCmdResolveImage*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pRects = (const XGL_IMAGE_RESOLVE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pRects);
    return pPacket;
}

typedef struct struct_xglCmdSetEvent {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_EVENT      event;
} struct_xglCmdSetEvent;

static struct_xglCmdSetEvent* interpret_body_as_xglCmdSetEvent(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdSetEvent* pPacket = (struct_xglCmdSetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdResetEvent {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_EVENT      event;
} struct_xglCmdResetEvent;

static struct_xglCmdResetEvent* interpret_body_as_xglCmdResetEvent(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdResetEvent* pPacket = (struct_xglCmdResetEvent*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdMemoryAtomic {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_GPU_MEMORY destMem;
    XGL_GPU_SIZE   destOffset;
    XGL_UINT64     srcData;
    XGL_ATOMIC_OP atomicOp;
} struct_xglCmdMemoryAtomic;

static struct_xglCmdMemoryAtomic* interpret_body_as_xglCmdMemoryAtomic(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdMemoryAtomic* pPacket = (struct_xglCmdMemoryAtomic*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdBeginQuery {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_QUERY_POOL queryPool;
    XGL_UINT       slot;
    XGL_FLAGS      flags;
} struct_xglCmdBeginQuery;

static struct_xglCmdBeginQuery* interpret_body_as_xglCmdBeginQuery(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdBeginQuery* pPacket = (struct_xglCmdBeginQuery*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdEndQuery {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_QUERY_POOL queryPool;
    XGL_UINT       slot;
} struct_xglCmdEndQuery;

static struct_xglCmdEndQuery* interpret_body_as_xglCmdEndQuery(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdEndQuery* pPacket = (struct_xglCmdEndQuery*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdResetQueryPool {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_QUERY_POOL queryPool;
    XGL_UINT       startQuery;
    XGL_UINT       queryCount;
} struct_xglCmdResetQueryPool;

static struct_xglCmdResetQueryPool* interpret_body_as_xglCmdResetQueryPool(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdResetQueryPool* pPacket = (struct_xglCmdResetQueryPool*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdWriteTimestamp {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_TIMESTAMP_TYPE       timestampType;
    XGL_GPU_MEMORY destMem;
    XGL_GPU_SIZE   destOffset;
} struct_xglCmdWriteTimestamp;

static struct_xglCmdWriteTimestamp* interpret_body_as_xglCmdWriteTimestamp(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdWriteTimestamp* pPacket = (struct_xglCmdWriteTimestamp*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdInitAtomicCounters {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER    cmdBuffer;
    XGL_PIPELINE_BIND_POINT          pipelineBindPoint;
    XGL_UINT          startCounter;
    XGL_UINT          counterCount;
    const XGL_UINT32* pData;
} struct_xglCmdInitAtomicCounters;

static struct_xglCmdInitAtomicCounters* interpret_body_as_xglCmdInitAtomicCounters(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdInitAtomicCounters* pPacket = (struct_xglCmdInitAtomicCounters*)pHeader->pBody;
    pPacket->header = pHeader;
    pPacket->pData = (const XGL_UINT32*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglCmdLoadAtomicCounters {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint;
    XGL_UINT       startCounter;
    XGL_UINT       counterCount;
    XGL_GPU_MEMORY srcMem;
    XGL_GPU_SIZE   srcOffset;
} struct_xglCmdLoadAtomicCounters;

static struct_xglCmdLoadAtomicCounters* interpret_body_as_xglCmdLoadAtomicCounters(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdLoadAtomicCounters* pPacket = (struct_xglCmdLoadAtomicCounters*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}

typedef struct struct_xglCmdSaveAtomicCounters {
    glv_trace_packet_header*  header;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint;
    XGL_UINT       startCounter;
    XGL_UINT       counterCount;
    XGL_GPU_MEMORY destMem;
    XGL_GPU_SIZE   destOffset;
} struct_xglCmdSaveAtomicCounters;

static struct_xglCmdSaveAtomicCounters* interpret_body_as_xglCmdSaveAtomicCounters(glv_trace_packet_header*  pHeader)
{
    struct_xglCmdSaveAtomicCounters* pPacket = (struct_xglCmdSaveAtomicCounters*)pHeader->pBody;
    pPacket->header = pHeader;
    return pPacket;
}
