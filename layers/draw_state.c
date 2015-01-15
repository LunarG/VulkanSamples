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
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "xgl_dispatch_table_helper.h"
#include "xgl_generic_intercept_proc_helper.h"
#include "xgl_struct_string_helper.h"
#include "xgl_struct_graphviz_helper.h"
#include "draw_state.h"
#include "layers_config.h"

static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static pthread_once_t g_initOnce = PTHREAD_ONCE_INIT;
// Could be smarter about locking with unique locks for various tasks, but just using one for now
pthread_mutex_t globalLock = PTHREAD_MUTEX_INITIALIZER;

// Ptr to LL of dbg functions
static XGL_LAYER_DBG_FUNCTION_NODE *g_pDbgFunctionHead = NULL;
static XGL_LAYER_DBG_REPORT_LEVEL g_reportingLevel = XGL_DBG_LAYER_LEVEL_INFO;
static XGL_LAYER_DBG_ACTION g_debugAction = XGL_DBG_LAYER_ACTION_LOG_MSG;
static FILE *g_logFile = NULL;

// Utility function to handle reporting
//  If callbacks are enabled, use them, otherwise use printf
static XGL_VOID layerCbMsg(XGL_DBG_MSG_TYPE msgType,
    XGL_VALIDATION_LEVEL validationLevel,
    XGL_BASE_OBJECT      srcObject,
    XGL_SIZE             location,
    XGL_INT              msgCode,
    const char*          pLayerPrefix,
    const char*          pMsg)
{
    if (g_debugAction & (XGL_DBG_LAYER_ACTION_LOG_MSG | XGL_DBG_LAYER_ACTION_CALLBACK)) {
        XGL_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
        switch (msgType) {
            case XGL_DBG_MSG_ERROR:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_ERROR) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}ERROR : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            case XGL_DBG_MSG_WARNING:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_WARN) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}WARN : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            case XGL_DBG_MSG_PERF_WARNING:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_PERF_WARN) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}PERF_WARN : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            default:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_INFO) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}INFO : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
        }
    }
}
// Return the size of the underlying struct based on struct type
static XGL_SIZE sTypeStructSize(XGL_STRUCTURE_TYPE sType)
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
        case XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO:
            return sizeof(XGL_MEMORY_VIEW_ATTACH_INFO);
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
            return sizeof(XGL_IMAGE_VIEW_ATTACH_INFO);
        case XGL_STRUCTURE_TYPE_MEMORY_STATE_TRANSITION:
            return sizeof(XGL_MEMORY_STATE_TRANSITION);
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
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO:
            return sizeof(XGL_DESCRIPTOR_SET_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO:
            return sizeof(XGL_RASTER_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO:
            return sizeof(XGL_MSAA_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO:
            return sizeof(XGL_COLOR_BLEND_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO:
            return sizeof(XGL_DEPTH_STENCIL_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
            return sizeof(XGL_CMD_BUFFER_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
            return sizeof(XGL_EVENT_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
            return sizeof(XGL_FENCE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
            return sizeof(XGL_QUEUE_SEMAPHORE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO:
            return sizeof(XGL_QUEUE_SEMAPHORE_OPEN_INFO);
        case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
            return sizeof(XGL_QUERY_POOL_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            return sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_IA_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_DB_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_CB_STATE);
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_RS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            return sizeof(XGL_PIPELINE_TESS_STATE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
            return sizeof(XGL_IMAGE_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
            return sizeof(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO);
        case XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO:
            return sizeof(XGL_LAYER_CREATE_INFO);
        default:
            return 0;
    }
}
// Return the size of the underlying struct based on Bind Point enum
//  Have to do this b/c VIEWPORT doesn't have sType in its createinfo struct
static XGL_SIZE dynStateCreateInfoSize(XGL_STATE_BIND_POINT sType)
{
    switch (sType)
    {
        case XGL_STATE_BIND_VIEWPORT:
            return sizeof(XGL_VIEWPORT_STATE_CREATE_INFO);
        case XGL_STATE_BIND_RASTER:
            return sizeof(XGL_RASTER_STATE_CREATE_INFO);
        case XGL_STATE_BIND_DEPTH_STENCIL:
            return sizeof(XGL_DEPTH_STENCIL_STATE_CREATE_INFO);
        case XGL_STATE_BIND_COLOR_BLEND:
            return sizeof(XGL_COLOR_BLEND_STATE_CREATE_INFO);
        case XGL_STATE_BIND_MSAA:
            return sizeof(XGL_MSAA_STATE_CREATE_INFO);
        default:
            return 0;
    }
}
// Block of code at start here for managing/tracking Pipeline state that this layer cares about
// Just track 2 shaders for now
#define XGL_NUM_GRAPHICS_SHADERS XGL_SHADER_STAGE_COMPUTE
#define MAX_SLOTS 2048

static uint64_t drawCount[NUM_DRAW_TYPES] = {0, 0, 0, 0};

typedef struct _SHADER_DS_MAPPING {
    XGL_UINT slotCount;
    XGL_DESCRIPTOR_SLOT_INFO* pShaderMappingSlot;
} SHADER_DS_MAPPING;

typedef struct _PIPELINE_LL_HEADER {
    XGL_STRUCTURE_TYPE sType;
    const XGL_VOID*    pNext;
} PIPELINE_LL_HEADER;

typedef struct _PIPELINE_NODE {
    XGL_PIPELINE           pipeline;
    struct _PIPELINE_NODE  *pNext;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO     *pCreateTree; // Ptr to shadow of data in create tree
    // 1st dimension of array is shader type
    SHADER_DS_MAPPING      dsMapping[XGL_NUM_GRAPHICS_SHADERS][XGL_MAX_DESCRIPTOR_SETS];
    // Vtx input info (if any)
    XGL_UINT                                vtxBindingCount;   // number of bindings
    XGL_VERTEX_INPUT_BINDING_DESCRIPTION*   pVertexBindingDescriptions;
    XGL_UINT                                vtxAttributeCount; // number of attributes
    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pVertexAttributeDescriptions;
} PIPELINE_NODE;

typedef struct _SAMPLER_NODE {
    XGL_SAMPLER              sampler;
    XGL_SAMPLER_CREATE_INFO  createInfo;
    struct _SAMPLER_NODE     *pNext;
} SAMPLER_NODE;

typedef struct _DYNAMIC_STATE_NODE {
    XGL_STATE_OBJECT     stateObj;
    XGL_STATE_BIND_POINT sType; // Extra data as VIEWPORT CreateInfo doesn't have sType
    PIPELINE_LL_HEADER   *pCreateInfo;
    struct _DYNAMIC_STATE_NODE *pNext;
} DYNAMIC_STATE_NODE;

// TODO : Should be tracking lastBound per cmdBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
static PIPELINE_NODE *pPipelineHead = NULL;
static SAMPLER_NODE *pSamplerHead = NULL;
static XGL_PIPELINE lastBoundPipeline = NULL;
#define MAX_BINDING 0xFFFFFFFF
static XGL_UINT lastVtxBinding = MAX_BINDING;

static DYNAMIC_STATE_NODE* pDynamicStateHead[XGL_NUM_STATE_BIND_POINT] = {0};
static DYNAMIC_STATE_NODE* pLastBoundDynamicState[XGL_NUM_STATE_BIND_POINT] = {0};

// Viewport state create info doesn't have sType so we have to pass in BIND_POINT
static void insertDynamicState(const XGL_STATE_OBJECT state, const PIPELINE_LL_HEADER* pCreateInfo, const XGL_STATE_BIND_POINT sType)
{
    pthread_mutex_lock(&globalLock);
    // Insert new node at head of appropriate LL
    DYNAMIC_STATE_NODE* pStateNode = (DYNAMIC_STATE_NODE*)malloc(sizeof(DYNAMIC_STATE_NODE));
    pStateNode->pNext = pDynamicStateHead[sType];
    pDynamicStateHead[sType] = pStateNode;
    pStateNode->stateObj = state;
    pStateNode->sType = sType;
    pStateNode->pCreateInfo = (PIPELINE_LL_HEADER*)malloc(dynStateCreateInfoSize(sType));
    memcpy(pStateNode->pCreateInfo, pCreateInfo, dynStateCreateInfoSize(sType));
    pthread_mutex_unlock(&globalLock);
}
// Set the last bound dynamic state of given type
// TODO : Need to track this per cmdBuffer and correlate cmdBuffer for Draw w/ last bound for that cmdBuffer?
static void setLastBoundDynamicState(const XGL_STATE_OBJECT state, const XGL_STATE_BIND_POINT sType)
{
    pthread_mutex_lock(&globalLock);
    DYNAMIC_STATE_NODE* pTrav = pDynamicStateHead[sType];
    while (pTrav && (state != pTrav->stateObj)) {
        pTrav = pTrav->pNext;
    }
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Unable to find dynamic state object %p, was it ever created?", (void*)state);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, state, 0, DRAWSTATE_INVALID_DYNAMIC_STATE_OBJECT, "DS", str);
    }
    pLastBoundDynamicState[sType] = pTrav;
    pthread_mutex_unlock(&globalLock);
}
// Print the last bound dynamic state
static void printDynamicState()
{
    pthread_mutex_lock(&globalLock);
    char str[1024];
    for (uint32_t i = 0; i < XGL_NUM_STATE_BIND_POINT; i++) {
        if (pLastBoundDynamicState[i]) {
            sprintf(str, "Reporting CreateInfo for currently bound %s object %p", string_XGL_STATE_BIND_POINT(i), pLastBoundDynamicState[i]->stateObj);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pLastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", str);
            switch (pLastBoundDynamicState[i]->sType)
            {
                case XGL_STATE_BIND_VIEWPORT:
                    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pLastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", xgl_print_xgl_viewport_state_create_info((XGL_VIEWPORT_STATE_CREATE_INFO*)pLastBoundDynamicState[i]->pCreateInfo, "  "));
                    break;
                default:
                    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pLastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", dynamic_display(pLastBoundDynamicState[i]->pCreateInfo, "  "));
                    break;
            }
        }
        else {
            sprintf(str, "No dynamic state of type %s bound", string_XGL_STATE_BIND_POINT(i));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", str);
        }
    }
    pthread_mutex_unlock(&globalLock);
}
// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_NODE *getPipeline(XGL_PIPELINE pipeline)
{
    pthread_mutex_lock(&globalLock);
    PIPELINE_NODE *pTrav = pPipelineHead;
    while (pTrav) {
        if (pTrav->pipeline == pipeline) {
            pthread_mutex_unlock(&globalLock);
            return pTrav;
        }
        pTrav = pTrav->pNext;
    }
    pthread_mutex_unlock(&globalLock);
    return NULL;
}

// For given sampler, return a ptr to its Create Info struct, or NULL if sampler not found
static XGL_SAMPLER_CREATE_INFO* getSamplerCreateInfo(const XGL_SAMPLER sampler)
{
    pthread_mutex_lock(&globalLock);
    SAMPLER_NODE *pTrav = pSamplerHead;
    while (pTrav) {
        if (sampler == pTrav->sampler) {
            pthread_mutex_unlock(&globalLock);
            return &pTrav->createInfo;
        }
        pTrav = pTrav->pNext;
    }
    pthread_mutex_unlock(&globalLock);
    return NULL;
}

// Init the pipeline mapping info based on pipeline create info LL tree
//  Threading note : Calls to this function should wrapped in mutex
static void initPipeline(PIPELINE_NODE *pPipeline, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo)
{
    // First init create info, we'll shadow the structs as we go down the tree
    pPipeline->pCreateTree = (XGL_GRAPHICS_PIPELINE_CREATE_INFO*)malloc(sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));
    memcpy(pPipeline->pCreateTree, pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));
    PIPELINE_LL_HEADER *pShadowTrav = (PIPELINE_LL_HEADER*)pPipeline->pCreateTree;
    PIPELINE_LL_HEADER *pTrav = (PIPELINE_LL_HEADER*)pCreateInfo->pNext;
    while (pTrav) {
        // Shadow the struct
        pShadowTrav->pNext = (PIPELINE_LL_HEADER*)malloc(sTypeStructSize(pTrav->sType));
        // Typically pNext is const so have to cast to avoid warning when we modify it here
        memcpy((void*)pShadowTrav->pNext, pTrav, sTypeStructSize(pTrav->sType));
        pShadowTrav = (PIPELINE_LL_HEADER*)pShadowTrav->pNext;
        // For deep copy DS Mapping into shadow
        XGL_PIPELINE_SHADER_STAGE_CREATE_INFO *pShadowShaderCI = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pShadowTrav;
        // TODO : Now that we shadow whole create info, the special copies are just a convenience that can be done away with once shadow is complete and correct
        // Special copy of DS Mapping info
        if (XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO == pTrav->sType) {
            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO *pSSCI = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pTrav;
            for (uint32_t i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++) {
                if (pSSCI->shader.descriptorSetMapping[i].descriptorCount > MAX_SLOTS) {
                    char str[1024];
                    sprintf(str, "descriptorCount for %s exceeds 2048 (%u), is this correct? Changing to 0", string_XGL_PIPELINE_SHADER_STAGE(pSSCI->shader.stage), pSSCI->shader.descriptorSetMapping[i].descriptorCount);
                    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pPipeline, 0, DRAWSTATE_DESCRIPTOR_MAX_EXCEEDED, "DS", str);
                    pSSCI->shader.descriptorSetMapping[i].descriptorCount = 0;
                }
                pPipeline->dsMapping[pSSCI->shader.stage][i].slotCount = pSSCI->shader.descriptorSetMapping[i].descriptorCount;
                // Deep copy DS Slot array into our shortcut data structure
                pPipeline->dsMapping[pSSCI->shader.stage][i].pShaderMappingSlot = (XGL_DESCRIPTOR_SLOT_INFO*)malloc(sizeof(XGL_DESCRIPTOR_SLOT_INFO)*pPipeline->dsMapping[pSSCI->shader.stage][i].slotCount);
                memcpy(pPipeline->dsMapping[pSSCI->shader.stage][i].pShaderMappingSlot, pSSCI->shader.descriptorSetMapping[i].pDescriptorInfo, sizeof(XGL_DESCRIPTOR_SLOT_INFO)*pPipeline->dsMapping[pSSCI->shader.stage][i].slotCount);
                // Deep copy into shadow tree
                pShadowShaderCI->shader.descriptorSetMapping[i].descriptorCount = pSSCI->shader.descriptorSetMapping[i].descriptorCount;
                pShadowShaderCI->shader.descriptorSetMapping[i].pDescriptorInfo = (XGL_DESCRIPTOR_SLOT_INFO*)malloc(sizeof(XGL_DESCRIPTOR_SLOT_INFO)*pShadowShaderCI->shader.descriptorSetMapping[i].descriptorCount);
                memcpy((XGL_DESCRIPTOR_SLOT_INFO*)pShadowShaderCI->shader.descriptorSetMapping[i].pDescriptorInfo, pSSCI->shader.descriptorSetMapping[i].pDescriptorInfo, sizeof(XGL_DESCRIPTOR_SLOT_INFO)*pShadowShaderCI->shader.descriptorSetMapping[i].descriptorCount);
            }
        }
        else if (XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO == pTrav->sType) {
            // Special copy of Vtx info
            XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO *pVICI = (XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pTrav;
            pPipeline->vtxBindingCount = pVICI->bindingCount;
            uint32_t allocSize = pPipeline->vtxBindingCount * sizeof(XGL_VERTEX_INPUT_BINDING_DESCRIPTION);
            pPipeline->pVertexBindingDescriptions = (XGL_VERTEX_INPUT_BINDING_DESCRIPTION*)malloc(allocSize);
            memcpy(pPipeline->pVertexBindingDescriptions, pVICI->pVertexAttributeDescriptions, allocSize);
            pPipeline->vtxAttributeCount = pVICI->attributeCount;
            allocSize = pPipeline->vtxAttributeCount * sizeof(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION);
            pPipeline->pVertexAttributeDescriptions = (XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION*)malloc(allocSize);
            memcpy(pPipeline->pVertexAttributeDescriptions, pVICI->pVertexAttributeDescriptions, allocSize);
        }
        pTrav = (PIPELINE_LL_HEADER*)pTrav->pNext;
    }
}

// Block of code at start here specifically for managing/tracking DSs
#define MAPPING_MEMORY  0x00000001
#define MAPPING_IMAGE   0x00000002
#define MAPPING_SAMPLER 0x00000004
#define MAPPING_DS      0x00000008

static char* stringSlotBinding(XGL_UINT binding)
{
    switch (binding)
    {
        case MAPPING_MEMORY:
            return "Memory View";
        case MAPPING_IMAGE:
            return "Image View";
        case MAPPING_SAMPLER:
            return "Sampler";
        default:
            return "UNKNOWN DS BINDING";
    }
}

typedef struct _DS_SLOT {
    XGL_UINT                     slot;
    XGL_DESCRIPTOR_SLOT_INFO     shaderSlotInfo[XGL_NUM_GRAPHICS_SHADERS];
    // Only 1 of 4 possible slot mappings active
    XGL_UINT                     activeMapping;
    XGL_UINT                     mappingMask; // store record of different mappings used
    XGL_MEMORY_VIEW_ATTACH_INFO  memView;
    XGL_IMAGE_VIEW_ATTACH_INFO   imageView;
    XGL_SAMPLER                  sampler;            
} DS_SLOT;

// Top-level node that points to start of DS
typedef struct _DS_LL_HEAD {
    XGL_DESCRIPTOR_SET dsID;
    XGL_UINT           numSlots;
    struct _DS_LL_HEAD *pNextDS;
    DS_SLOT            *dsSlot; // Dynamically allocated array of DS_SLOTs
    XGL_BOOL           updateActive; // Track if DS is in an update block
} DS_LL_HEAD;

// ptr to HEAD of LL of DSs
static DS_LL_HEAD *pDSHead = NULL;
// Last DS that was bound, and slotOffset for the binding
static XGL_DESCRIPTOR_SET lastBoundDS[XGL_MAX_DESCRIPTOR_SETS] = {NULL, NULL};
static XGL_UINT lastBoundSlotOffset[XGL_MAX_DESCRIPTOR_SETS] = {0, 0};

// Return DS Head ptr for specified ds or else NULL
static DS_LL_HEAD* getDS(XGL_DESCRIPTOR_SET ds)
{
    pthread_mutex_lock(&globalLock);
    DS_LL_HEAD *pTrav = pDSHead;
    while (pTrav) {
        if (pTrav->dsID == ds) {
            pthread_mutex_unlock(&globalLock);
            return pTrav;
        }
        pTrav = pTrav->pNextDS;
    }
    pthread_mutex_unlock(&globalLock);
    return NULL;
}

// Initialize a DS where all slots are UNUSED for all shaders
static void initDS(DS_LL_HEAD *pDS)
{
    for (uint32_t i = 0; i < pDS->numSlots; i++) {
        memset((void*)&pDS->dsSlot[i], 0, sizeof(DS_SLOT));
        pDS->dsSlot[i].slot = i;
    }
}

// Return XGL_TRUE if DS Exists and is within an xglBeginDescriptorSetUpdate() call sequence, otherwise XGL_FALSE
static XGL_BOOL dsUpdate(XGL_DESCRIPTOR_SET ds)
{
    DS_LL_HEAD *pTrav = getDS(ds);
    if (pTrav)
        return pTrav->updateActive;
    return XGL_FALSE;
}

// Clear specified slotCount DS Slots starting at startSlot
// Return XGL_TRUE if DS exists and is successfully cleared to 0s
static XGL_BOOL clearDS(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount)
{
    DS_LL_HEAD *pTrav = getDS(descriptorSet);
    pthread_mutex_lock(&globalLock);
    if (!pTrav || ((startSlot + slotCount) > pTrav->numSlots)) {
        // TODO : Log more meaningful error here
        pthread_mutex_unlock(&globalLock);
        return XGL_FALSE;
    }
    for (uint32_t i = startSlot; i < slotCount; i++) {
        memset((void*)&pTrav->dsSlot[i], 0, sizeof(DS_SLOT));
    }
    pthread_mutex_unlock(&globalLock);
    return XGL_TRUE;
}

static void dsSetMapping(DS_SLOT* pSlot, XGL_UINT mapping)
{
    pSlot->mappingMask   |= mapping;
    pSlot->activeMapping = mapping;
}
// Populate pStr w/ a string noting all of the slot mappings based on mapping flag
static char* noteSlotMapping(XGL_UINT32 mapping, char *pStr)
{
    if (MAPPING_MEMORY & mapping)
        strcat(pStr, "\n\tMemory View previously mapped");
    if (MAPPING_IMAGE & mapping)
        strcat(pStr, "\n\tImage View previously mapped");
    if (MAPPING_SAMPLER & mapping)
        strcat(pStr, "\n\tSampler previously mapped");
    if (MAPPING_DS & mapping)
        strcat(pStr, "\n\tDESCRIPTOR SET ptr previously mapped");
    return pStr;
}

static void dsSetMemMapping(XGL_DESCRIPTOR_SET descriptorSet, DS_SLOT* pSlot, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView)
{
    if (pSlot->mappingMask) {
        char str[1024];
        char map_str[1024] = {0};
        sprintf(str, "While mapping Memory View to slot %u previous Mapping(s) identified:%s", pSlot->slot, noteSlotMapping(pSlot->mappingMask, map_str));
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_SLOT_REMAPPING, "DS", str);
    }
    memcpy(&pSlot->memView, pMemView, sizeof(XGL_MEMORY_VIEW_ATTACH_INFO));
    dsSetMapping(pSlot, MAPPING_MEMORY);
}

static XGL_BOOL dsMemMapping(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews)
{
    DS_LL_HEAD *pTrav = getDS(descriptorSet);
    if (pTrav) {
        if (pTrav->numSlots < (startSlot + slotCount)) {
            return XGL_FALSE;
        }
        for (uint32_t i = 0; i < slotCount; i++) {
            dsSetMemMapping(descriptorSet, &pTrav->dsSlot[i+startSlot], &pMemViews[i]);
        }
    }
    else
        return XGL_FALSE;
    return XGL_TRUE;
}

static void dsSetImageMapping(XGL_DESCRIPTOR_SET descriptorSet, DS_SLOT* pSlot, const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews)
{
    if (pSlot->mappingMask) {
        char str[1024];
        char map_str[1024] = {0};
        sprintf(str, "While mapping Image View to slot %u previous Mapping(s) identified:%s", pSlot->slot, noteSlotMapping(pSlot->mappingMask, map_str));
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_SLOT_REMAPPING, "DS", str);
    }
    memcpy(&pSlot->imageView, pImageViews, sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));
    dsSetMapping(pSlot, MAPPING_IMAGE);
}

static XGL_BOOL dsImageMapping(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews)
{
    DS_LL_HEAD *pTrav = getDS(descriptorSet);
    if (pTrav) {
        if (pTrav->numSlots < (startSlot + slotCount)) {
            return XGL_FALSE;
        }
        for (uint32_t i = 0; i < slotCount; i++) {
            dsSetImageMapping(descriptorSet, &pTrav->dsSlot[i+startSlot], &pImageViews[i]);
        }
    }
    else
        return XGL_FALSE;
    return XGL_TRUE;
}

static void dsSetSamplerMapping(XGL_DESCRIPTOR_SET descriptorSet, DS_SLOT* pSlot, const XGL_SAMPLER sampler)
{
    if (pSlot->mappingMask) {
        char str[1024];
        char map_str[1024] = {0};
        sprintf(str, "While mapping Sampler to slot %u previous Mapping(s) identified:%s", pSlot->slot, noteSlotMapping(pSlot->mappingMask, map_str));
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_SLOT_REMAPPING, "DS", str);
    }
    pSlot->sampler = sampler;
    dsSetMapping(pSlot, MAPPING_SAMPLER);
}

static XGL_BOOL dsSamplerMapping(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_SAMPLER* pSamplers)
{
    DS_LL_HEAD *pTrav = getDS(descriptorSet);
    if (pTrav) {
        if (pTrav->numSlots < (startSlot + slotCount)) {
            return XGL_FALSE;
        }
        for (uint32_t i = 0; i < slotCount; i++) {
            dsSetSamplerMapping(descriptorSet, &pTrav->dsSlot[i+startSlot], pSamplers[i]);
        }
    }
    else
        return XGL_FALSE;
    return XGL_TRUE;
}
// Print the last bound Gfx Pipeline
static void printPipeline()
{
    PIPELINE_NODE *pPipeTrav = getPipeline(lastBoundPipeline);
    if (!pPipeTrav) {
        // nothing to print
    }
    else {
        char* pipeStr = xgl_print_xgl_graphics_pipeline_create_info(pPipeTrav->pCreateTree, "{DS}");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", pipeStr);
    }
}
// Dump subgraph w/ DS info
static void dsDumpDot(FILE* pOutFile)
{
    const int i = 0; // hard-coding to just the first DS index for now
    uint32_t skipUnusedCount = 0; // track consecutive unused slots for minimal reporting
    DS_LL_HEAD *pDS = getDS(lastBoundDS[i]);
    if (pDS) {
        fprintf(pOutFile, "subgraph DS_SLOTS\n{\nlabel=\"DS0 Slots\"\n");
        // First create simple array node as central DS reference point
        fprintf(pOutFile, "\"DS0_MEMORY\" [\nlabel = <<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\"> <TR><TD PORT=\"ds2\">DS0 Memory</TD></TR>");
        uint32_t j;
        char label[1024];
        for (j = 0; j < pDS->numSlots; j++) {
            // Don't draw unused slots
            if (0 != pDS->dsSlot[j].activeMapping)
                fprintf(pOutFile, "<TR><TD PORT=\"slot%u\">slot%u</TD></TR>", j, j);
        }
        fprintf(pOutFile, "</TABLE>>\n];\n");
        // Now tie each slot to its info
        for (j = 0; j < pDS->numSlots; j++) {
            switch (pDS->dsSlot[j].activeMapping)
            {
                case MAPPING_MEMORY:
                    /*
                    if (0 != skipUnusedCount) {// finish sequence of unused slots
                        sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
                        strcat(ds_config_str, tmp_str);
                        skipUnusedCount = 0;
                    }*/
                    sprintf(label, "MemAttachInfo Slot%u", j);
                    fprintf(pOutFile, "%s", xgl_gv_print_xgl_memory_view_attach_info(&pDS->dsSlot[j].memView, label));
                    fprintf(pOutFile, "\"DS0_MEMORY\":slot%u -> \"%s\" [];\n", j, label);
                    break;
                case MAPPING_IMAGE:
                    /*if (0 != skipUnusedCount) {// finish sequence of unused slots
                        sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
                        strcat(ds_config_str, tmp_str);
                        skipUnusedCount = 0;
                    }*/
                    sprintf(label, "ImageAttachInfo Slot%u", j);
                    fprintf(pOutFile, "%s", xgl_gv_print_xgl_image_view_attach_info(&pDS->dsSlot[j].imageView, label));
                    fprintf(pOutFile, "\"DS0_MEMORY\":slot%u -> \"%s\" [];\n", j, label);
                    break;
                case MAPPING_SAMPLER:
                    /*if (0 != skipUnusedCount) {// finish sequence of unused slots
                        sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
                        strcat(ds_config_str, tmp_str);
                        skipUnusedCount = 0;
                    }*/
                    sprintf(label, "SamplerAttachInfo Slot%u", j);
                    fprintf(pOutFile, "%s", xgl_gv_print_xgl_sampler_create_info(getSamplerCreateInfo(pDS->dsSlot[j].sampler), label));
                    fprintf(pOutFile, "\"DS0_MEMORY\":slot%u -> \"%s\" [];\n", j, label);
                    break;
                default:
                    /*if (!skipUnusedCount) {// only report start of unused sequences
                        sprintf(tmp_str, "----Skipping slot(s) w/o a view attached...\n");
                        strcat(ds_config_str, tmp_str);
                    }*/
                    skipUnusedCount++;
                    break;
            }

        }
        /*if (0 != skipUnusedCount) {// finish sequence of unused slots
            sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
            strcat(ds_config_str, tmp_str);
            skipUnusedCount = 0;
        }*/
        fprintf(pOutFile, "}\n");
    }
}
// Dump a GraphViz dot file showing the pipeline
static void dumpDotFile(char *outFileName)
{
    PIPELINE_NODE *pPipeTrav = getPipeline(lastBoundPipeline);
    if (pPipeTrav) {
        FILE* pOutFile;
        pOutFile = fopen(outFileName, "w");
        fprintf(pOutFile, "digraph g {\ngraph [\nrankdir = \"TB\"\n];\nnode [\nfontsize = \"16\"\nshape = \"plaintext\"\n];\nedge [\n];\n");
        fprintf(pOutFile, "subgraph PipelineStateObject\n{\nlabel=\"Pipeline State Object\"\n");
        fprintf(pOutFile, "%s", xgl_gv_print_xgl_graphics_pipeline_create_info(pPipeTrav->pCreateTree, "PSO HEAD"));
        fprintf(pOutFile, "}\n");
        // TODO : Add dynamic state dump here
        fprintf(pOutFile, "subgraph dynamicState\n{\nlabel=\"Non-Orthogonal XGL State\"\n");
        for (uint32_t i = 0; i < XGL_NUM_STATE_BIND_POINT; i++) {
            if (pLastBoundDynamicState[i]) {
                switch (pLastBoundDynamicState[i]->sType)
                {
                    case XGL_STATE_BIND_VIEWPORT:
                        fprintf(pOutFile, "%s", xgl_gv_print_xgl_viewport_state_create_info((XGL_VIEWPORT_STATE_CREATE_INFO*)pLastBoundDynamicState[i]->pCreateInfo, "VIEWPORT State"));
                        break;
                    default:
                        fprintf(pOutFile, "%s", dynamic_gv_display(pLastBoundDynamicState[i]->pCreateInfo, string_XGL_STATE_BIND_POINT(pLastBoundDynamicState[i]->sType)));
                        break;
                }
            }
        }
        fprintf(pOutFile, "}\n"); // close dynamicState subgraph
        dsDumpDot(pOutFile);
        fprintf(pOutFile, "}\n"); // close main graph "g"
        fclose(pOutFile);
    }
}
// Synch up currently bound pipeline settings with DS mappings
static void synchDSMapping()
{
    // First verify that we have a bound pipeline
    PIPELINE_NODE *pPipeTrav = getPipeline(lastBoundPipeline);
    char str[1024];
    if (!pPipeTrav) {
        sprintf(str, "Can't find last bound Pipeline %p!", (void*)lastBoundPipeline);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NO_PIPELINE_BOUND, "DS", str);
    }
    else {
        // Synch Descriptor Set Mapping
        for (uint32_t i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++) {
            DS_LL_HEAD *pDS;
            if (lastBoundDS[i]) {
                pDS = getDS(lastBoundDS[i]);
                if (!pDS) {
                    sprintf(str, "Can't find last bound DS %p. Did you need to bind DS to index %u?", (void*)lastBoundDS[i], i);
                    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NO_DS_BOUND, "DS", str);
                }
                else { // We have a good DS & Pipeline, store pipeline mappings in DS
                    XGL_UINT slotOffset = lastBoundSlotOffset[i];
                    for (uint32_t j = 0; j < XGL_NUM_GRAPHICS_SHADERS; j++) { // j is shader selector
                        if (pPipeTrav->dsMapping[j][i].slotCount > (pDS->numSlots - slotOffset)) {
                            sprintf(str, "DS Mapping for shader %u has more slots (%u) than DS %p (%u) minus slotOffset (%u) (%u slots available)!", j, pPipeTrav->dsMapping[j][i].slotCount, (void*)pDS->dsID, pDS->numSlots, slotOffset, (pDS->numSlots - slotOffset));
                            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_DS_SLOT_NUM_MISMATCH, "DS", str);
                        }
                        else {
                            for (uint32_t r = 0; r < pPipeTrav->dsMapping[j][i].slotCount; r++) {
                                pDS->dsSlot[r+slotOffset].shaderSlotInfo[j] = pPipeTrav->dsMapping[j][i].pShaderMappingSlot[r];
                            }
                        }
                    }
                }
            }
            else {
                // Verify that no shader is mapping this DS
                uint32_t dsUsed = 0;
                for (uint32_t j = 0; j < XGL_NUM_GRAPHICS_SHADERS; j++) { // j is shader selector
                    if (pPipeTrav->dsMapping[j][i].slotCount > 0) {
                        dsUsed = 1;
                        sprintf(str, "No DS was bound to index %u, but shader type %s has slots bound to that DS.", i, string_XGL_PIPELINE_SHADER_STAGE(j));
                        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NO_DS_BOUND, "DS", str);
                    }
                }
                if (0 == dsUsed) {
                    sprintf(str, "No DS was bound to index %u, but no shaders are using that DS so this is not an issue.", i);
                    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", str);
                }
            }
        }
        // Verify Vtx binding
        if (MAX_BINDING != lastVtxBinding) {
            if (lastVtxBinding >= pPipeTrav->vtxBindingCount) {
                sprintf(str, "Vtx binding Index of %u exceeds PSO pVertexBindingDescriptions max array index of %u.", lastVtxBinding, (pPipeTrav->vtxBindingCount - 1));
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS", str);
            }
            else {
                char *tmpStr = xgl_print_xgl_vertex_input_binding_description(&pPipeTrav->pVertexBindingDescriptions[lastVtxBinding], "{DS}INFO : ");
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmpStr);
                free(tmpStr);
            }
        }
    }
}

