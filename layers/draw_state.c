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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader_platform.h"
#include "xgl_dispatch_table_helper.h"
#include "xgl_generic_intercept_proc_helper.h"
#include "xgl_struct_string_helper.h"
#include "xgl_struct_graphviz_helper.h"
#include "draw_state.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"
#include "layers_msg.h"
static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

// Return the size of the underlying struct based on struct type
static size_t sTypeStructSize(XGL_STRUCTURE_TYPE sType)
{
    switch (sType)
    {
        case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
            return sizeof(XGL_APPLICATION_INFO);
        case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
            return sizeof(XGL_DEVICE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
            return sizeof(XGL_MEMORY_ALLOC_INFO);
        case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
            return sizeof(XGL_MEMORY_OPEN_INFO);
        case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
            return sizeof(XGL_PEER_MEMORY_OPEN_INFO);
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO:
            return sizeof(XGL_BUFFER_VIEW_ATTACH_INFO);
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
            return sizeof(XGL_IMAGE_VIEW_ATTACH_INFO);
        case XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO:
            return sizeof(XGL_EVENT_WAIT_INFO);
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
            return sizeof(XGL_IMAGE_VIEW_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
            return sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
            return sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
            return sizeof(XGL_SHADER_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            return sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
            return sizeof(XGL_SAMPLER_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
            return sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_RS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_CB_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_DS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
            return sizeof(XGL_CMD_BUFFER_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
            return sizeof(XGL_EVENT_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
            return sizeof(XGL_FENCE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO:
            return sizeof(XGL_QUEUE_SEMAPHORE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO:
            return sizeof(XGL_QUEUE_SEMAPHORE_OPEN_INFO);
        case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
            return sizeof(XGL_QUERY_POOL_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            return sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
            return sizeof(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_IA_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_TESS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_VP_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_RS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_MS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_CB_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_DS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
            return sizeof(XGL_IMAGE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
            return sizeof(XGL_BUFFER_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
            return sizeof(XGL_BUFFER_VIEW_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
            return sizeof(XGL_FRAMEBUFFER_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO:
            return sizeof(XGL_CMD_BUFFER_BEGIN_INFO);
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
            return sizeof(XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO);
        case XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
            return sizeof(XGL_RENDER_PASS_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO:
            return sizeof(XGL_LAYER_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_BARRIER:
            return sizeof(XGL_PIPELINE_BARRIER);
        case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
            return sizeof(XGL_MEMORY_BARRIER);
        case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            return sizeof(XGL_BUFFER_MEMORY_BARRIER);
        case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
            return sizeof(XGL_IMAGE_MEMORY_BARRIER);
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO:
            return sizeof(XGL_DESCRIPTOR_REGION_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            return sizeof(XGL_UPDATE_SAMPLERS);
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            return sizeof(XGL_UPDATE_SAMPLER_TEXTURES);
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            return sizeof(XGL_UPDATE_IMAGES);
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            return sizeof(XGL_UPDATE_BUFFERS);
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            return sizeof(XGL_UPDATE_AS_COPY);
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
            return sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO);
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
            return sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO);
        default:
            return 0;
    }
}
// Return the size of the underlying struct based on sType
static size_t dynStateCreateInfoSize(XGL_STRUCTURE_TYPE sType)
{
    switch (sType)
    {
        case XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_RS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_DS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
            return sizeof(XGL_DYNAMIC_CB_STATE_CREATE_INFO);
        default:
            return 0;
    }
}
// Return a string representation of CMD_TYPE enum
static char* cmdTypeToString(CMD_TYPE cmd)
{
    switch (cmd)
    {
        case CMD_BINDPIPELINE:
            return "CMD_BINDPIPELINE";
        case CMD_BINDPIPELINEDELTA:
            return "CMD_BINDPIPELINEDELTA";
        case CMD_BINDDYNAMICSTATEOBJECT:
            return "CMD_BINDDYNAMICSTATEOBJECT";
        case CMD_BINDDESCRIPTORSET:
            return "CMD_BINDDESCRIPTORSET";
        case CMD_BINDINDEXBUFFER:
            return "CMD_BINDINDEXBUFFER";
        case CMD_BINDVERTEXBUFFER:
            return "CMD_BINDVERTEXBUFFER";
        case CMD_DRAW:
            return "CMD_DRAW";
        case CMD_DRAWINDEXED:
            return "CMD_DRAWINDEXED";
        case CMD_DRAWINDIRECT:
            return "CMD_DRAWINDIRECT";
        case CMD_DRAWINDEXEDINDIRECT:
            return "CMD_DRAWINDEXEDINDIRECT";
        case CMD_DISPATCH:
            return "CMD_DISPATCH";
        case CMD_DISPATCHINDIRECT:
            return "CMD_DISPATCHINDIRECT";
        case CMD_COPYBUFFER:
            return "CMD_COPYBUFFER";
        case CMD_COPYIMAGE:
            return "CMD_COPYIMAGE";
        case CMD_COPYBUFFERTOIMAGE:
            return "CMD_COPYBUFFERTOIMAGE";
        case CMD_COPYIMAGETOBUFFER:
            return "CMD_COPYIMAGETOBUFFER";
        case CMD_CLONEIMAGEDATA:
            return "CMD_CLONEIMAGEDATA";
        case CMD_UPDATEBUFFER:
            return "CMD_UPDATEBUFFER";
        case CMD_FILLBUFFER:
            return "CMD_FILLBUFFER";
        case CMD_CLEARCOLORIMAGE:
            return "CMD_CLEARCOLORIMAGE";
        case CMD_CLEARCOLORIMAGERAW:
            return "CMD_CLEARCOLORIMAGERAW";
        case CMD_CLEARDEPTHSTENCIL:
            return "CMD_CLEARDEPTHSTENCIL";
        case CMD_RESOLVEIMAGE:
            return "CMD_RESOLVEIMAGE";
        case CMD_SETEVENT:
            return "CMD_SETEVENT";
        case CMD_RESETEVENT:
            return "CMD_RESETEVENT";
        case CMD_WAITEVENTS:
            return "CMD_WAITEVENTS";
        case CMD_PIPELINEBARRIER:
            return "CMD_PIPELINEBARRIER";
        case CMD_BEGINQUERY:
            return "CMD_BEGINQUERY";
        case CMD_ENDQUERY:
            return "CMD_ENDQUERY";
        case CMD_RESETQUERYPOOL:
            return "CMD_RESETQUERYPOOL";
        case CMD_WRITETIMESTAMP:
            return "CMD_WRITETIMESTAMP";
        case CMD_INITATOMICCOUNTERS:
            return "CMD_INITATOMICCOUNTERS";
        case CMD_LOADATOMICCOUNTERS:
            return "CMD_LOADATOMICCOUNTERS";
        case CMD_SAVEATOMICCOUNTERS:
            return "CMD_SAVEATOMICCOUNTERS";
        case CMD_BEGINRENDERPASS:
            return "CMD_BEGINRENDERPASS";
        case CMD_ENDRENDERPASS:
            return "CMD_ENDRENDERPASS";
        default:
            return "UNKNOWN";
    }
}
// Block of code at start here for managing/tracking Pipeline state that this layer cares about
// Just track 2 shaders for now
#define XGL_NUM_GRAPHICS_SHADERS XGL_SHADER_STAGE_COMPUTE
#define MAX_SLOTS 2048

static uint64_t g_drawCount[NUM_DRAW_TYPES] = {0, 0, 0, 0};

// TODO : Should be tracking lastBound per cmdBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
static PIPELINE_NODE*    g_pPipelineHead = NULL;
static SAMPLER_NODE*     g_pSamplerHead = NULL;
static IMAGE_NODE*       g_pImageHead = NULL;
static BUFFER_NODE*      g_pBufferHead = NULL;
static GLOBAL_CB_NODE*   g_pCmdBufferHead = NULL;
static XGL_CMD_BUFFER    g_lastCmdBuffer = NULL;
#define MAX_BINDING 0xFFFFFFFF // Default vtxBinding value in CB Node to identify if no vtxBinding set

static DYNAMIC_STATE_NODE* g_pDynamicStateHead[XGL_NUM_STATE_BIND_POINT] = {0};

static void insertDynamicState(const XGL_DYNAMIC_STATE_OBJECT state, const GENERIC_HEADER* pCreateInfo, XGL_STATE_BIND_POINT bindPoint)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // Insert new node at head of appropriate LL
    DYNAMIC_STATE_NODE* pStateNode = (DYNAMIC_STATE_NODE*)malloc(sizeof(DYNAMIC_STATE_NODE));
    pStateNode->pNext = g_pDynamicStateHead[bindPoint];
    g_pDynamicStateHead[bindPoint] = pStateNode;
    pStateNode->stateObj = state;
    pStateNode->pCreateInfo = (GENERIC_HEADER*)malloc(dynStateCreateInfoSize(pCreateInfo->sType));
    memcpy(pStateNode->pCreateInfo, pCreateInfo, dynStateCreateInfoSize(pCreateInfo->sType));
    // VP has embedded ptr so need to handle that as special case
    if (XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO == pCreateInfo->sType) {
        XGL_DYNAMIC_VP_STATE_CREATE_INFO* pVPCI = (XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pStateNode->pCreateInfo;
        XGL_VIEWPORT** ppViewports = (XGL_VIEWPORT**)&pVPCI->pViewports;
        size_t vpSize = sizeof(XGL_VIEWPORT) * pVPCI->viewportAndScissorCount;
        if (vpSize) {
            *ppViewports = (XGL_VIEWPORT*)malloc(vpSize);
            memcpy(*ppViewports, ((XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pCreateInfo)->pViewports, vpSize);
        }
        XGL_RECT** ppScissors = (XGL_RECT**)&pVPCI->pScissors;
        size_t scSize = sizeof(XGL_RECT) * pVPCI->viewportAndScissorCount;
        if (scSize) {
            *ppScissors = (XGL_RECT*)malloc(scSize);
            memcpy(*ppScissors, ((XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pCreateInfo)->pScissors, scSize);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
}
// Free all allocated nodes for Dynamic State objs
static void freeDynamicState()
{
    for (uint32_t i = 0; i < XGL_NUM_STATE_BIND_POINT; i++) {
        DYNAMIC_STATE_NODE* pStateNode = g_pDynamicStateHead[i];
        DYNAMIC_STATE_NODE* pFreeMe = pStateNode;
        while (pStateNode) {
            pFreeMe = pStateNode;
            pStateNode = pStateNode->pNext;
            assert(pFreeMe->pCreateInfo);
            if (XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO == pFreeMe->pCreateInfo->sType) {
                XGL_DYNAMIC_VP_STATE_CREATE_INFO* pVPCI = (XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pFreeMe->pCreateInfo;
                if (pVPCI->pViewports) {
                    void** ppToFree = (void**)&pVPCI->pViewports;
                    free(*ppToFree);
                }
                if (pVPCI->pScissors) {
                    void** ppToFree = (void**)&pVPCI->pScissors;
                    free(*ppToFree);
                }
            }
            free(pFreeMe->pCreateInfo);
            free(pFreeMe);
        }
    }
}
// Free all sampler nodes
static void freeSamplers()
{
    SAMPLER_NODE* pSampler = g_pSamplerHead;
    SAMPLER_NODE* pFreeMe = pSampler;
    while (pSampler) {
        pFreeMe = pSampler;
        pSampler = pSampler->pNext;
        free(pFreeMe);
    }
}
// Free all image nodes
static void freeImages()
{
    IMAGE_NODE* pImage = g_pImageHead;
    IMAGE_NODE* pFreeMe = pImage;
    while (pImage) {
        pFreeMe = pImage;
        pImage = pImage->pNext;
        free(pFreeMe);
    }
}
// Free all buffer nodes
static void freeBuffers()
{
    BUFFER_NODE* pBuffer = g_pBufferHead;
    BUFFER_NODE* pFreeMe = pBuffer;
    while (pBuffer) {
        pFreeMe = pBuffer;
        pBuffer = pBuffer->pNext;
        free(pFreeMe);
    }
}
static GLOBAL_CB_NODE* getCBNode(XGL_CMD_BUFFER cb);
// Print the last bound dynamic state
static void printDynamicState(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        loader_platform_thread_lock_mutex(&globalLock);
        char str[1024];
        for (uint32_t i = 0; i < XGL_NUM_STATE_BIND_POINT; i++) {
            if (pCB->lastBoundDynamicState[i]) {
                sprintf(str, "Reporting CreateInfo for currently bound %s object %p", string_XGL_STATE_BIND_POINT(i), pCB->lastBoundDynamicState[i]->stateObj);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pCB->lastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", str);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pCB->lastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", dynamic_display(pCB->lastBoundDynamicState[i]->pCreateInfo, "  "));
                break;
            }
            else {
                sprintf(str, "No dynamic state of type %s bound", string_XGL_STATE_BIND_POINT(i));
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", str);
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
}
// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_NODE *getPipeline(XGL_PIPELINE pipeline)
{
    loader_platform_thread_lock_mutex(&globalLock);
    PIPELINE_NODE *pTrav = g_pPipelineHead;
    while (pTrav) {
        if (pTrav->pipeline == pipeline) {
            loader_platform_thread_unlock_mutex(&globalLock);
            return pTrav;
        }
        pTrav = pTrav->pNext;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return NULL;
}

// For given sampler, return a ptr to its Create Info struct, or NULL if sampler not found
// TODO : Use this function to display sampler info
//   commenting out for now to avoid warning about not being used
/*
static XGL_SAMPLER_CREATE_INFO* getSamplerCreateInfo(const XGL_SAMPLER sampler)
{
    loader_platform_thread_lock_mutex(&globalLock);
    SAMPLER_NODE *pTrav = g_pSamplerHead;
    while (pTrav) {
        if (sampler == pTrav->sampler) {
            loader_platform_thread_unlock_mutex(&globalLock);
            return &pTrav->createInfo;
        }
        pTrav = pTrav->pNext;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return NULL;
}
*/

// Init the pipeline mapping info based on pipeline create info LL tree
//  Threading note : Calls to this function should wrapped in mutex
static void initPipeline(PIPELINE_NODE *pPipeline, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo)
{
    // First init create info, we'll shadow the structs as we go down the tree
    pPipeline->pCreateTree = (XGL_GRAPHICS_PIPELINE_CREATE_INFO*)malloc(sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));
    memcpy(pPipeline->pCreateTree, pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));
    GENERIC_HEADER *pShadowTrav = (GENERIC_HEADER*)pPipeline->pCreateTree;
    GENERIC_HEADER *pTrav = (GENERIC_HEADER*)pCreateInfo->pNext;
    while (pTrav) {
        // Shadow the struct
        pShadowTrav->pNext = (GENERIC_HEADER*)malloc(sTypeStructSize(pTrav->sType));
        // Typically pNext is const so have to cast to avoid warning when we modify it here
        memcpy((void*)pShadowTrav->pNext, pTrav, sTypeStructSize(pTrav->sType));
        pShadowTrav = (GENERIC_HEADER*)pShadowTrav->pNext;
        // Special copy of Vtx info as it has embedded array
        if (XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO == pTrav->sType) {
            XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pVICI = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pShadowTrav;
            pPipeline->vtxBindingCount = pVICI->bindingCount;
            uint32_t allocSize = pPipeline->vtxBindingCount * sizeof(XGL_VERTEX_INPUT_BINDING_DESCRIPTION);
            if (allocSize) {
                pPipeline->pVertexBindingDescriptions = (XGL_VERTEX_INPUT_BINDING_DESCRIPTION*)malloc(allocSize);
                memcpy(pPipeline->pVertexBindingDescriptions, ((XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pTrav)->pVertexAttributeDescriptions, allocSize);
            }
            pPipeline->vtxAttributeCount = pVICI->attributeCount;
            allocSize = pPipeline->vtxAttributeCount * sizeof(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION);
            if (allocSize) {
                pPipeline->pVertexAttributeDescriptions = (XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION*)malloc(allocSize);
                memcpy(pPipeline->pVertexAttributeDescriptions, ((XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pTrav)->pVertexAttributeDescriptions, allocSize);
            }
        }
        else if (XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO == pTrav->sType) {
            // Special copy of CB state as it has embedded array
            XGL_PIPELINE_CB_STATE_CREATE_INFO *pCBCI = (XGL_PIPELINE_CB_STATE_CREATE_INFO*)pShadowTrav;
            pPipeline->attachmentCount = pCBCI->attachmentCount;
            uint32_t allocSize = pPipeline->attachmentCount * sizeof(XGL_PIPELINE_CB_ATTACHMENT_STATE);
            if (allocSize) {
                pPipeline->pAttachments = (XGL_PIPELINE_CB_ATTACHMENT_STATE*)malloc(allocSize);
                XGL_PIPELINE_CB_ATTACHMENT_STATE** ppAttachments = (XGL_PIPELINE_CB_ATTACHMENT_STATE**)&pCBCI->pAttachments;
                *ppAttachments = pPipeline->pAttachments;
                memcpy(pPipeline->pAttachments, ((XGL_PIPELINE_CB_STATE_CREATE_INFO*)pTrav)->pAttachments, allocSize);
            }
        }
        pTrav = (GENERIC_HEADER*)pTrav->pNext;
    }
}
// Free the Pipeline nodes
static void freePipelines()
{
    PIPELINE_NODE* pPipeline = g_pPipelineHead;
    PIPELINE_NODE* pFreeMe = pPipeline;
    while (pPipeline) {
        pFreeMe = pPipeline;
        GENERIC_HEADER* pShadowTrav = (GENERIC_HEADER*)pPipeline->pCreateTree;
        GENERIC_HEADER* pShadowFree = pShadowTrav;
        while (pShadowTrav) {
            pShadowFree = pShadowTrav;
            pShadowTrav = (GENERIC_HEADER*)pShadowTrav->pNext;
            if (XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO == pShadowFree->sType) {
                // Free the vtx data shadowed directly into pPipeline node
                if (pFreeMe->pVertexBindingDescriptions)
                    free(pFreeMe->pVertexBindingDescriptions);
                if (pFreeMe->pVertexAttributeDescriptions)
                    free(pFreeMe->pVertexAttributeDescriptions);
            }
            else if (XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO == pShadowFree->sType) {
                // Free attachment data shadowed into pPipeline node
                if (pFreeMe->pAttachments)
                    free(pFreeMe->pAttachments);
            }
            free(pShadowFree);
        }
        pPipeline = pPipeline->pNext;
        free(pFreeMe);
    }
}
// Block of code at start here specifically for managing/tracking DSs

// ptr to HEAD of LL of DS Regions
static REGION_NODE* g_pRegionHead = NULL;
// ptr to HEAD of LL of top-level Layouts
static LAYOUT_NODE* g_pLayoutHead = NULL;

// Return Region node ptr for specified region or else NULL
static REGION_NODE* getRegionNode(XGL_DESCRIPTOR_REGION region)
{
    loader_platform_thread_lock_mutex(&globalLock);
    REGION_NODE* pTrav = g_pRegionHead;
    while (pTrav) {
        if (pTrav->region == region) {
            loader_platform_thread_unlock_mutex(&globalLock);
            return pTrav;
        }
        pTrav = pTrav->pNext;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return NULL;
}
// Return Set node ptr for specified set or else NULL
static SET_NODE* getSetNode(XGL_DESCRIPTOR_SET set)
{
    loader_platform_thread_lock_mutex(&globalLock);
    REGION_NODE* pTrav = g_pRegionHead;
    while (pTrav) {
        SET_NODE* pSet = pTrav->pSets;
        while (pSet) {
            if (pSet->set == set) {
                loader_platform_thread_unlock_mutex(&globalLock);
                return pSet;
            }
            pSet = pSet->pNext;
        }
        pTrav = pTrav->pNext;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return NULL;
}

// Return XGL_TRUE if DS Exists and is within an xglBeginDescriptorRegionUpdate() call sequence, otherwise XGL_FALSE
static bool32_t dsUpdateActive(XGL_DESCRIPTOR_SET ds)
{
    SET_NODE* pTrav = getSetNode(ds);
    if (pTrav) {
        REGION_NODE* pRegion = NULL;
        pRegion = getRegionNode(pTrav->region);
        if (pRegion) {
            return pRegion->updateActive;
        }
    }
    return XGL_FALSE;
}

static LAYOUT_NODE* getLayoutNode(XGL_DESCRIPTOR_SET_LAYOUT layout) {
    loader_platform_thread_lock_mutex(&globalLock);
    LAYOUT_NODE* pTrav = g_pLayoutHead;
    while (pTrav) {
        if (pTrav->layout == layout) {
            loader_platform_thread_unlock_mutex(&globalLock);
            return pTrav;
        }
        pTrav = pTrav->pNext;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return NULL;
}

static uint32_t getUpdateIndex(GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            return ((XGL_UPDATE_SAMPLERS*)pUpdateStruct)->index;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            return ((XGL_UPDATE_SAMPLER_TEXTURES*)pUpdateStruct)->index;
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            return ((XGL_UPDATE_IMAGES*)pUpdateStruct)->index;
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            return ((XGL_UPDATE_BUFFERS*)pUpdateStruct)->index;
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            return ((XGL_UPDATE_AS_COPY*)pUpdateStruct)->descriptorIndex;
        default:
            // TODO : Flag specific error for this case
            return 0;
    }
}

static uint32_t getUpdateUpperBound(GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            return (((XGL_UPDATE_SAMPLERS*)pUpdateStruct)->count + ((XGL_UPDATE_SAMPLERS*)pUpdateStruct)->index) - 1;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            return (((XGL_UPDATE_SAMPLER_TEXTURES*)pUpdateStruct)->count + ((XGL_UPDATE_SAMPLER_TEXTURES*)pUpdateStruct)->index) - 1;
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            return (((XGL_UPDATE_IMAGES*)pUpdateStruct)->count + ((XGL_UPDATE_IMAGES*)pUpdateStruct)->index) - 1;
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            return (((XGL_UPDATE_BUFFERS*)pUpdateStruct)->count + ((XGL_UPDATE_BUFFERS*)pUpdateStruct)->index) - 1;
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            // TODO : Need to understand this case better and make sure code is correct
            return (((XGL_UPDATE_AS_COPY*)pUpdateStruct)->count + ((XGL_UPDATE_AS_COPY*)pUpdateStruct)->descriptorIndex) - 1;
        default:
            // TODO : Flag specific error for this case
            return 0;
    }
}

// Verify that the descriptor type in the update struct matches what's expected by the layout
static bool32_t validateUpdateType(GENERIC_HEADER* pUpdateStruct, const LAYOUT_NODE* pLayout)//XGL_DESCRIPTOR_TYPE type)
{
    // First get actual type of update
    XGL_DESCRIPTOR_TYPE actualType;
    uint32_t i = 0;
    uint32_t bound = getUpdateUpperBound(pUpdateStruct);
    switch (pUpdateStruct->sType)
    {
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            actualType = XGL_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            actualType = XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE;
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            actualType = ((XGL_UPDATE_IMAGES*)pUpdateStruct)->descriptorType;
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            actualType = ((XGL_UPDATE_BUFFERS*)pUpdateStruct)->descriptorType;
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            actualType = ((XGL_UPDATE_AS_COPY*)pUpdateStruct)->descriptorType;
            break;
        default:
            // TODO : Flag specific error for this case
            return 0;
    }
    for (i = ((XGL_UPDATE_SAMPLERS*)pUpdateStruct)->index; i < bound; i++) {
        if (pLayout->pTypes[i] != actualType)
            return 0;
    }
    return 1;
}

// Verify that update region for this update does not exceed max layout index for this type
static bool32_t validateUpdateSize(GENERIC_HEADER* pUpdateStruct, uint32_t layoutIdx)
{
    if (getUpdateUpperBound(pUpdateStruct) > layoutIdx)
        return 0;
    return 1;
}

// Determine the update type, allocate a new struct of that type, shadow the given pUpdate
//   struct into the new struct and return ptr to shadow struct cast as GENERIC_HEADER
static GENERIC_HEADER* shadowUpdateNode(GENERIC_HEADER* pUpdate)
{
    GENERIC_HEADER* pNewNode = NULL;
    size_t array_size = 0;
    size_t base_array_size = 0;
    size_t total_array_size = 0;
    XGL_UPDATE_BUFFERS* pUBCI;
    switch (pUpdate->sType)
    {
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            pNewNode = (GENERIC_HEADER*)malloc(sizeof(XGL_UPDATE_SAMPLERS));
            memcpy(pNewNode, pUpdate, sizeof(XGL_UPDATE_SAMPLERS));
            array_size = sizeof(XGL_SAMPLER) * ((XGL_UPDATE_SAMPLERS*)pNewNode)->count;
            ((XGL_UPDATE_SAMPLERS*)pNewNode)->pSamplers = (XGL_SAMPLER*)malloc(array_size);
            memcpy((XGL_SAMPLER*)((XGL_UPDATE_SAMPLERS*)pNewNode)->pSamplers, ((XGL_UPDATE_SAMPLERS*)pUpdate)->pSamplers, array_size);
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            pNewNode = (GENERIC_HEADER*)malloc(sizeof(XGL_UPDATE_SAMPLER_TEXTURES));
            memcpy(pNewNode, pUpdate, sizeof(XGL_UPDATE_SAMPLER_TEXTURES));
            array_size = sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO) * ((XGL_UPDATE_SAMPLER_TEXTURES*)pNewNode)->count;
            ((XGL_UPDATE_SAMPLER_TEXTURES*)pNewNode)->pSamplerImageViews = (XGL_SAMPLER_IMAGE_VIEW_INFO*)malloc(array_size);
            for (uint32_t i = 0; i < ((XGL_UPDATE_SAMPLER_TEXTURES*)pNewNode)->count; i++) {
                memcpy((XGL_SAMPLER_IMAGE_VIEW_INFO*)&((XGL_UPDATE_SAMPLER_TEXTURES*)pNewNode)->pSamplerImageViews[i], &((XGL_UPDATE_SAMPLER_TEXTURES*)pUpdate)->pSamplerImageViews[i], sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO));
                ((XGL_SAMPLER_IMAGE_VIEW_INFO*)((XGL_UPDATE_SAMPLER_TEXTURES*)pNewNode)->pSamplerImageViews)[i].pImageView = malloc(sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));
                memcpy((XGL_IMAGE_VIEW_ATTACH_INFO*)((XGL_UPDATE_SAMPLER_TEXTURES*)pNewNode)->pSamplerImageViews[i].pImageView, ((XGL_UPDATE_SAMPLER_TEXTURES*)pUpdate)->pSamplerImageViews[i].pImageView, sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));
            }
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            pNewNode = (GENERIC_HEADER*)malloc(sizeof(XGL_UPDATE_IMAGES));
            memcpy(pNewNode, pUpdate, sizeof(XGL_UPDATE_IMAGES));
            base_array_size = sizeof(XGL_IMAGE_VIEW_ATTACH_INFO*) * ((XGL_UPDATE_IMAGES*)pNewNode)->count;
            total_array_size = (sizeof(XGL_IMAGE_VIEW_ATTACH_INFO) * ((XGL_UPDATE_IMAGES*)pNewNode)->count) + base_array_size;
            XGL_IMAGE_VIEW_ATTACH_INFO*** pppLocalImageViews = (XGL_IMAGE_VIEW_ATTACH_INFO***)&((XGL_UPDATE_IMAGES*)pNewNode)->pImageViews;
            *pppLocalImageViews = (XGL_IMAGE_VIEW_ATTACH_INFO**)malloc(total_array_size);
            for (uint32_t i = 0; i < ((XGL_UPDATE_IMAGES*)pNewNode)->count; i++) {
                *pppLocalImageViews[i] = (XGL_IMAGE_VIEW_ATTACH_INFO*)(*pppLocalImageViews + base_array_size + (i * sizeof(XGL_IMAGE_VIEW_ATTACH_INFO)));
                memcpy(*pppLocalImageViews[i], ((XGL_UPDATE_IMAGES*)pUpdate)->pImageViews[i], sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));
            }
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            pUBCI = (XGL_UPDATE_BUFFERS*)pUpdate;
            pNewNode = (GENERIC_HEADER*)malloc(sizeof(XGL_UPDATE_BUFFERS));
            memcpy(pNewNode, pUpdate, sizeof(XGL_UPDATE_BUFFERS));
            base_array_size = sizeof(XGL_BUFFER_VIEW_ATTACH_INFO*) * pUBCI->count;
            total_array_size = (sizeof(XGL_BUFFER_VIEW_ATTACH_INFO) * pUBCI->count) + base_array_size;
            XGL_BUFFER_VIEW_ATTACH_INFO*** pppLocalBufferViews = (XGL_BUFFER_VIEW_ATTACH_INFO***)&((XGL_UPDATE_BUFFERS*)pNewNode)->pBufferViews;
            *pppLocalBufferViews = (XGL_BUFFER_VIEW_ATTACH_INFO**)malloc(total_array_size);
            for (uint32_t i = 0; i < pUBCI->count; i++) {
                // Set ptr and then copy data into that ptr
                *pppLocalBufferViews[i] = (XGL_BUFFER_VIEW_ATTACH_INFO*)(*pppLocalBufferViews + base_array_size + (sizeof(XGL_BUFFER_VIEW_ATTACH_INFO) * i));
                memcpy(*pppLocalBufferViews[i], pUBCI->pBufferViews[i], sizeof(XGL_BUFFER_VIEW_ATTACH_INFO));
            }
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            pNewNode = (GENERIC_HEADER*)malloc(sizeof(XGL_UPDATE_AS_COPY));
            memcpy(pNewNode, pUpdate, sizeof(XGL_UPDATE_AS_COPY));
            break;
        default:
            // TODO : Flag specific error for this case
            return NULL;
    }
    // Make sure that pNext for the end of shadow copy is NULL
    pNewNode->pNext = NULL;
    return pNewNode;
}
// For given ds, update it's mapping based on pUpdateChain linked-list
static void dsUpdate(XGL_DESCRIPTOR_SET ds, GENERIC_HEADER* pUpdateChain)
{
    SET_NODE* pSet = getSetNode(ds);
    LAYOUT_NODE* pLayout = NULL;
    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pLayoutCI;
    // TODO : If pCIList is NULL, flag error
    GENERIC_HEADER* pUpdates = pUpdateChain;
    // Perform all updates
    while (pUpdates) {
        pLayout = pSet->pLayouts;
        // For each update first find the layout section that it overlaps
        while (pLayout && (pLayout->startIndex > getUpdateIndex(pUpdates))) {
            pLayout = pLayout->pPriorSetLayout;
        }
        if (!pLayout) {
            char str[1024];
            sprintf(str, "Descriptor Set %p does not have index to match update index %u for update type %s!", ds, getUpdateIndex(pUpdates), string_XGL_STRUCTURE_TYPE(pUpdates->sType));
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS", str);
        }
        else {
            // Next verify that update is correct size
            if (!validateUpdateSize(pUpdates, pLayout->endIndex)) {
                char str[48*1024]; // TODO : Keep count of layout CI structs and size this string dynamically based on that count
                char* pDSstr = xgl_print_xgl_descriptor_set_layout_create_info(pLayoutCI, "{DS}    ");
                pLayoutCI = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pLayout->pCreateInfoList;
                sprintf(str, "Descriptor update type of %s is out of bounds for matching layout w/ CI:\n%s!", string_XGL_STRUCTURE_TYPE(pUpdates->sType), pDSstr);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS", str);
                free(pDSstr);
            }
            else { // TODO : should we skip update on a type mismatch or force it?
                // We have the right layout section, now verify that update is of the right type
                if (!validateUpdateType(pUpdates, pLayout)) {
                    char str[1024];
                    sprintf(str, "Descriptor update type of %s does not match overlapping layout type!", string_XGL_STRUCTURE_TYPE(pUpdates->sType));
                    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS", str);
                }
                else {
                    // Finally perform the update
                    // TODO : Info message that update successful
                    GENERIC_HEADER* pUpdateInsert = pSet->pUpdateStructs;
                    GENERIC_HEADER* pPrev = pUpdateInsert;
                    // Create new update struct for this set's shadow copy
                    GENERIC_HEADER* pNewNode = shadowUpdateNode(pUpdates);
                    if (NULL == pNewNode) {
                        char str[1024];
                        sprintf(str, "Out of memory while attempting to allocate UPDATE struct in xglUpdateDescriptors()");
                        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
                    }
                    else {
                        if (!pUpdateInsert) {
                            pSet->pUpdateStructs = pNewNode;
                        }
                        else {
                            // Find either the existing, matching region, or end of list for initial update chain
                            // TODO : Need to validate this, I suspect there are holes in this algorithm
                            uint32_t totalIndex = 0;
                            while (pUpdateInsert && (getUpdateIndex(pUpdates) != totalIndex)) {
                                totalIndex = getUpdateUpperBound(pUpdates);
                                pPrev = pUpdateInsert;
                                pUpdateInsert = (GENERIC_HEADER*)pUpdateInsert->pNext;
                            }
                            pPrev->pNext = pNewNode;
                        }
                    }
                }
            }
        }
        pUpdates = (GENERIC_HEADER*)pUpdates->pNext;
    }
}
// Free a shadowed update node
static void freeShadowUpdateTree(GENERIC_HEADER* pUpdate)
{
    GENERIC_HEADER* pShadowUpdate = pUpdate;
    GENERIC_HEADER* pFreeUpdate = pShadowUpdate;
    while(pShadowUpdate) {
        pFreeUpdate = pShadowUpdate;
        pShadowUpdate = (GENERIC_HEADER*)pShadowUpdate->pNext;
        uint32_t index = 0;
        XGL_UPDATE_SAMPLERS* pUS = NULL;
        XGL_UPDATE_SAMPLER_TEXTURES* pUST = NULL;
        XGL_UPDATE_IMAGES* pUI = NULL;
        XGL_UPDATE_BUFFERS* pUB = NULL;
        void** ppToFree = NULL;
        switch (pFreeUpdate->sType)
        {
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                pUS = (XGL_UPDATE_SAMPLERS*)pFreeUpdate;
                if (pUS->pSamplers) {
                    ppToFree = (void**)&pUS->pSamplers;
                    free(*ppToFree);
                }
                break;
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                pUST = (XGL_UPDATE_SAMPLER_TEXTURES*)pFreeUpdate;
                for (index = 0; index < pUST->count; index++) {
                    if (pUST->pSamplerImageViews[index].pImageView) {
                        ppToFree = (void**)&pUST->pSamplerImageViews[index].pImageView;
                        free(*ppToFree);
                    }
                }
                ppToFree = (void**)&pUST->pSamplerImageViews;
                free(*ppToFree);
                break;
            case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
                pUI = (XGL_UPDATE_IMAGES*)pFreeUpdate;
                if (pUI->pImageViews) {
                    ppToFree = (void**)&pUI->pImageViews;
                    free(*ppToFree);
                }
                break;
            case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
                pUB = (XGL_UPDATE_BUFFERS*)pFreeUpdate;
                if (pUB->pBufferViews) {
                    ppToFree = (void**)&pUB->pBufferViews;
                    free(*ppToFree);
                }
                break;
            case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
                break;
            default:
                assert(0);
                break;
        }
        free(pFreeUpdate);
    }
}
// Free all DS Regions including their Sets & related sub-structs
static void freeRegions()
{
    REGION_NODE* pRegion = g_pRegionHead;
    REGION_NODE* pFreeMe = pRegion;
    while (pRegion) {
        pFreeMe = pRegion;
        SET_NODE* pSet = pRegion->pSets;
        SET_NODE* pFreeSet = pSet;
        while (pSet) {
            pFreeSet = pSet;
            pSet = pSet->pNext;
            // Freeing layouts handled in freeLayouts() function
            // Free Update shadow struct tree
            freeShadowUpdateTree(pFreeSet->pUpdateStructs);
            free(pFreeSet);
        }
        pRegion = pRegion->pNext;
        if (pFreeMe->createInfo.pTypeCount) {
            void** ppToFree = (void**)&pFreeMe->createInfo.pTypeCount;
            free(*ppToFree);
        }
        free(pFreeMe);
    }
}
// WARN : Once freeLayouts() called, any layout ptrs in Region/Set data structure will be invalid
static void freeLayouts()
{
    LAYOUT_NODE* pLayout = g_pLayoutHead;
    LAYOUT_NODE* pFreeLayout = pLayout;
    while (pLayout) {
        pFreeLayout = pLayout;
        GENERIC_HEADER* pTrav = (GENERIC_HEADER*)pLayout->pCreateInfoList;
        while (pTrav) {
            void* pToFree = (void*)pTrav;
            pTrav = (GENERIC_HEADER*)pTrav->pNext;
            free(pToFree);
        }
        pLayout = pLayout->pNext;
        free(pFreeLayout);
    }
}
// Currently clearing a set is removing all previous updates to that set
//  TODO : Validate if this is correct clearing behavior
static void clearDescriptorSet(XGL_DESCRIPTOR_SET set)
{
    SET_NODE* pSet = getSetNode(set);
    if (!pSet) {
        // TODO : Return error
    }
    else {
        freeShadowUpdateTree(pSet->pUpdateStructs);
    }
}

static void clearDescriptorRegion(XGL_DESCRIPTOR_REGION region)
{
    REGION_NODE* pRegion = getRegionNode(region);
    if (!pRegion) {
        char str[1024];
        sprintf(str, "Unable to find region node for region %p specified in xglClearDescriptorRegion() call", (void*)region);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, region, 0, DRAWSTATE_INVALID_REGION, "DS", str);
    }
    else
    {
        // For every set off of this region, clear it
        SET_NODE* pSet = pRegion->pSets;
        while (pSet) {
            clearDescriptorSet(pSet->set);
        }
    }
}

// Code here to manage the Cmd buffer LL
static GLOBAL_CB_NODE* getCBNode(XGL_CMD_BUFFER cb)
{
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = g_pCmdBufferHead;
    while (pCB) {
        if (cb == pCB->cmdBuffer) {
            loader_platform_thread_unlock_mutex(&globalLock);
            return pCB;
        }
        pCB = pCB->pNextGlobalCBNode;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return NULL;
}
// Free all CB Nodes
static void freeCmdBuffers()
{
    GLOBAL_CB_NODE* pCB = g_pCmdBufferHead;
    GLOBAL_CB_NODE* pFreeMe = pCB;
    while (pCB) {
        pFreeMe = pCB;
        CMD_NODE* pCmd = pCB->pCmds;
        CMD_NODE* pFreeCmd = pCmd;
        while (pCmd) {
            pFreeCmd = pCmd;
            pCmd = pCmd->pNext;
            free(pFreeCmd);
        }
        pCB = pCB->pNextGlobalCBNode;
        free(pFreeMe);
    }
}
static void addCmd(GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd)
{
    CMD_NODE* pCmd = (CMD_NODE*)malloc(sizeof(CMD_NODE));
    if (pCmd) {
        // init cmd node and append to end of cmd LL
        memset(pCmd, 0, sizeof(CMD_NODE));
        pCB->numCmds++;
        pCmd->cmdNumber = pCB->numCmds;
        pCmd->type = cmd;
        if (!pCB->pCmds) {
            pCB->pCmds = pCmd;
        }
        else {
            assert(pCB->lastCmd);
            pCB->lastCmd->pNext = pCmd;
        }
        pCB->lastCmd = pCmd;
    }
    else {
        char str[1024];
        sprintf(str, "Out of memory while attempting to allocate new CMD_NODE for cmdBuffer %p", (void*)pCB->cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pCB->cmdBuffer, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
    }
}
static void resetCB(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        CMD_NODE* pCur = pCB->pCmds;
        CMD_NODE* pFreeMe = pCur;
        while (pCur) {
            pFreeMe = pCur;
            pCur = pCur->pNext;
            free(pFreeMe);
        }
        // Reset CB state
        GLOBAL_CB_NODE* pSaveNext = pCB->pNextGlobalCBNode;
        XGL_FLAGS saveFlags = pCB->flags;
        XGL_QUEUE_TYPE saveQT = pCB->queueType;
        memset(pCB, 0, sizeof(GLOBAL_CB_NODE));
        pCB->cmdBuffer = cb;
        pCB->flags = saveFlags;
        pCB->queueType = saveQT;
        pCB->pNextGlobalCBNode = pSaveNext;
        pCB->lastVtxBinding = MAX_BINDING;
    }
}
// Set the last bound dynamic state of given type
// TODO : Need to track this per cmdBuffer and correlate cmdBuffer for Draw w/ last bound for that cmdBuffer?
static void setLastBoundDynamicState(const XGL_CMD_BUFFER cmdBuffer, const XGL_DYNAMIC_STATE_OBJECT state, const XGL_STATE_BIND_POINT sType)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_BINDDYNAMICSTATEOBJECT);
        loader_platform_thread_lock_mutex(&globalLock);
        DYNAMIC_STATE_NODE* pTrav = g_pDynamicStateHead[sType];
        while (pTrav && (state != pTrav->stateObj)) {
            pTrav = pTrav->pNext;
        }
        if (!pTrav) {
            char str[1024];
            sprintf(str, "Unable to find dynamic state object %p, was it ever created?", (void*)state);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, state, 0, DRAWSTATE_INVALID_DYNAMIC_STATE_OBJECT, "DS", str);
        }
        pCB->lastBoundDynamicState[sType] = pTrav;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
}
// Print the last bound Gfx Pipeline
static void printPipeline(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        if (!pPipeTrav) {
            // nothing to print
        }
        else {
            char* pipeStr = xgl_print_xgl_graphics_pipeline_create_info(pPipeTrav->pCreateTree, "{DS}");
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", pipeStr);
            free(pipeStr);
        }
    }
}
// Dump subgraph w/ DS info
static void dsDumpDot(const XGL_CMD_BUFFER cb, FILE* pOutFile)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB && pCB->lastBoundDescriptorSet) {
        SET_NODE* pSet = getSetNode(pCB->lastBoundDescriptorSet);
        REGION_NODE* pRegion = getRegionNode(pSet->region);
        char tmp_str[1024];
        fprintf(pOutFile, "subgraph cluster_DescriptorRegion\n{\nlabel=\"Descriptor Region\"\n");
        sprintf(tmp_str, "Region (%p)", pRegion->region);
        char* pGVstr = xgl_gv_print_xgl_descriptor_region_create_info(&pRegion->createInfo, tmp_str);
        fprintf(pOutFile, "%s", pGVstr);
        free(pGVstr);
        fprintf(pOutFile, "subgraph cluster_DescriptorSet\n{\nlabel=\"Descriptor Set (%p)\"\n", pSet->set);
        sprintf(tmp_str, "Descriptor Set (%p)", pSet->set);
        LAYOUT_NODE* pLayout = pSet->pLayouts;
        uint32_t layout_index = 0;
        while (pLayout) {
            ++layout_index;
            sprintf(tmp_str, "LAYOUT%u", layout_index);
            pGVstr = xgl_gv_print_xgl_descriptor_set_layout_create_info(pLayout->pCreateInfoList, tmp_str);
            fprintf(pOutFile, "%s", pGVstr);
            free(pGVstr);
            pLayout = pLayout->pNext;
            if (pLayout) {
                fprintf(pOutFile, "\"%s\" -> \"LAYOUT%u\" [];\n", tmp_str, layout_index+1);
            }
        }
        if (pSet->pUpdateStructs) {
            pGVstr = dynamic_gv_display(pSet->pUpdateStructs, "Descriptor Updates");
            fprintf(pOutFile, "%s", pGVstr);
            free(pGVstr);
        }
        fprintf(pOutFile, "}\n");
        fprintf(pOutFile, "}\n");
        pRegion = pRegion->pNext;
    }
}

// Dump a GraphViz dot file showing the pipeline
static void dumpDotFile(const XGL_CMD_BUFFER cb, char *outFileName)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        if (pPipeTrav) {
            FILE* pOutFile;
            pOutFile = fopen(outFileName, "w");
            fprintf(pOutFile, "digraph g {\ngraph [\nrankdir = \"TB\"\n];\nnode [\nfontsize = \"16\"\nshape = \"plaintext\"\n];\nedge [\n];\n");
            fprintf(pOutFile, "subgraph cluster_dynamicState\n{\nlabel=\"Dynamic State\"\n");
            char* pGVstr = NULL;
            for (uint32_t i = 0; i < XGL_NUM_STATE_BIND_POINT; i++) {
                if (pCB->lastBoundDynamicState[i]) {
                    pGVstr = dynamic_gv_display(pCB->lastBoundDynamicState[i]->pCreateInfo, string_XGL_STATE_BIND_POINT(i));
                    fprintf(pOutFile, "%s", pGVstr);
                    free(pGVstr);
                }
            }
            fprintf(pOutFile, "}\n"); // close dynamicState subgraph
            fprintf(pOutFile, "subgraph cluster_PipelineStateObject\n{\nlabel=\"Pipeline State Object\"\n");
            pGVstr = xgl_gv_print_xgl_graphics_pipeline_create_info(pPipeTrav->pCreateTree, "PSO HEAD");
            fprintf(pOutFile, "%s", pGVstr);
            free(pGVstr);
            fprintf(pOutFile, "}\n");
            dsDumpDot(cb, pOutFile);
            fprintf(pOutFile, "}\n"); // close main graph "g"
            fclose(pOutFile);
        }
    }
}
// Synch up currently bound pipeline settings with DS mappings
// TODO : Update name. We don't really have to "synch" the descriptors anymore and "mapping" is outdated as well.
static void synchDSMapping(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB && pCB->lastBoundPipeline) {
        // First verify that we have a Node for bound pipeline
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        char str[1024];
        if (!pPipeTrav) {
            sprintf(str, "Can't find last bound Pipeline %p!", (void*)pCB->lastBoundPipeline);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NO_PIPELINE_BOUND, "DS", str);
        }
        else {
            // Verify Vtx binding
            if (MAX_BINDING != pCB->lastVtxBinding) {
                if (pCB->lastVtxBinding >= pPipeTrav->vtxBindingCount) {
                    sprintf(str, "Vtx binding Index of %u exceeds PSO pVertexBindingDescriptions max array index of %u.", pCB->lastVtxBinding, (pPipeTrav->vtxBindingCount - 1));
                    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS", str);
                }
                else {
                    char *tmpStr = xgl_print_xgl_vertex_input_binding_description(&pPipeTrav->pVertexBindingDescriptions[pCB->lastVtxBinding], "{DS}INFO : ");
                    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmpStr);
                    free(tmpStr);
                }
            }
        }
    }
}
// Print details of DS config to stdout
static void printDSConfig(const XGL_CMD_BUFFER cb)
{
    char tmp_str[1024];
    char ds_config_str[1024*256] = {0}; // TODO : Currently making this buffer HUGE w/o overrun protection.  Need to be smarter, start smaller, and grow as needed.
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        SET_NODE* pSet = getSetNode(pCB->lastBoundDescriptorSet);
        REGION_NODE* pRegion = getRegionNode(pSet->region);
        // Print out region details
        sprintf(tmp_str, "Details for region %p.", (void*)pRegion->region);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        char* pRegionStr = xgl_print_xgl_descriptor_region_create_info(&pRegion->createInfo, " ");
        sprintf(ds_config_str, "%s", pRegionStr);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", ds_config_str);
        free(pRegionStr);
        // Print out set details
        char prefix[10];
        uint32_t index = 0;
        sprintf(tmp_str, "Details for descriptor set %p.", (void*)pSet->set);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        LAYOUT_NODE* pLayout = pSet->pLayouts;
        while (pLayout) {
            // Print layout details
            sprintf(tmp_str, "Layout #%u, (object %p) for DS %p.", index+1, (void*)pLayout->layout, (void*)pSet->set);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
            sprintf(prefix, "  [L%u] ", index);
            char* pDSLstr = xgl_print_xgl_descriptor_set_layout_create_info(&pLayout->pCreateInfoList[0], prefix);
            sprintf(ds_config_str, "%s", pDSLstr);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", ds_config_str);
            free(pDSLstr);
            pLayout = pLayout->pPriorSetLayout;
            index++;
        }
        GENERIC_HEADER* pUpdate = pSet->pUpdateStructs;
        if (pUpdate) {
            sprintf(tmp_str, "Update Chain [UC] for descriptor set %p:", (void*)pSet->set);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
            sprintf(prefix, "  [UC] ");
            sprintf(ds_config_str, "%s", dynamic_display(pUpdate, prefix));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", ds_config_str);
            // TODO : If there is a "view" associated with this update, print CI for that view
        }
        else {
            sprintf(tmp_str, "No Update Chain for descriptor set %p (xglUpdateDescriptors has not been called)", (void*)pSet->set);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        }
    }
}

static void printCB(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        char str[1024];
        CMD_NODE* pCmd = pCB->pCmds;
        sprintf(str, "Cmds in CB %p", (void*)cb);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", str);
        while (pCmd) {
            sprintf(str, "  CMD#%lu: %s", pCmd->cmdNumber, cmdTypeToString(pCmd->type));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, DRAWSTATE_NONE, "DS", str);
            pCmd = pCmd->pNext;
        }
    }
    else {
        // Nothing to print
    }
}

static void synchAndDumpDot(const XGL_CMD_BUFFER cb)
{
    synchDSMapping(cb);
    dumpDotFile(cb, "pipeline_dump.dot");
}

static void synchAndPrintDSConfig(const XGL_CMD_BUFFER cb)
{
    synchDSMapping(cb);
    printDSConfig(cb);
    printPipeline(cb);
    printDynamicState(cb);
    static int autoDumpOnce = 1;
    if (autoDumpOnce) {
        autoDumpOnce = 0;
        dumpDotFile(cb, "pipeline_dump.dot");
        // Convert dot to png if dot available
#if defined(_WIN32)
// FIXME: NEED WINDOWS EQUIVALENT
#else // WIN32
        if(access( "/usr/bin/dot", X_OK) != -1) {
            system("/usr/bin/dot pipeline_dump.dot -Tpng -o pipeline_dump.png");
        }
#endif // WIN32
    }
}

static void initDrawState(void)
{
    const char *strOpt;
    // initialize DrawState options
    strOpt = getLayerOption("DrawStateReportLevel");
    if (strOpt != NULL)
        g_reportingLevel = atoi(strOpt);

    strOpt = getLayerOption("DrawStateDebugAction");
    if (strOpt != NULL)
        g_debugAction = atoi(strOpt);

    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption("DrawStateLogFilename");
        if (strOpt)
        {
            g_logFile = fopen(strOpt, "w");
        }
        if (g_logFile == NULL)
            g_logFile = stdout;
    }
    // initialize Layer dispatch table
    // TODO handle multiple GPUs
    xglGetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (XGL_PHYSICAL_GPU) pCurObj->nextObject);

    xglGetProcAddrType fpGetProcAddr = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (char *) "xglGetProcAddr");
    nextTable.GetProcAddr = fpGetProcAddr;

    if (!globalLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during xglCreateInstance(), and then we
        // can clean it up during xglDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateInstance(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_INSTANCE* pInstance)
{
    XGL_RESULT result = nextTable.CreateInstance(pAppInfo, pAllocCb, pInstance);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyInstance(XGL_INSTANCE instance)
{
    XGL_RESULT result = nextTable.DestroyInstance(instance);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateGpus(XGL_INSTANCE instance, uint32_t maxGpus, uint32_t* pGpuCount, XGL_PHYSICAL_GPU* pGpus)
{
    XGL_RESULT result = nextTable.EnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyDevice(XGL_DEVICE device)
{
    // Free all the memory
    loader_platform_thread_lock_mutex(&globalLock);
    freePipelines();
    freeSamplers();
    freeImages();
    freeBuffers();
    freeCmdBuffers();
    freeDynamicState();
    freeRegions();
    freeLayouts();
    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.DestroyDevice(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const char* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    if (gpu != NULL)
    {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        pCurObj = gpuw;
        loader_platform_thread_once(&g_initOnce, initDrawState);
        XGL_RESULT result = nextTable.EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        return result;
    } else
    {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return XGL_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pOutLayerCount = 1;
        strncpy((char *) pOutLayers[0], "DrawState", maxStringSize);
        return XGL_SUCCESS;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(XGL_DEVICE device, XGL_QUEUE_TYPE queueType, uint32_t queueIndex, XGL_QUEUE* pQueue)
{
    XGL_RESULT result = nextTable.GetDeviceQueue(device, queueType, queueIndex, pQueue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, uint32_t cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
{
    for (uint32_t i=0; i < cmdBufferCount; i++) {
        // Validate that cmd buffers have been updated
    }
    XGL_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(XGL_QUEUE queue, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs)
{
    XGL_RESULT result = nextTable.QueueSetGlobalMemReferences(queue, memRefCount, pMemRefs);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueWaitIdle(XGL_QUEUE queue)
{
    XGL_RESULT result = nextTable.QueueWaitIdle(queue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDeviceWaitIdle(XGL_DEVICE device)
{
    XGL_RESULT result = nextTable.DeviceWaitIdle(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocMemory(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglFreeMemory(XGL_GPU_MEMORY mem)
{
    XGL_RESULT result = nextTable.FreeMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetMemoryPriority(XGL_GPU_MEMORY mem, XGL_MEMORY_PRIORITY priority)
{
    XGL_RESULT result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglMapMemory(XGL_GPU_MEMORY mem, XGL_FLAGS flags, void** ppData)
{
    XGL_RESULT result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(XGL_GPU_MEMORY mem)
{
    XGL_RESULT result = nextTable.UnmapMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(XGL_DEVICE device, const void* pSysMem, size_t memSize, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu0;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.GetMultiGpuCompatibility((XGL_PHYSICAL_GPU)gpuw->nextObject, gpu1, pInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    XGL_RESULT result = nextTable.OpenSharedQueueSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerMemory(XGL_DEVICE device, const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerImage(XGL_DEVICE device, const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyObject(XGL_OBJECT object)
{
    // TODO : When wrapped objects (such as dynamic state) are destroyed, need to clean up memory
    XGL_RESULT result = nextTable.DestroyObject(object);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    XGL_RESULT result = nextTable.BindObjectMemory(object, allocationIdx, mem, offset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemoryRange(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_SIZE rangeOffset, XGL_GPU_SIZE rangeSize, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{
    XGL_RESULT result = nextTable.BindObjectMemoryRange(object, allocationIdx, rangeOffset, rangeSize, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindImageMemoryRange(XGL_IMAGE image, uint32_t allocationIdx, const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{
    XGL_RESULT result = nextTable.BindImageMemoryRange(image, allocationIdx, bindInfo, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFence(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence)
{
    XGL_RESULT result = nextTable.CreateFence(device, pCreateInfo, pFence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(XGL_FENCE fence)
{
    XGL_RESULT result = nextTable.GetFenceStatus(fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, uint32_t fenceCount, const XGL_FENCE* pFences, bool32_t waitAll, uint64_t timeout)
{
    XGL_RESULT result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    XGL_RESULT result = nextTable.CreateQueueSemaphore(device, pCreateInfo, pSemaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSignalQueueSemaphore(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore)
{
    XGL_RESULT result = nextTable.SignalQueueSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitQueueSemaphore(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore)
{
    XGL_RESULT result = nextTable.WaitQueueSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateEvent(XGL_DEVICE device, const XGL_EVENT_CREATE_INFO* pCreateInfo, XGL_EVENT* pEvent)
{
    XGL_RESULT result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetEventStatus(XGL_EVENT event)
{
    XGL_RESULT result = nextTable.GetEventStatus(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetEvent(XGL_EVENT event)
{
    XGL_RESULT result = nextTable.SetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetEvent(XGL_EVENT event)
{
    XGL_RESULT result = nextTable.ResetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool)
{
    XGL_RESULT result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBuffer(XGL_DEVICE device, const XGL_BUFFER_CREATE_INFO* pCreateInfo, XGL_BUFFER* pBuffer)
{
    XGL_RESULT result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        BUFFER_NODE *pNewNode = (BUFFER_NODE*)malloc(sizeof(BUFFER_NODE));
        pNewNode->buffer = *pBuffer;
        memcpy(&pNewNode->createInfo, pCreateInfo, sizeof(XGL_BUFFER_CREATE_INFO));
        pNewNode->pNext = g_pBufferHead;
        g_pBufferHead = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBufferView(XGL_DEVICE device, const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo, XGL_BUFFER_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetFastClearColor(XGL_IMAGE image, const float color[4])
{
    XGL_RESULT result = nextTable.SetFastClearColor(image, color);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetFastClearDepth(XGL_IMAGE image, float depth)
{
    XGL_RESULT result = nextTable.SetFastClearDepth(image, depth);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        IMAGE_NODE *pNewNode = (IMAGE_NODE*)malloc(sizeof(IMAGE_NODE));
        pNewNode->image = *pView;
        memcpy(&pNewNode->createInfo, pCreateInfo, sizeof(XGL_IMAGE_VIEW_CREATE_INFO));
        pNewNode->pNext = g_pImageHead;
        g_pImageHead = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateShader(XGL_DEVICE device, const XGL_SHADER_CREATE_INFO* pCreateInfo, XGL_SHADER* pShader)
{
    XGL_RESULT result = nextTable.CreateShader(device, pCreateInfo, pShader);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipeline(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    // Create LL HEAD for this Pipeline
    char str[1024];
    sprintf(str, "Created Gfx Pipeline %p", (void*)*pPipeline);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pPipeline, 0, DRAWSTATE_NONE, "DS", str);
    loader_platform_thread_lock_mutex(&globalLock);
    PIPELINE_NODE *pTrav = g_pPipelineHead;
    if (pTrav) {
        while (pTrav->pNext)
            pTrav = pTrav->pNext;
        pTrav->pNext = (PIPELINE_NODE*)malloc(sizeof(PIPELINE_NODE));
        pTrav = pTrav->pNext;
    }
    else {
        pTrav = (PIPELINE_NODE*)malloc(sizeof(PIPELINE_NODE));
        g_pPipelineHead = pTrav;
    }
    memset((void*)pTrav, 0, sizeof(PIPELINE_NODE));
    pTrav->pipeline = *pPipeline;
    initPipeline(pTrav, pCreateInfo);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglStorePipeline(XGL_PIPELINE pipeline, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(XGL_DEVICE device, size_t dataSize, const void* pData, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreatePipelineDelta(XGL_DEVICE device, XGL_PIPELINE p1, XGL_PIPELINE p2, XGL_PIPELINE_DELTA* delta)
{
    XGL_RESULT result = nextTable.CreatePipelineDelta(device, p1, p2, delta);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateSampler(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler)
{
    XGL_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        SAMPLER_NODE *pNewNode = (SAMPLER_NODE*)malloc(sizeof(SAMPLER_NODE));
        pNewNode->sampler = *pSampler;
        memcpy(&pNewNode->createInfo, pCreateInfo, sizeof(XGL_SAMPLER_CREATE_INFO));
        pNewNode->pNext = g_pSamplerHead;
        g_pSamplerHead = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(XGL_DEVICE device, XGL_FLAGS stageFlags, const uint32_t* pSetBindPoints, XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout, const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList, XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout)
{
    XGL_RESULT result = nextTable.CreateDescriptorSetLayout(device, stageFlags, pSetBindPoints, priorSetLayout, pSetLayoutInfoList, pSetLayout);
    if (XGL_SUCCESS == result) {
        LAYOUT_NODE* pNewNode = (LAYOUT_NODE*)malloc(sizeof(LAYOUT_NODE));
        if (NULL == pNewNode) {
            char str[1024];
            sprintf(str, "Out of memory while attempting to allocate LAYOUT_NODE in xglCreateDescriptorSetLayout()");
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, *pSetLayout, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
        }
        memset(pNewNode, 0, sizeof(LAYOUT_NODE));
        // TODO : API Currently missing a count here that we should multiply by struct size
        pNewNode->pCreateInfoList = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)malloc(sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO));
        memset((void*)pNewNode->pCreateInfoList, 0, sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO));
        void* pCITrav = NULL;
        uint32_t totalCount = 0;
        if (pSetLayoutInfoList) {
            memcpy((void*)pNewNode->pCreateInfoList, pSetLayoutInfoList, sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO));
            pCITrav = (void*)pSetLayoutInfoList->pNext;
            totalCount = pSetLayoutInfoList->count;
        }
        void** ppNext = (void**)&pNewNode->pCreateInfoList->pNext;
        while (pCITrav) {
            totalCount += ((XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pCITrav)->count;
            *ppNext = (void*)malloc(sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO));
            memcpy((void*)*ppNext, pCITrav, sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO));
            pCITrav = (void*)((XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pCITrav)->pNext;
            *ppNext = (void*)((XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)*ppNext)->pNext;
        }
        if (totalCount > 0) {
            pNewNode->pTypes = (XGL_DESCRIPTOR_TYPE*)malloc(totalCount*sizeof(XGL_DESCRIPTOR_TYPE));
            XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pLCI = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pSetLayoutInfoList;
            while (pLCI) {
                for (uint32_t i = 0; i < pLCI->count; i++) {
                    pNewNode->pTypes[i] = pLCI->descriptorType;
                }
                pLCI = (XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pLCI->pNext;
            }
        }
        pNewNode->layout = *pSetLayout;
        pNewNode->stageFlags = stageFlags;
        uint32_t i = (XGL_SHADER_STAGE_FLAGS_ALL == stageFlags) ? 0 : XGL_SHADER_STAGE_COMPUTE;
        for (uint32_t stage = XGL_SHADER_STAGE_FLAGS_COMPUTE_BIT; stage > 0; stage >>= 1) {
            assert(i < XGL_NUM_SHADER_STAGE);
            if (stage & stageFlags)
                pNewNode->shaderStageBindPoints[i] = pSetBindPoints[i];
            i = (i == 0) ? 0 : (i-1);
        }
        pNewNode->startIndex = 0;
        LAYOUT_NODE* pPriorNode = getLayoutNode(priorSetLayout);
        // Point to prior node or NULL if no prior node
        if (NULL != priorSetLayout && pPriorNode == NULL) {
            char str[1024];
            sprintf(str, "Invalid priorSetLayout of %p passed to xglCreateDescriptorSetLayout()", (void*)priorSetLayout);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, priorSetLayout, 0, DRAWSTATE_INVALID_LAYOUT, "DS", str);
        }
        else if (pPriorNode != NULL) { // We have a node for a valid prior layout
            // Get count for prior layout
            pNewNode->startIndex = pPriorNode->endIndex + 1;
        }
        pNewNode->endIndex = pNewNode->startIndex + totalCount - 1;
        assert(pNewNode->endIndex >= pNewNode->startIndex);
        pNewNode->pPriorSetLayout = pPriorNode;
        // Put new node at Head of global Layer list
        pNewNode->pNext = g_pLayoutHead;
        g_pLayoutHead = pNewNode;
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginDescriptorRegionUpdate(XGL_DEVICE device, XGL_DESCRIPTOR_UPDATE_MODE updateMode)
{
    XGL_RESULT result = nextTable.BeginDescriptorRegionUpdate(device, updateMode);
    if (XGL_SUCCESS == result) {
        if (!g_pRegionHead) {
            char str[1024];
            sprintf(str, "No descriptor region found! Global descriptor region is NULL!");
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, g_pRegionHead, 0, DRAWSTATE_NO_DS_REGION, "DS", str);
        }
        else {
            REGION_NODE* pRegionNode = g_pRegionHead;
            if (!pRegionNode) {
                char str[1024];
                sprintf(str, "Unable to find region node for global region head %p", (void*)g_pRegionHead);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, g_pRegionHead, 0, DRAWSTATE_INTERNAL_ERROR, "DS", str);
            }
            else {
                pRegionNode->updateActive = 1;
            }
        }
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndDescriptorRegionUpdate(XGL_DEVICE device, XGL_CMD_BUFFER cmd)
{
    XGL_RESULT result = nextTable.EndDescriptorRegionUpdate(device, cmd);
    if (XGL_SUCCESS == result) {
        if (!g_pRegionHead) {
            char str[1024];
            sprintf(str, "No descriptor region found! Global descriptor region is NULL!");
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, g_pRegionHead, 0, DRAWSTATE_NO_DS_REGION, "DS", str);
        }
        else {
            REGION_NODE* pRegionNode = g_pRegionHead;
            if (!pRegionNode) {
                char str[1024];
                sprintf(str, "Unable to find region node for global region head %p", (void*)g_pRegionHead);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, g_pRegionHead, 0, DRAWSTATE_INTERNAL_ERROR, "DS", str);
            }
            else {
                if (!pRegionNode->updateActive) {
                    char str[1024];
                    sprintf(str, "You must call xglBeginDescriptorRegionUpdate() before this call to xglEndDescriptorRegionUpdate()!");
                    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, g_pRegionHead, 0, DRAWSTATE_DS_END_WITHOUT_BEGIN, "DS", str);
                }
                else {
                    pRegionNode->updateActive = 0;
                }
            }
        }
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorRegion(XGL_DEVICE device, XGL_DESCRIPTOR_REGION_USAGE regionUsage, uint32_t maxSets, const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_REGION* pDescriptorRegion)
{
    XGL_RESULT result = nextTable.CreateDescriptorRegion(device, regionUsage, maxSets, pCreateInfo, pDescriptorRegion);
    if (XGL_SUCCESS == result) {
        // Insert this region into Global Region LL at head
        char str[1024];
        sprintf(str, "Created Descriptor Region %p", (void*)*pDescriptorRegion);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pDescriptorRegion, 0, DRAWSTATE_NONE, "DS", str);
        loader_platform_thread_lock_mutex(&globalLock);
        REGION_NODE* pNewNode = (REGION_NODE*)malloc(sizeof(REGION_NODE));
        if (NULL == pNewNode) {
            char str[1024];
            sprintf(str, "Out of memory while attempting to allocate REGION_NODE in xglCreateDescriptorRegion()");
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, *pDescriptorRegion, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
        }
        else {
            memset(pNewNode, 0, sizeof(REGION_NODE));
            pNewNode->pNext = g_pRegionHead;
            g_pRegionHead = pNewNode;
            XGL_DESCRIPTOR_REGION_CREATE_INFO* pCI = (XGL_DESCRIPTOR_REGION_CREATE_INFO*)&pNewNode->createInfo;
            memcpy((void*)pCI, pCreateInfo, sizeof(XGL_DESCRIPTOR_REGION_CREATE_INFO));
            size_t typeCountSize = pNewNode->createInfo.count * sizeof(XGL_DESCRIPTOR_TYPE_COUNT);
            if (typeCountSize) {
                XGL_DESCRIPTOR_TYPE_COUNT** ppTypeCount = (XGL_DESCRIPTOR_TYPE_COUNT**)&pNewNode->createInfo.pTypeCount;
                *ppTypeCount = (XGL_DESCRIPTOR_TYPE_COUNT*)malloc(typeCountSize);
                memcpy((void*)*ppTypeCount, pCreateInfo->pTypeCount, typeCountSize);
            }
            pNewNode->regionUsage  = regionUsage;
            pNewNode->updateActive = 0;
            pNewNode->maxSets      = maxSets;
            pNewNode->region       = *pDescriptorRegion;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    else {
        // Need to do anything if region create fails?
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglClearDescriptorRegion(XGL_DESCRIPTOR_REGION descriptorRegion)
{
    XGL_RESULT result = nextTable.ClearDescriptorRegion(descriptorRegion);
    if (XGL_SUCCESS == result) {
        clearDescriptorRegion(descriptorRegion);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocDescriptorSets(XGL_DESCRIPTOR_REGION descriptorRegion, XGL_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts, XGL_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount)
{
    XGL_RESULT result = nextTable.AllocDescriptorSets(descriptorRegion, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    if ((XGL_SUCCESS == result) || (*pCount > 0)) {
        REGION_NODE *pRegionNode = getRegionNode(descriptorRegion);
        if (!pRegionNode) {
            char str[1024];
            sprintf(str, "Unable to find region node for region %p specified in xglAllocDescriptorSets() call", (void*)descriptorRegion);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorRegion, 0, DRAWSTATE_INVALID_REGION, "DS", str);
        }
        else {
            for (uint32_t i = 0; i < *pCount; i++) {
                char str[1024];
                sprintf(str, "Created Descriptor Set %p", (void*)pDescriptorSets[i]);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS", str);
                // Create new set node and add to head of region nodes
                SET_NODE* pNewNode = (SET_NODE*)malloc(sizeof(SET_NODE));
                if (NULL == pNewNode) {
                    char str[1024];
                    sprintf(str, "Out of memory while attempting to allocate SET_NODE in xglAllocDescriptorSets()");
                    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
                }
                else {
                    memset(pNewNode, 0, sizeof(SET_NODE));
                    // Insert set at head of Set LL for this region
                    pNewNode->pNext = pRegionNode->pSets;
                    pRegionNode->pSets = pNewNode;
                    LAYOUT_NODE* pLayout = getLayoutNode(pSetLayouts[i]);
                    if (NULL == pLayout) {
                        char str[1024];
                        sprintf(str, "Unable to find set layout node for layout %p specified in xglAllocDescriptorSets() call", (void*)pSetLayouts[i]);
                        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pSetLayouts[i], 0, DRAWSTATE_INVALID_LAYOUT, "DS", str);
                    }
                    pNewNode->pLayouts = pLayout;
                    pNewNode->region = descriptorRegion;
                    pNewNode->set = pDescriptorSets[i];
                    pNewNode->setUsage = setUsage;
                    // TODO : Make sure to set this correctly
                    pNewNode->descriptorCount = pLayout->endIndex + 1;
                }
            }
        }
    }
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglClearDescriptorSets(XGL_DESCRIPTOR_REGION descriptorRegion, uint32_t count, const XGL_DESCRIPTOR_SET* pDescriptorSets)
{
    for (uint32_t i = 0; i < count; i++) {
        clearDescriptorSet(pDescriptorSets[i]);
    }
    nextTable.ClearDescriptorSets(descriptorRegion, count, pDescriptorSets);
}

XGL_LAYER_EXPORT void XGLAPI xglUpdateDescriptors(XGL_DESCRIPTOR_SET descriptorSet, const void* pUpdateChain)
{
    if (!dsUpdateActive(descriptorSet)) {
        char str[1024];
        sprintf(str, "You must call xglBeginDescriptorRegionUpdate() before this call to xglUpdateDescriptors()!");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, g_pRegionHead, 0, DRAWSTATE_UPDATE_WITHOUT_BEGIN, "DS", str);
    }
    else {
        // pUpdateChain is a Linked-list of XGL_UPDATE_* structures defining the mappings for the descriptors
        dsUpdate(descriptorSet, (GENERIC_HEADER*)pUpdateChain);
    }

    nextTable.UpdateDescriptors(descriptorSet, pUpdateChain);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicViewportState(XGL_DEVICE device, const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_VP_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, XGL_STATE_BIND_VIEWPORT);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicRasterState(XGL_DEVICE device, const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_RS_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, XGL_STATE_BIND_RASTER);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicColorBlendState(XGL_DEVICE device, const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_CB_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, XGL_STATE_BIND_COLOR_BLEND);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicDepthStencilState(XGL_DEVICE device, const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_DS_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, XGL_STATE_BIND_DEPTH_STENCIL);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer)
{
    XGL_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    if (XGL_SUCCESS == result) {
        GLOBAL_CB_NODE* pCB = (GLOBAL_CB_NODE*)malloc(sizeof(GLOBAL_CB_NODE));
        memset(pCB, 0, sizeof(GLOBAL_CB_NODE));
        pCB->pNextGlobalCBNode = g_pCmdBufferHead;
        g_pCmdBufferHead = pCB;
        pCB->cmdBuffer = *pCmdBuffer;
        pCB->flags = pCreateInfo->flags;
        pCB->queueType = pCreateInfo->queueType;
        pCB->lastVtxBinding = MAX_BINDING;
        g_lastCmdBuffer = *pCmdBuffer;
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    if (XGL_SUCCESS == result) {
        GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
        if (CB_NEW != pCB->state)
            resetCB(cmdBuffer);
        pCB->state = CB_UPDATE_ACTIVE;
        g_lastCmdBuffer = cmdBuffer;
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    XGL_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    if (XGL_SUCCESS == result) {
        GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
        pCB->state = CB_UPDATE_COMPLETE;
        g_lastCmdBuffer = cmdBuffer;
        printCB(cmdBuffer);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    XGL_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    if (XGL_SUCCESS == result) {
        resetCB(cmdBuffer);
        g_lastCmdBuffer = cmdBuffer;
    }
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindPipeline(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_BINDPIPELINE);
        if (getPipeline(pipeline)) {
            pCB->lastBoundPipeline = pipeline;
        }
        else {
            char str[1024];
            sprintf(str, "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pipeline, 0, DRAWSTATE_INVALID_PIPELINE, "DS", str);
        }
        synchAndDumpDot(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindPipelineDelta(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        // TODO : Handle storing Pipeline Deltas to cmd buffer here
        g_lastCmdBuffer = cmdBuffer;
        synchAndDumpDot(cmdBuffer);
        addCmd(pCB, CMD_BINDPIPELINEDELTA);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDynamicStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT state)
{
    setLastBoundDynamicState(cmdBuffer, state, stateBindPoint);
    synchAndDumpDot(cmdBuffer);
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDescriptorSet(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_DESCRIPTOR_SET descriptorSet, const uint32_t* pUserData)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_BINDDESCRIPTORSET);
        if (getSetNode(descriptorSet)) {
            if (dsUpdateActive(descriptorSet)) {
                // TODO : Not sure if it's valid to made this check here. May need to make at QueueSubmit time
                char str[1024];
                sprintf(str, "You must call xglEndDescriptorRegionUpdate(%p) before this call to xglCmdBindDescriptorSet()!", (void*)descriptorSet);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_BINDING_DS_NO_END_UPDATE, "DS", str);
            }
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->lastBoundDescriptorSet = descriptorSet;
            loader_platform_thread_unlock_mutex(&globalLock);
            synchAndDumpDot(cmdBuffer);
            char str[1024];
            sprintf(str, "DS %p bound on pipeline %s", (void*)descriptorSet, string_XGL_PIPELINE_BIND_POINT(pipelineBindPoint));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_NONE, "DS", str);
        }
        else {
            char str[1024];
            sprintf(str, "Attempt to bind DS %p that doesn't exist!", (void*)descriptorSet);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_INVALID_SET, "DS", str);
        }
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, descriptorSet, pUserData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindIndexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_BINDINDEXBUFFER);
        // TODO : Track idxBuffer binding
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindVertexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t binding)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_BINDVERTEXBUFFER);
        pCB->lastVtxBinding = binding;
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDraw(XGL_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_DRAW);
        pCB->drawCount[DRAW]++;
        char str[1024];
        sprintf(str, "xglCmdDraw() call #%lu, reporting DS state:", g_drawCount[DRAW]++);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexed(XGL_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_DRAWINDEXED);
        pCB->drawCount[DRAW_INDEXED]++;
        char str[1024];
        sprintf(str, "xglCmdDrawIndexed() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED]++);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_DRAWINDIRECT);
        pCB->drawCount[DRAW_INDIRECT]++;
        char str[1024];
        sprintf(str, "xglCmdDrawIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDIRECT]++);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_DRAWINDEXEDINDIRECT);
        pCB->drawCount[DRAW_INDEXED_INDIRECT]++;
        char str[1024];
        sprintf(str, "xglCmdDrawIndexedIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED_INDIRECT]++);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatch(XGL_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_DISPATCH);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_DISPATCHINDIRECT);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_COPYBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t regionCount, const XGL_IMAGE_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_COPYIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBufferToImage(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_IMAGE destImage, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_COPYBUFFERTOIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImageToBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_COPYIMAGETOBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_CLONEIMAGEDATA);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdUpdateBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const uint32_t* pData)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_UPDATEBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdFillBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, uint32_t data)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_FILLBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const float color[4], uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_CLEARCOLORIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImageRaw(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const uint32_t color[4], uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_CLEARCOLORIMAGERAW);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, float depth, uint32_t stencil, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_CLEARDEPTHSTENCIL);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_RESOLVEIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event, XGL_SET_EVENT pipeEvent)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_SETEVENT);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_RESETEVENT);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdResetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWaitEvents(XGL_CMD_BUFFER cmdBuffer, const XGL_EVENT_WAIT_INFO* pWaitInfo)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_WAITEVENTS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdPipelineBarrier(XGL_CMD_BUFFER cmdBuffer, const XGL_PIPELINE_BARRIER* pBarrier)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_PIPELINEBARRIER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot, XGL_FLAGS flags)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_BEGINQUERY);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_ENDQUERY);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_RESETQUERYPOOL);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWriteTimestamp(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_WRITETIMESTAMP);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdInitAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_INITATOMICCOUNTERS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdLoadAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER srcBuffer, XGL_GPU_SIZE srcOffset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_LOADATOMICCOUNTERS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSaveAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_SAVEATOMICCOUNTERS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFramebuffer(XGL_DEVICE device, const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo, XGL_FRAMEBUFFER* pFramebuffer)
{
    XGL_RESULT result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateRenderPass(XGL_DEVICE device, const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo, XGL_RENDER_PASS* pRenderPass)
{
    XGL_RESULT result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginRenderPass(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_BEGINRENDERPASS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBeginRenderPass(cmdBuffer, renderPass);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndRenderPass(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        g_lastCmdBuffer = cmdBuffer;
        addCmd(pCB, CMD_ENDRENDERPASS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdEndRenderPass(cmdBuffer, renderPass);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel)
{
    XGL_RESULT result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    XGL_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (XGL_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(XGL_LAYER_DBG_FUNCTION_NODE));
    if (!pNewDbgFuncNode)
        return XGL_ERROR_OUT_OF_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    XGL_RESULT result = nextTable.DbgRegisterMsgCallback(pfnMsgCallback, pUserData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    XGL_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
    XGL_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;
    while (pTrav) {
        if (pTrav->pfnMsgCallback == pfnMsgCallback) {
            pPrev->pNext = pTrav->pNext;
            if (g_pDbgFunctionHead == pTrav)
                g_pDbgFunctionHead = pTrav->pNext;
            free(pTrav);
            break;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }
    XGL_RESULT result = nextTable.DbgUnregisterMsgCallback(pfnMsgCallback);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(XGL_DEVICE device, int32_t msgCode, XGL_DBG_MSG_FILTER filter)
{
    XGL_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(XGL_BASE_OBJECT object, size_t tagSize, const void* pTag)
{
    XGL_RESULT result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{
    XGL_RESULT result = nextTable.DbgSetGlobalOption(dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetDeviceOption(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData)
{
    XGL_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerBegin(XGL_CMD_BUFFER cmdBuffer, const char* pMarker)
{
    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerEnd(XGL_CMD_BUFFER cmdBuffer)
{
    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

#if defined(WIN32)
// FIXME: NEED WINDOWS EQUIVALENT
#else // WIN32
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.WsiX11AssociateConnection((XGL_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(XGL_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc)
{
    XGL_RESULT result = nextTable.WsiX11GetMSC(device, window, crtc, pMsc);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11QueuePresent(XGL_QUEUE queue, const XGL_WSI_X11_PRESENT_INFO* pPresentInfo, XGL_FENCE fence)
{
    XGL_RESULT result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}
#endif // WIN32
// TODO : Want to pass in a cmdBuffer here based on which state to display
void drawStateDumpDotFile(char* outFileName)
{
    // TODO : Currently just setting cmdBuffer based on global var
    dumpDotFile(g_lastCmdBuffer, outFileName);
}

void drawStateDumpPngFile(char* outFileName)
{
#if defined(_WIN32)
// FIXME: NEED WINDOWS EQUIVALENT
        char str[1024];
        sprintf(str, "Cannot execute dot program yet on Windows.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_MISSING_DOT_PROGRAM, "DS", str);
#else // WIN32
    char dotExe[32] = "/usr/bin/dot";
    if( access(dotExe, X_OK) != -1) {
        dumpDotFile(g_lastCmdBuffer, "/tmp/tmp.dot");
        char dotCmd[1024];
        sprintf(dotCmd, "%s /tmp/tmp.dot -Tpng -o %s", dotExe, outFileName);
        system(dotCmd);
        remove("/tmp/tmp.dot");
    }
    else {
        char str[1024];
        sprintf(str, "Cannot execute dot program at (%s) to dump requested %s file.", dotExe, outFileName);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_MISSING_DOT_PROGRAM, "DS", str);
    }
#endif // WIN32
}

XGL_LAYER_EXPORT void* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const char* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    void *addr;

    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;
    else if (!strncmp("drawStateDumpDotFile", funcName, sizeof("drawStateDumpDotFile")))
        return drawStateDumpDotFile;
    else if (!strncmp("drawStateDumpPngFile", funcName, sizeof("drawStateDumpPngFile")))
        return drawStateDumpPngFile;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA(gpuw->nextObject, funcName);
    }
}