// Checks to make sure that shader mapping matches slot binding
// Print an ERROR and return XGL_FALSE if they don't line up
static XGL_BOOL verifyShaderSlotMapping(const XGL_UINT slot, const XGL_UINT slotBinding, const XGL_UINT shaderStage, const XGL_DESCRIPTOR_SET_SLOT_TYPE shaderMapping)
{
    XGL_BOOL error = XGL_FALSE;
    char str[1024];
    switch (shaderMapping)
    {
        case XGL_SLOT_SHADER_TEXTURE_RESOURCE:
        case XGL_SLOT_SHADER_RESOURCE:
            if (MAPPING_MEMORY != slotBinding && MAPPING_IMAGE != slotBinding)
                error = XGL_TRUE;
            break;
        case XGL_SLOT_SHADER_SAMPLER:
            if (MAPPING_SAMPLER != slotBinding)
                error = XGL_TRUE;
            break;
        case XGL_SLOT_SHADER_UAV:
            if (MAPPING_MEMORY != slotBinding)
                error = XGL_TRUE;
            break;
        case XGL_SLOT_NEXT_DESCRIPTOR_SET:
            if (MAPPING_DS != slotBinding)
                error = XGL_TRUE;
            break;
        case XGL_SLOT_UNUSED:
            break;
        default:
            sprintf(str, "For DS slot %u, unknown shader slot mapping w/ value %u", slot, shaderMapping);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_UNKNOWN_DS_MAPPING, "DS", str);
            return XGL_FALSE;
    }
    if (XGL_TRUE == error) {
        sprintf(str, "DS Slot #%u binding of %s does not match %s shader mapping of %s", slot, stringSlotBinding(slotBinding), string_XGL_PIPELINE_SHADER_STAGE(shaderStage), string_XGL_DESCRIPTOR_SET_SLOT_TYPE(shaderMapping));
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_DS_MAPPING_MISMATCH, "DS", str);
        return XGL_FALSE;
    }
    return XGL_TRUE;
}

// Print details of DS config to stdout
static void printDSConfig()
{
    uint32_t skipUnusedCount = 0; // track consecutive unused slots for minimal reporting
    char tmp_str[1024];
    char ds_config_str[1024*256] = {0}; // TODO : Currently making this buffer HUGE w/o overrun protection.  Need to be smarter, start smaller, and grow as needed.
    for (uint32_t i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++) {
        if (lastBoundDS[i]) {
            DS_LL_HEAD *pDS = getDS(lastBoundDS[i]);
            XGL_UINT slotOffset = lastBoundSlotOffset[i];
            if (pDS) {
                sprintf(tmp_str, "DS INFO : Slot bindings for DS[%u] (%p) - %u slots and slotOffset %u:\n", i, (void*)pDS->dsID, pDS->numSlots, slotOffset);
                strcat(ds_config_str, tmp_str);
                for (uint32_t j = 0; j < pDS->numSlots; j++) {
                    switch (pDS->dsSlot[j].activeMapping)
                    {
                        case MAPPING_MEMORY:
                            if (0 != skipUnusedCount) {// finish sequence of unused slots
                                sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
                                strcat(ds_config_str, tmp_str);
                                skipUnusedCount = 0;
                            }
                            sprintf(tmp_str, "----Slot %u\n    Mapped to Memory View %p:\n%s", j, (void*)&pDS->dsSlot[j].memView, xgl_print_xgl_memory_view_attach_info(&pDS->dsSlot[j].memView, "        "));
                            strcat(ds_config_str, tmp_str);
                            break;
                        case MAPPING_IMAGE:
                            if (0 != skipUnusedCount) {// finish sequence of unused slots
                                sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
                                strcat(ds_config_str, tmp_str);
                                skipUnusedCount = 0;
                            }
                            sprintf(tmp_str, "----Slot %u\n    Mapped to Image View %p:\n%s", j, (void*)&pDS->dsSlot[j].imageView, xgl_print_xgl_image_view_attach_info(&pDS->dsSlot[j].imageView, "        "));
                            strcat(ds_config_str, tmp_str);
                            break;
                        case MAPPING_SAMPLER:
                            if (0 != skipUnusedCount) {// finish sequence of unused slots
                                sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
                                strcat(ds_config_str, tmp_str);
                                skipUnusedCount = 0;
                            }
                            sprintf(tmp_str, "----Slot %u\n    Mapped to Sampler Object %p:\n%s", j, (void*)pDS->dsSlot[j].sampler, xgl_print_xgl_sampler_create_info(getSamplerCreateInfo(pDS->dsSlot[j].sampler), "        "));
                            strcat(ds_config_str, tmp_str);
                            break;
                        default:
                            if (!skipUnusedCount) {// only report start of unused sequences
                                sprintf(tmp_str, "----Skipping slot(s) w/o a view attached...\n");
                                strcat(ds_config_str, tmp_str);
                            }
                            skipUnusedCount++;
                            break;
                    }
                    // For each shader type, check its mapping
                    for (uint32_t k = 0; k < XGL_NUM_GRAPHICS_SHADERS; k++) {
                        if (XGL_SLOT_UNUSED != pDS->dsSlot[j].shaderSlotInfo[k].slotObjectType) {
                            sprintf(tmp_str, "    Shader type %s has %s slot type mapping to shaderEntityIndex %u\n", string_XGL_PIPELINE_SHADER_STAGE(k), string_XGL_DESCRIPTOR_SET_SLOT_TYPE(pDS->dsSlot[j].shaderSlotInfo[k].slotObjectType), pDS->dsSlot[j].shaderSlotInfo[k].shaderEntityIndex);
                            strcat(ds_config_str, tmp_str);
                            verifyShaderSlotMapping(j, pDS->dsSlot[j].activeMapping, k, pDS->dsSlot[j].shaderSlotInfo[k].slotObjectType);
                        }
                    }
                }
                if (0 != skipUnusedCount) {// finish sequence of unused slots
                    sprintf(tmp_str, "----Skipped %u slot%s w/o a view attached...\n", skipUnusedCount, (1 != skipUnusedCount) ? "s" : "");
                    strcat(ds_config_str, tmp_str);
                    skipUnusedCount = 0;
                }
            }
            else {
                char str[1024];
                sprintf(str, "Can't find last bound DS %p. Did you need to bind DS to index %u?", (void*)lastBoundDS[i], i);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NO_DS_BOUND, "DS", str);
            }
        }
    }
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", ds_config_str);
}

static void synchAndPrintDSConfig()
{
    synchDSMapping();
    printDSConfig();
    printPipeline();
    printDynamicState();
    static int autoDumpOnce = 1;
    if (autoDumpOnce) {
        autoDumpOnce = 0;
        dumpDotFile("pipeline_dump.dot");
        // Convert dot to png if dot available
        if(access( "/usr/bin/dot", X_OK) != -1) {
            system("/usr/bin/dot pipeline_dump.dot -Tpng -o pipeline_dump.png");
        }
    }
}

static void initDrawState()
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

    xglGetProcAddrType fpGetProcAddr = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetProcAddr");
    nextTable.GetProcAddr = fpGetProcAddr;
}


XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyDevice(XGL_DEVICE device)
{
    XGL_RESULT result = nextTable.DestroyDevice(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize, XGL_SIZE* pOutLayerCount, XGL_CHAR* const* pOutLayers, XGL_VOID* pReserved)
{
    if (gpu != NULL)
    {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        pCurObj = gpuw;
        pthread_once(&g_initOnce, initDrawState);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(XGL_DEVICE device, XGL_QUEUE_TYPE queueType, XGL_UINT queueIndex, XGL_QUEUE* pQueue)
{
    XGL_RESULT result = nextTable.GetDeviceQueue(device, queueType, queueIndex, pQueue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, XGL_UINT cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
{
    XGL_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(XGL_QUEUE queue, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs)
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglMapMemory(XGL_GPU_MEMORY mem, XGL_FLAGS flags, XGL_VOID** ppData)
{
    XGL_RESULT result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(XGL_GPU_MEMORY mem)
{
    XGL_RESULT result = nextTable.UnmapMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(XGL_DEVICE device, const XGL_VOID* pSysMem, XGL_SIZE memSize, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglRemapVirtualMemoryPages(XGL_DEVICE device, XGL_UINT rangeCount, const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges, XGL_UINT preWaitSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPreWaitSemaphores, XGL_UINT postSignalSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPostSignalSemaphores)
{
    XGL_RESULT result = nextTable.RemapVirtualMemoryPages(device, rangeCount, pRanges, preWaitSemaphoreCount, pPreWaitSemaphores, postSignalSemaphoreCount, pPostSignalSemaphores);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu0;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initDrawState);
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
    XGL_RESULT result = nextTable.DestroyObject(object);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(XGL_OBJECT object, XGL_UINT allocationIdx, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    XGL_RESULT result = nextTable.BindObjectMemory(object, allocationIdx, mem, offset);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, XGL_UINT fenceCount, const XGL_FENCE* pFences, XGL_BOOL waitAll, XGL_UINT64 timeout)
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
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
    pthread_mutex_lock(&globalLock);
    PIPELINE_NODE *pTrav = pPipelineHead;
    if (pTrav) {
        while (pTrav->pNext)
            pTrav = pTrav->pNext;
        pTrav->pNext = (PIPELINE_NODE*)malloc(sizeof(PIPELINE_NODE));
        pTrav = pTrav->pNext;
    }
    else {
        pTrav = (PIPELINE_NODE*)malloc(sizeof(PIPELINE_NODE));
        pPipelineHead = pTrav;
    }
    memset((void*)pTrav, 0, sizeof(PIPELINE_NODE));
    pTrav->pipeline = *pPipeline;
    initPipeline(pTrav, pCreateInfo);
    pthread_mutex_unlock(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglStorePipeline(XGL_PIPELINE pipeline, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(XGL_DEVICE device, XGL_SIZE dataSize, const XGL_VOID* pData, XGL_PIPELINE* pPipeline)
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
    pthread_mutex_lock(&globalLock);
    SAMPLER_NODE *pNewNode = (SAMPLER_NODE*)malloc(sizeof(SAMPLER_NODE));
    pNewNode->sampler = *pSampler;
    memcpy(&pNewNode->createInfo, pCreateInfo, sizeof(XGL_SAMPLER_CREATE_INFO));
    pNewNode->pNext = pSamplerHead;
    pSamplerHead = pNewNode;
    pthread_mutex_unlock(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSet(XGL_DEVICE device, const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_SET* pDescriptorSet)
{
    XGL_RESULT result = nextTable.CreateDescriptorSet(device, pCreateInfo, pDescriptorSet);
    // Create LL chain
    char str[1024];
    sprintf(str, "Created Descriptor Set (DS) %p", (void*)*pDescriptorSet);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pDescriptorSet, 0, DRAWSTATE_NONE, "DS", str);
    pthread_mutex_lock(&globalLock);
    DS_LL_HEAD *pTrav = pDSHead;
    if (pTrav) {
        // Grow existing list
        while (pTrav->pNextDS)
            pTrav = pTrav->pNextDS;
        pTrav->pNextDS = (DS_LL_HEAD*)malloc(sizeof(DS_LL_HEAD));
        pTrav = pTrav->pNextDS;
    }
    else { // Create new list
        pTrav = (DS_LL_HEAD*)malloc(sizeof(DS_LL_HEAD));
        pDSHead = pTrav;
    }
    pTrav->dsSlot = (DS_SLOT*)malloc(sizeof(DS_SLOT) * pCreateInfo->slots);
    pTrav->dsID = *pDescriptorSet;
    pTrav->numSlots = pCreateInfo->slots;
    pTrav->pNextDS = NULL;
    pTrav->updateActive = XGL_FALSE;
    initDS(pTrav);
    pthread_mutex_unlock(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglBeginDescriptorSetUpdate(XGL_DESCRIPTOR_SET descriptorSet)
{
    DS_LL_HEAD* pDS = getDS(descriptorSet);
    if (!pDS) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "Specified Descriptor Set %p does not exist!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_INVALID_DS, "DS", str);
    }
    else {
        pDS->updateActive = XGL_TRUE;
    }
    nextTable.BeginDescriptorSetUpdate(descriptorSet);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglEndDescriptorSetUpdate(XGL_DESCRIPTOR_SET descriptorSet)
{
    if (!dsUpdate(descriptorSet)) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "You must call xglBeginDescriptorSetUpdate(%p) before this call to xglEndDescriptorSetUpdate()!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_END_WITHOUT_BEGIN, "DS", str);
    }
    else {
        DS_LL_HEAD* pDS = getDS(descriptorSet);
        if (!pDS) {
            char str[1024];
            sprintf(str, "Specified Descriptor Set %p does not exist!", (void*)descriptorSet);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_INVALID_DS, "DS", str);
        }
        else {
            pDS->updateActive = XGL_FALSE;
        }
    }
    nextTable.EndDescriptorSetUpdate(descriptorSet);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachSamplerDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_SAMPLER* pSamplers)
{
    if (!dsUpdate(descriptorSet)) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "You must call xglBeginDescriptorSetUpdate(%p) before this call to xglAttachSamplerDescriptors()!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_ATTACH_WITHOUT_BEGIN, "DS", str);
    }
    else {
        if (!dsSamplerMapping(descriptorSet, startSlot, slotCount, pSamplers)) {
            char str[1024];
            sprintf(str, "Unable to attach sampler descriptors to DS %p!", (void*)descriptorSet);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_SAMPLE_ATTACH_FAILED, "DS", str);
        }
    }
    nextTable.AttachSamplerDescriptors(descriptorSet, startSlot, slotCount, pSamplers);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachImageViewDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews)
{
    if (!dsUpdate(descriptorSet)) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "You must call xglBeginDescriptorSetUpdate(%p) before this call to xglAttachSamplerDescriptors()!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_ATTACH_WITHOUT_BEGIN, "DS", str);
    }
    else {
        if (!dsImageMapping(descriptorSet, startSlot, slotCount, pImageViews)) {
            char str[1024];
            sprintf(str, "Unable to attach image view descriptors to DS %p!", (void*)descriptorSet);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_IMAGE_ATTACH_FAILED, "DS", str);
        }
    }
    nextTable.AttachImageViewDescriptors(descriptorSet, startSlot, slotCount, pImageViews);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachMemoryViewDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews)
{
    if (!dsUpdate(descriptorSet)) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "You must call xglBeginDescriptorSetUpdate(%p) before this call to xglAttachSamplerDescriptors()!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_ATTACH_WITHOUT_BEGIN, "DS", str);
    }
    else {
        if (!dsMemMapping(descriptorSet, startSlot, slotCount, pMemViews)) {
            char str[1024];
            sprintf(str, "Unable to attach memory view descriptors to DS %p!", (void*)descriptorSet);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_MEMORY_ATTACH_FAILED, "DS", str);
        }
    }
    nextTable.AttachMemoryViewDescriptors(descriptorSet, startSlot, slotCount, pMemViews);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachNestedDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets)
{
    if (!dsUpdate(descriptorSet)) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "You must call xglBeginDescriptorSetUpdate(%p) before this call to xglAttachSamplerDescriptors()!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_ATTACH_WITHOUT_BEGIN, "DS", str);
    }
    nextTable.AttachNestedDescriptors(descriptorSet, startSlot, slotCount, pNestedDescriptorSets);
}

// TODO : Does xglBeginDescriptorSetUpdate() have to be called before this function?
XGL_LAYER_EXPORT XGL_VOID XGLAPI xglClearDescriptorSetSlots(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount)
{
    if (!dsUpdate(descriptorSet)) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "You must call xglBeginDescriptorSetUpdate(%p) before this call to xglClearDescriptorSetSlots()!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_DS_ATTACH_WITHOUT_BEGIN, "DS", str);
    }
    if (!clearDS(descriptorSet, startSlot, slotCount)) {
        // TODO : This is where we should flag a REAL error
        char str[1024];
        sprintf(str, "Unable to perform xglClearDescriptorSetSlots(%p, %u, %u) call!", descriptorSet, startSlot, slotCount);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_CLEAR_DS_FAILED, "DS", str);
    }
    nextTable.ClearDescriptorSetSlots(descriptorSet, startSlot, slotCount);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateViewportState(XGL_DEVICE device, const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo, XGL_VIEWPORT_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateViewportState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (PIPELINE_LL_HEADER*)pCreateInfo, XGL_STATE_BIND_VIEWPORT);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateRasterState(XGL_DEVICE device, const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo, XGL_RASTER_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateRasterState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (PIPELINE_LL_HEADER*)pCreateInfo, XGL_STATE_BIND_RASTER);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateMsaaState(XGL_DEVICE device, const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo, XGL_MSAA_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateMsaaState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (PIPELINE_LL_HEADER*)pCreateInfo, XGL_STATE_BIND_MSAA);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorBlendState(XGL_DEVICE device, const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo, XGL_COLOR_BLEND_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateColorBlendState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (PIPELINE_LL_HEADER*)pCreateInfo, XGL_STATE_BIND_COLOR_BLEND);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilState(XGL_DEVICE device, const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDepthStencilState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (PIPELINE_LL_HEADER*)pCreateInfo, XGL_STATE_BIND_DEPTH_STENCIL);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer)
{
    XGL_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    XGL_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    XGL_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindPipeline(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline)
{
    if (getPipeline(pipeline)) {
        lastBoundPipeline = pipeline;
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pipeline, 0, DRAWSTATE_INVALID_PIPELINE, "DS", str);
    }
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindPipelineDelta(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta)
{
    nextTable.CmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_STATE_OBJECT state)
{
    setLastBoundDynamicState(state, stateBindPoint);
    nextTable.CmdBindStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindDescriptorSet(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT index, XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT slotOffset)
{
    if (getDS(descriptorSet)) {
        assert(index < XGL_MAX_DESCRIPTOR_SETS);
        if (dsUpdate(descriptorSet)) {
            char str[1024];
            sprintf(str, "You must call xglEndDescriptorSetUpdate(%p) before this call to xglCmdBindDescriptorSet()!", (void*)descriptorSet);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_BINDING_DS_NO_END_UPDATE, "DS", str);
        }
        pthread_mutex_lock(&globalLock);
        lastBoundDS[index] = descriptorSet;
        lastBoundSlotOffset[index] = slotOffset;
        pthread_mutex_unlock(&globalLock);
        char str[1024];
        sprintf(str, "DS %p bound to DS index %u on pipeline %s", (void*)descriptorSet, index, string_XGL_PIPELINE_BIND_POINT(pipelineBindPoint));
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_NONE, "DS", str);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to bind DS %p that doesn't exist!", (void*)descriptorSet);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_INVALID_DS, "DS", str);
    }
    nextTable.CmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, index, descriptorSet, slotOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindDynamicMemoryView(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView)
{
    nextTable.CmdBindDynamicMemoryView(cmdBuffer, pipelineBindPoint, pMemView);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindVertexData(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT binding)
{
    lastVtxBinding = binding;
    nextTable.CmdBindVertexData(cmdBuffer, mem, offset, binding);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindIndexData(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    nextTable.CmdBindIndexData(cmdBuffer, mem, offset, indexType);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdPrepareMemoryRegions(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_MEMORY_STATE_TRANSITION* pStateTransitions)
{
    nextTable.CmdPrepareMemoryRegions(cmdBuffer, transitionCount, pStateTransitions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdPrepareImages(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_IMAGE_STATE_TRANSITION* pStateTransitions)
{
    nextTable.CmdPrepareImages(cmdBuffer, transitionCount, pStateTransitions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDraw(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstVertex, XGL_UINT vertexCount, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    char str[1024];
    sprintf(str, "xglCmdDraw() call #%lu, reporting DS state:", drawCount[DRAW]++);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
    synchAndPrintDSConfig();
    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndexed(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstIndex, XGL_UINT indexCount, XGL_INT vertexOffset, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    char str[1024];
    sprintf(str, "xglCmdDrawIndexed() call #%lu, reporting DS state:", drawCount[DRAW_INDEXED]++);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
    synchAndPrintDSConfig();
    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride)
{
    char str[1024];
    sprintf(str, "xglCmdDrawIndirect() call #%lu, reporting DS state:", drawCount[DRAW_INDIRECT]++);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
    synchAndPrintDSConfig();
    nextTable.CmdDrawIndirect(cmdBuffer, mem, offset, count, stride);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride)
{
    char str[1024];
    sprintf(str, "xglCmdDrawIndexedIndirect() call #%lu, reporting DS state:", drawCount[DRAW_INDEXED_INDIRECT]++);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
    synchAndPrintDSConfig();
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, mem, offset, count, stride);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDispatch(XGL_CMD_BUFFER cmdBuffer, XGL_UINT x, XGL_UINT y, XGL_UINT z)
{
    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    nextTable.CmdDispatchIndirect(cmdBuffer, mem, offset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_COPY* pRegions)
{
    nextTable.CmdCopyMemory(cmdBuffer, srcMem, destMem, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_IMAGE_COPY* pRegions)
{
    nextTable.CmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyMemoryToImage(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    nextTable.CmdCopyMemoryToImage(cmdBuffer, srcMem, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyImageToMemory(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    nextTable.CmdCopyImageToMemory(cmdBuffer, srcImage, destMem, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout)
{
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdUpdateMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const XGL_UINT32* pData)
{
    nextTable.CmdUpdateMemory(cmdBuffer, destMem, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdFillMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, XGL_UINT32 data)
{
    nextTable.CmdFillMemory(cmdBuffer, destMem, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_FLOAT color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    nextTable.CmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearColorImageRaw(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_UINT32 color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    nextTable.CmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, XGL_FLOAT depth, XGL_UINT32 stencil, XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    nextTable.CmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
    nextTable.CmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdSetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event, XGL_SET_EVENT pipeEvent)
{
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    nextTable.CmdResetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdWaitEvents(XGL_CMD_BUFFER cmdBuffer, const XGL_EVENT_WAIT_INFO* pWaitInfo)
{
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdPipelineBarrier( XGL_CMD_BUFFER cmdBuffer, const XGL_PIPELINE_BARRIER* pBarrier)
{
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdMemoryAtomic(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_UINT64 srcData, XGL_ATOMIC_OP atomicOp)
{
    nextTable.CmdMemoryAtomic(cmdBuffer, destMem, destOffset, srcData, atomicOp);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot, XGL_FLAGS flags)
{
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot)
{
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount)
{
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdWriteTimestamp(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset)
{
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destMem, destOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdInitAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, const XGL_UINT32* pData)
{
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdLoadAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY srcMem, XGL_GPU_SIZE srcOffset)
{
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcMem, srcOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdSaveAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset)
{
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destMem, destOffset);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel)
{
    XGL_RESULT result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData)
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(XGL_DEVICE device, XGL_INT msgCode, XGL_DBG_MSG_FILTER filter)
{
    XGL_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(XGL_BASE_OBJECT object, XGL_SIZE tagSize, const XGL_VOID* pTag)
{
    XGL_RESULT result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.DbgSetGlobalOption(dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetDeviceOption(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDbgMarkerBegin(XGL_CMD_BUFFER cmdBuffer, const XGL_CHAR* pMarker)
{
    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDbgMarkerEnd(XGL_CMD_BUFFER cmdBuffer)
{
    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initDrawState);
    XGL_RESULT result = nextTable.WsiX11AssociateConnection((XGL_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(XGL_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, XGL_UINT64* pMsc)
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

XGL_VOID drawStateDumpDotFile(char* outFileName)
{
    dumpDotFile(outFileName);
}

XGL_VOID drawStateDumpPngFile(char* outFileName)
{
    char dotExe[32] = "/usr/bin/dot";
    if( access(dotExe, X_OK) != -1) {
        dumpDotFile("/tmp/tmp.dot");
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
}

XGL_LAYER_EXPORT XGL_VOID* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    void *addr;

    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initDrawState);

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
