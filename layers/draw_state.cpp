/*
 * Vulkan
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
#include <unordered_map>

#include "loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "vk_struct_graphviz_helper.h"
#pragma GCC diagnostic warning "-Wwrite-strings"
#include "vk_struct_size_helper.h"
#include "draw_state.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"
#include "layers_msg.h"

unordered_map<VK_SAMPLER, SAMPLER_NODE*> sampleMap;
unordered_map<VK_IMAGE_VIEW, IMAGE_NODE*> imageMap;
unordered_map<VK_BUFFER_VIEW, BUFFER_NODE*> bufferMap;
unordered_map<VK_DYNAMIC_STATE_OBJECT, DYNAMIC_STATE_NODE*> dynamicStateMap;
unordered_map<VK_PIPELINE, PIPELINE_NODE*> pipelineMap;
unordered_map<VK_DESCRIPTOR_POOL, POOL_NODE*> poolMap;
unordered_map<VK_DESCRIPTOR_SET, SET_NODE*> setMap;
unordered_map<VK_DESCRIPTOR_SET_LAYOUT, LAYOUT_NODE*> layoutMap;
// Map for layout chains
unordered_map<VK_CMD_BUFFER, GLOBAL_CB_NODE*> cmdBufferMap;
unordered_map<VK_RENDER_PASS, VK_RENDER_PASS_CREATE_INFO*> renderPassMap;
unordered_map<VK_FRAMEBUFFER, VK_FRAMEBUFFER_CREATE_INFO*> frameBufferMap;

static VK_LAYER_DISPATCH_TABLE nextTable;
static VK_BASE_LAYER_OBJECT *pCurObj;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);
// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;
#define MAX_TID 513
static loader_platform_thread_id g_tidMapping[MAX_TID] = {0};
static uint32_t g_maxTID = 0;
// Map actual TID to an index value and return that index
//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs
static uint32_t getTIDIndex() {
    loader_platform_thread_id tid = loader_platform_get_thread_id();
    for (uint32_t i = 0; i < g_maxTID; i++) {
        if (tid == g_tidMapping[i])
            return i;
    }
    // Don't yet have mapping, set it and return newly set index
    uint32_t retVal = (uint32_t) g_maxTID;
    g_tidMapping[g_maxTID++] = tid;
    assert(g_maxTID < MAX_TID);
    return retVal;
}
// Return a string representation of CMD_TYPE enum
static string cmdTypeToString(CMD_TYPE cmd)
{
    switch (cmd)
    {
        case CMD_BINDPIPELINE:
            return "CMD_BINDPIPELINE";
        case CMD_BINDPIPELINEDELTA:
            return "CMD_BINDPIPELINEDELTA";
        case CMD_BINDDYNAMICSTATEOBJECT:
            return "CMD_BINDDYNAMICSTATEOBJECT";
        case CMD_BINDDESCRIPTORSETS:
            return "CMD_BINDDESCRIPTORSETS";
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
        case CMD_BLITIMAGE:
            return "CMD_BLITIMAGE";
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
        case CMD_DBGMARKERBEGIN:
            return "CMD_DBGMARKERBEGIN";
        case CMD_DBGMARKEREND:
            return "CMD_DBGMARKEREND";
        default:
            return "UNKNOWN";
    }
}
// Block of code at start here for managing/tracking Pipeline state that this layer cares about
// Just track 2 shaders for now
#define VK_NUM_GRAPHICS_SHADERS VK_SHADER_STAGE_COMPUTE
#define MAX_SLOTS 2048
#define NUM_COMMAND_BUFFERS_TO_DISPLAY 10

static uint64_t g_drawCount[NUM_DRAW_TYPES] = {0, 0, 0, 0};

// TODO : Should be tracking lastBound per cmdBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
// Track the last cmd buffer touched by this thread
static VK_CMD_BUFFER    g_lastCmdBuffer[MAX_TID] = {NULL};
// Track the last group of CBs touched for displaying to dot file
static GLOBAL_CB_NODE*   g_pLastTouchedCB[NUM_COMMAND_BUFFERS_TO_DISPLAY] = {NULL};
static uint32_t g_lastTouchedCBIndex = 0;
// Track the last global DrawState of interest touched by any thread
static GLOBAL_CB_NODE*        g_lastGlobalCB = NULL;
static PIPELINE_NODE*         g_lastBoundPipeline = NULL;
static DYNAMIC_STATE_NODE*    g_lastBoundDynamicState[VK_NUM_STATE_BIND_POINT] = {NULL};
static VK_DESCRIPTOR_SET     g_lastBoundDescriptorSet = NULL;
#define MAX_BINDING 0xFFFFFFFF // Default vtxBinding value in CB Node to identify if no vtxBinding set

//static DYNAMIC_STATE_NODE* g_pDynamicStateHead[VK_NUM_STATE_BIND_POINT] = {0};

static void insertDynamicState(const VK_DYNAMIC_STATE_OBJECT state, const GENERIC_HEADER* pCreateInfo, VK_STATE_BIND_POINT bindPoint)
{
    VK_DYNAMIC_VP_STATE_CREATE_INFO* pVPCI = NULL;
    size_t scSize = 0;
    size_t vpSize = 0;
    loader_platform_thread_lock_mutex(&globalLock);
    DYNAMIC_STATE_NODE* pStateNode = new DYNAMIC_STATE_NODE;
    pStateNode->stateObj = state;
    switch (pCreateInfo->sType) {
        case VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VK_DYNAMIC_VP_STATE_CREATE_INFO));
            pVPCI = (VK_DYNAMIC_VP_STATE_CREATE_INFO*)pCreateInfo;
            pStateNode->create_info.vpci.pScissors = new VK_RECT[pStateNode->create_info.vpci.viewportAndScissorCount];
            pStateNode->create_info.vpci.pViewports = new VK_VIEWPORT[pStateNode->create_info.vpci.viewportAndScissorCount];
            scSize = pVPCI->viewportAndScissorCount * sizeof(VK_RECT);
            vpSize = pVPCI->viewportAndScissorCount * sizeof(VK_VIEWPORT);
            memcpy((void*)pStateNode->create_info.vpci.pScissors, pVPCI->pScissors, scSize);
            memcpy((void*)pStateNode->create_info.vpci.pViewports, pVPCI->pViewports, vpSize);
            break;
        case VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VK_DYNAMIC_RS_STATE_CREATE_INFO));
            break;
        case VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VK_DYNAMIC_CB_STATE_CREATE_INFO));
            break;
        case VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VK_DYNAMIC_DS_STATE_CREATE_INFO));
            break;
        default:
            assert(0);
            break;
    }
    pStateNode->pCreateInfo = (GENERIC_HEADER*)&pStateNode->create_info.cbci;
    dynamicStateMap[state] = pStateNode;
    loader_platform_thread_unlock_mutex(&globalLock);
}
// Free all allocated nodes for Dynamic State objs
static void deleteDynamicState()
{
    for (unordered_map<VK_DYNAMIC_STATE_OBJECT, DYNAMIC_STATE_NODE*>::iterator ii=dynamicStateMap.begin(); ii!=dynamicStateMap.end(); ++ii) {
        if (VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO == (*ii).second->create_info.vpci.sType) {
            delete[] (*ii).second->create_info.vpci.pScissors;
            delete[] (*ii).second->create_info.vpci.pViewports;
        }
        delete (*ii).second;
    }
}
// Free all sampler nodes
static void deleteSamplers()
{
    for (unordered_map<VK_SAMPLER, SAMPLER_NODE*>::iterator ii=sampleMap.begin(); ii!=sampleMap.end(); ++ii) {
        delete (*ii).second;
    }
}
static VK_IMAGE_VIEW_CREATE_INFO* getImageViewCreateInfo(VK_IMAGE_VIEW view)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (imageMap.find(view) == imageMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    else {
        loader_platform_thread_unlock_mutex(&globalLock);
        return &imageMap[view]->createInfo;
    }
}
// Free all image nodes
static void deleteImages()
{
    for (unordered_map<VK_IMAGE_VIEW, IMAGE_NODE*>::iterator ii=imageMap.begin(); ii!=imageMap.end(); ++ii) {
        delete (*ii).second;
    }
}
static VK_BUFFER_VIEW_CREATE_INFO* getBufferViewCreateInfo(VK_BUFFER_VIEW view)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (bufferMap.find(view) == bufferMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    else {
        loader_platform_thread_unlock_mutex(&globalLock);
        return &bufferMap[view]->createInfo;
    }
}
// Free all buffer nodes
static void deleteBuffers()
{
    for (unordered_map<VK_BUFFER_VIEW, BUFFER_NODE*>::iterator ii=bufferMap.begin(); ii!=bufferMap.end(); ++ii) {
        delete (*ii).second;
    }
}
static GLOBAL_CB_NODE* getCBNode(VK_CMD_BUFFER cb);

static void updateCBTracking(VK_CMD_BUFFER cb)
{
    g_lastCmdBuffer[getTIDIndex()] = cb;
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    loader_platform_thread_lock_mutex(&globalLock);
    g_lastGlobalCB = pCB;
    // TODO : This is a dumb algorithm. Need smart LRU that drops off oldest
    for (uint32_t i = 0; i < NUM_COMMAND_BUFFERS_TO_DISPLAY; i++) {
        if (g_pLastTouchedCB[i] == pCB) {
            loader_platform_thread_unlock_mutex(&globalLock);
            return;
        }
    }
    g_pLastTouchedCB[g_lastTouchedCBIndex++] = pCB;
    g_lastTouchedCBIndex = g_lastTouchedCBIndex % NUM_COMMAND_BUFFERS_TO_DISPLAY;
    loader_platform_thread_unlock_mutex(&globalLock);
}

// Print the last bound dynamic state
static void printDynamicState(const VK_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        loader_platform_thread_lock_mutex(&globalLock);
        char str[4*1024];
        for (uint32_t i = 0; i < VK_NUM_STATE_BIND_POINT; i++) {
            if (pCB->lastBoundDynamicState[i]) {
                sprintf(str, "Reporting CreateInfo for currently bound %s object %p", string_VK_STATE_BIND_POINT((VK_STATE_BIND_POINT)i), pCB->lastBoundDynamicState[i]->stateObj);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pCB->lastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", str);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pCB->lastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", dynamic_display(pCB->lastBoundDynamicState[i]->pCreateInfo, "  ").c_str());
                break;
            }
            else {
                sprintf(str, "No dynamic state of type %s bound", string_VK_STATE_BIND_POINT((VK_STATE_BIND_POINT)i));
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", str);
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cb);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cb, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
}
// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_NODE* getPipeline(VK_PIPELINE pipeline)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (pipelineMap.find(pipeline) == pipelineMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return pipelineMap[pipeline];
}

// For given sampler, return a ptr to its Create Info struct, or NULL if sampler not found
static VK_SAMPLER_CREATE_INFO* getSamplerCreateInfo(const VK_SAMPLER sampler)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (sampleMap.find(sampler) == sampleMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return &sampleMap[sampler]->createInfo;
}

// Init the pipeline mapping info based on pipeline create info LL tree
//  Threading note : Calls to this function should wrapped in mutex
static void initPipeline(PIPELINE_NODE* pPipeline, const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo)
{
    // First init create info, we'll shadow the structs as we go down the tree
    // TODO : Validate that no create info is incorrectly replicated
    memcpy(&pPipeline->graphicsPipelineCI, pCreateInfo, sizeof(VK_GRAPHICS_PIPELINE_CREATE_INFO));
    GENERIC_HEADER* pTrav = (GENERIC_HEADER*)pCreateInfo->pNext;
    GENERIC_HEADER* pPrev = (GENERIC_HEADER*)&pPipeline->graphicsPipelineCI; // Hold prev ptr to tie chain of structs together
    size_t bufferSize = 0;
    VK_PIPELINE_VERTEX_INPUT_CREATE_INFO* pVICI = NULL;
    VK_PIPELINE_CB_STATE_CREATE_INFO*     pCBCI = NULL;
    VK_PIPELINE_SHADER_STAGE_CREATE_INFO* pTmpPSSCI = NULL;
    while (pTrav) {
        switch (pTrav->sType) {
            case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
                pTmpPSSCI = (VK_PIPELINE_SHADER_STAGE_CREATE_INFO*)pTrav;
                switch (pTmpPSSCI->shader.stage) {
                    case VK_SHADER_STAGE_VERTEX:
                        pPrev->pNext = &pPipeline->vsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->vsCI;
                        memcpy(&pPipeline->vsCI, pTmpPSSCI, sizeof(VK_PIPELINE_SHADER_STAGE_CREATE_INFO));
                        break;
                    case VK_SHADER_STAGE_TESS_CONTROL:
                        pPrev->pNext = &pPipeline->tcsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->tcsCI;
                        memcpy(&pPipeline->tcsCI, pTmpPSSCI, sizeof(VK_PIPELINE_SHADER_STAGE_CREATE_INFO));
                        break;
                    case VK_SHADER_STAGE_TESS_EVALUATION:
                        pPrev->pNext = &pPipeline->tesCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->tesCI;
                        memcpy(&pPipeline->tesCI, pTmpPSSCI, sizeof(VK_PIPELINE_SHADER_STAGE_CREATE_INFO));
                        break;
                    case VK_SHADER_STAGE_GEOMETRY:
                        pPrev->pNext = &pPipeline->gsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->gsCI;
                        memcpy(&pPipeline->gsCI, pTmpPSSCI, sizeof(VK_PIPELINE_SHADER_STAGE_CREATE_INFO));
                        break;
                    case VK_SHADER_STAGE_FRAGMENT:
                        pPrev->pNext = &pPipeline->fsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->fsCI;
                        memcpy(&pPipeline->fsCI, pTmpPSSCI, sizeof(VK_PIPELINE_SHADER_STAGE_CREATE_INFO));
                        break;
                    case VK_SHADER_STAGE_COMPUTE:
                        // TODO : Flag error, CS is specified through VK_COMPUTE_PIPELINE_CREATE_INFO
                        break;
                    default:
                        // TODO : Flag error
                        break;
                }
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
                pPrev->pNext = &pPipeline->vertexInputCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->vertexInputCI;
                memcpy((void*)&pPipeline->vertexInputCI, pTrav, sizeof(VK_PIPELINE_VERTEX_INPUT_CREATE_INFO));
                // Copy embedded ptrs
                pVICI = (VK_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pTrav;
                pPipeline->vtxBindingCount = pVICI->bindingCount;
                if (pPipeline->vtxBindingCount) {
                    pPipeline->pVertexBindingDescriptions = new VK_VERTEX_INPUT_BINDING_DESCRIPTION[pPipeline->vtxBindingCount];
                    bufferSize = pPipeline->vtxBindingCount * sizeof(VK_VERTEX_INPUT_BINDING_DESCRIPTION);
                    memcpy((void*)pPipeline->pVertexBindingDescriptions, ((VK_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pTrav)->pVertexAttributeDescriptions, bufferSize);
                }
                pPipeline->vtxAttributeCount = pVICI->attributeCount;
                if (pPipeline->vtxAttributeCount) {
                    pPipeline->pVertexAttributeDescriptions = new VK_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION[pPipeline->vtxAttributeCount];
                    bufferSize = pPipeline->vtxAttributeCount * sizeof(VK_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION);
                    memcpy((void*)pPipeline->pVertexAttributeDescriptions, ((VK_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pTrav)->pVertexAttributeDescriptions, bufferSize);
                }
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->iaStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->iaStateCI;
                memcpy((void*)&pPipeline->iaStateCI, pTrav, sizeof(VK_PIPELINE_IA_STATE_CREATE_INFO));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->tessStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->tessStateCI;
                memcpy((void*)&pPipeline->tessStateCI, pTrav, sizeof(VK_PIPELINE_TESS_STATE_CREATE_INFO));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->vpStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->vpStateCI;
                memcpy((void*)&pPipeline->vpStateCI, pTrav, sizeof(VK_PIPELINE_VP_STATE_CREATE_INFO));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->rsStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->rsStateCI;
                memcpy((void*)&pPipeline->rsStateCI, pTrav, sizeof(VK_PIPELINE_RS_STATE_CREATE_INFO));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->msStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->msStateCI;
                memcpy((void*)&pPipeline->msStateCI, pTrav, sizeof(VK_PIPELINE_MS_STATE_CREATE_INFO));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->cbStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->cbStateCI;
                memcpy((void*)&pPipeline->cbStateCI, pTrav, sizeof(VK_PIPELINE_CB_STATE_CREATE_INFO));
                // Copy embedded ptrs
                pCBCI = (VK_PIPELINE_CB_STATE_CREATE_INFO*)pTrav;
                pPipeline->attachmentCount = pCBCI->attachmentCount;
                if (pPipeline->attachmentCount) {
                    pPipeline->pAttachments = new VK_PIPELINE_CB_ATTACHMENT_STATE[pPipeline->attachmentCount];
                    bufferSize = pPipeline->attachmentCount * sizeof(VK_PIPELINE_CB_ATTACHMENT_STATE);
                    memcpy((void*)pPipeline->pAttachments, ((VK_PIPELINE_CB_STATE_CREATE_INFO*)pTrav)->pAttachments, bufferSize);
                }
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->dsStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->dsStateCI;
                memcpy((void*)&pPipeline->dsStateCI, pTrav, sizeof(VK_PIPELINE_DS_STATE_CREATE_INFO));
                break;
            default:
                assert(0);
                break;
        }
        pTrav = (GENERIC_HEADER*)pTrav->pNext;
    }
    pipelineMap[pPipeline->pipeline] = pPipeline;
}
// Free the Pipeline nodes
static void deletePipelines()
{
    for (unordered_map<VK_PIPELINE, PIPELINE_NODE*>::iterator ii=pipelineMap.begin(); ii!=pipelineMap.end(); ++ii) {
        if ((*ii).second->pVertexBindingDescriptions) {
            delete[] (*ii).second->pVertexBindingDescriptions;
        }
        if ((*ii).second->pVertexAttributeDescriptions) {
            delete[] (*ii).second->pVertexAttributeDescriptions;
        }
        if ((*ii).second->pAttachments) {
            delete[] (*ii).second->pAttachments;
        }
        delete (*ii).second;
    }
}
// For given pipeline, return number of MSAA samples, or one if MSAA disabled
static uint32_t getNumSamples(const VK_PIPELINE pipeline)
{
    PIPELINE_NODE* pPipe = pipelineMap[pipeline];
    if (VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO == pPipe->msStateCI.sType) {
        if (pPipe->msStateCI.multisampleEnable)
            return pPipe->msStateCI.samples;
    }
    return 1;
}
// Validate state related to the PSO
static void validatePipelineState(const GLOBAL_CB_NODE* pCB, const VK_PIPELINE_BIND_POINT pipelineBindPoint, const VK_PIPELINE pipeline)
{
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
        // Verify that any MSAA request in PSO matches sample# in bound FB
        uint32_t psoNumSamples = getNumSamples(pipeline);
        if (pCB->activeRenderPass) {
            VK_RENDER_PASS_CREATE_INFO* pRPCI = renderPassMap[pCB->activeRenderPass];
            VK_FRAMEBUFFER_CREATE_INFO* pFBCI = frameBufferMap[pCB->framebuffer];
            if (psoNumSamples != pFBCI->sampleCount) {
                char str[1024];
                sprintf(str, "Num samples mismatche! Binding PSO (%p) with %u samples while current RenderPass (%p) uses FB (%p) with %u samples!", (void*)pipeline, psoNumSamples, (void*)pCB->activeRenderPass, (void*)pCB->framebuffer, pFBCI->sampleCount);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pipeline, 0, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS", str);
            }
        } else {
            // TODO : I believe it's an error if we reach this point and don't have an activeRenderPass
            //   Verify and flag error as appropriate
        }
        // TODO : Add more checks here
    } else {
        // TODO : Validate non-gfx pipeline updates
    }
}

// Block of code at start here specifically for managing/tracking DSs

// Return Pool node ptr for specified pool or else NULL
static POOL_NODE* getPoolNode(VK_DESCRIPTOR_POOL pool)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (poolMap.find(pool) == poolMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return poolMap[pool];
}
// Return Set node ptr for specified set or else NULL
static SET_NODE* getSetNode(VK_DESCRIPTOR_SET set)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (setMap.find(set) == setMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return setMap[set];
}

// Return VK_TRUE if DS Exists and is within an vkBeginDescriptorPoolUpdate() call sequence, otherwise VK_FALSE
static bool32_t dsUpdateActive(VK_DESCRIPTOR_SET ds)
{
    // Note, both "get" functions use global mutex so this guy does not
    SET_NODE* pTrav = getSetNode(ds);
    if (pTrav) {
        POOL_NODE* pPool = getPoolNode(pTrav->pool);
        if (pPool) {
            return pPool->updateActive;
        }
    }
    return VK_FALSE;
}

static LAYOUT_NODE* getLayoutNode(const VK_DESCRIPTOR_SET_LAYOUT layout) {
    loader_platform_thread_lock_mutex(&globalLock);
    if (layoutMap.find(layout) == layoutMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return layoutMap[layout];
}

// For given update struct, return binding
static uint32_t getUpdateBinding(const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            return ((VK_UPDATE_SAMPLERS*)pUpdateStruct)->binding;
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            return ((VK_UPDATE_SAMPLER_TEXTURES*)pUpdateStruct)->binding;
        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
            return ((VK_UPDATE_IMAGES*)pUpdateStruct)->binding;
        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
            return ((VK_UPDATE_BUFFERS*)pUpdateStruct)->binding;
        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
            return ((VK_UPDATE_AS_COPY*)pUpdateStruct)->binding;
        default:
            // TODO : Flag specific error for this case
            assert(0);
            return 0;
    }
}
// Return count for given update struct
static uint32_t getUpdateArrayIndex(const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            return (((VK_UPDATE_SAMPLERS*)pUpdateStruct)->arrayIndex);
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            return (((VK_UPDATE_SAMPLER_TEXTURES*)pUpdateStruct)->arrayIndex);
        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
            return (((VK_UPDATE_IMAGES*)pUpdateStruct)->arrayIndex);
        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
            return (((VK_UPDATE_BUFFERS*)pUpdateStruct)->arrayIndex);
        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
            // TODO : Need to understand this case better and make sure code is correct
            return (((VK_UPDATE_AS_COPY*)pUpdateStruct)->arrayElement);
        default:
            // TODO : Flag specific error for this case
            assert(0);
            return 0;
    }
}
// Return count for given update struct
static uint32_t getUpdateCount(const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            return (((VK_UPDATE_SAMPLERS*)pUpdateStruct)->count);
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            return (((VK_UPDATE_SAMPLER_TEXTURES*)pUpdateStruct)->count);
        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
            return (((VK_UPDATE_IMAGES*)pUpdateStruct)->count);
        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
            return (((VK_UPDATE_BUFFERS*)pUpdateStruct)->count);
        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
            // TODO : Need to understand this case better and make sure code is correct
            return (((VK_UPDATE_AS_COPY*)pUpdateStruct)->count);
        default:
            // TODO : Flag specific error for this case
            assert(0);
            return 0;
    }
}
// For given Layout Node and binding, return index where that binding begins
static uint32_t getBindingStartIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i<binding; i++) {
        offsetIndex += pLayout->createInfo.pBinding[i].count;
    }
    return offsetIndex;
}
// For given layout node and binding, return last index that is updated
static uint32_t getBindingEndIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i<=binding; i++) {
        offsetIndex += pLayout->createInfo.pBinding[i].count;
    }
    return offsetIndex-1;
}
// For given layout and update, return the first overall index of the layout that is update
static uint32_t getUpdateStartIndex(const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct)
{
    return (getBindingStartIndex(pLayout, getUpdateBinding(pUpdateStruct))+getUpdateArrayIndex(pUpdateStruct));
}
// For given layout and update, return the last overall index of the layout that is update
static uint32_t getUpdateEndIndex(const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct)
{
    return (getBindingStartIndex(pLayout, getUpdateBinding(pUpdateStruct))+getUpdateArrayIndex(pUpdateStruct)+getUpdateCount(pUpdateStruct)-1);
}
// Verify that the descriptor type in the update struct matches what's expected by the layout
static bool32_t validateUpdateType(const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct)
{
    // First get actual type of update
    VK_DESCRIPTOR_TYPE actualType;
    uint32_t i = 0;
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            actualType = VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            actualType = VK_DESCRIPTOR_TYPE_SAMPLER_TEXTURE;
            break;
        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
            actualType = ((VK_UPDATE_IMAGES*)pUpdateStruct)->descriptorType;
            break;
        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
            actualType = ((VK_UPDATE_BUFFERS*)pUpdateStruct)->descriptorType;
            break;
        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
            actualType = ((VK_UPDATE_AS_COPY*)pUpdateStruct)->descriptorType;
            break;
        default:
            // TODO : Flag specific error for this case
            return 0;
    }
    for (i = getUpdateStartIndex(pLayout, pUpdateStruct); i <= getUpdateEndIndex(pLayout, pUpdateStruct); i++) {
        if (pLayout->pTypes[i] != actualType)
            return 0;
    }
    return 1;
}
// Determine the update type, allocate a new struct of that type, shadow the given pUpdate
//   struct into the new struct and return ptr to shadow struct cast as GENERIC_HEADER
// NOTE : Calls to this function should be wrapped in mutex
static GENERIC_HEADER* shadowUpdateNode(GENERIC_HEADER* pUpdate)
{
    GENERIC_HEADER* pNewNode = NULL;
    VK_UPDATE_SAMPLERS* pUS = NULL;
    VK_UPDATE_SAMPLER_TEXTURES* pUST = NULL;
    VK_UPDATE_BUFFERS* pUB = NULL;
    VK_UPDATE_IMAGES* pUI = NULL;
    VK_UPDATE_AS_COPY* pUAC = NULL;
    size_t array_size = 0;
    size_t base_array_size = 0;
    size_t total_array_size = 0;
    size_t baseBuffAddr = 0;
    VK_IMAGE_VIEW_ATTACH_INFO** ppLocalImageViews = NULL;
    VK_BUFFER_VIEW_ATTACH_INFO** ppLocalBufferViews = NULL;
    char str[1024];
    switch (pUpdate->sType)
    {
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            pUS = new VK_UPDATE_SAMPLERS;
            pNewNode = (GENERIC_HEADER*)pUS;
            memcpy(pUS, pUpdate, sizeof(VK_UPDATE_SAMPLERS));
            pUS->pSamplers = new VK_SAMPLER[pUS->count];
            array_size = sizeof(VK_SAMPLER) * pUS->count;
            memcpy((void*)pUS->pSamplers, ((VK_UPDATE_SAMPLERS*)pUpdate)->pSamplers, array_size);
            break;
        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            pUST = new VK_UPDATE_SAMPLER_TEXTURES;
            pNewNode = (GENERIC_HEADER*)pUST;
            memcpy(pUST, pUpdate, sizeof(VK_UPDATE_SAMPLER_TEXTURES));
            pUST->pSamplerImageViews = new VK_SAMPLER_IMAGE_VIEW_INFO[pUST->count];
            array_size = sizeof(VK_SAMPLER_IMAGE_VIEW_INFO) * pUST->count;
            memcpy((void*)pUST->pSamplerImageViews, ((VK_UPDATE_SAMPLER_TEXTURES*)pUpdate)->pSamplerImageViews, array_size);
            for (uint32_t i = 0; i < pUST->count; i++) {
                VK_IMAGE_VIEW_ATTACH_INFO** ppIV = (VK_IMAGE_VIEW_ATTACH_INFO**)&pUST->pSamplerImageViews[i].pImageView;
                *ppIV = new VK_IMAGE_VIEW_ATTACH_INFO;
                memcpy((void*)*ppIV, ((VK_UPDATE_SAMPLER_TEXTURES*)pUpdate)->pSamplerImageViews[i].pImageView, sizeof(VK_IMAGE_VIEW_ATTACH_INFO));
            }
            break;
        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
            pUI = new VK_UPDATE_IMAGES;
            pNewNode = (GENERIC_HEADER*)pUI;
            memcpy(pUI, pUpdate, sizeof(VK_UPDATE_IMAGES));
            pUI->pImageViews = new VK_IMAGE_VIEW_ATTACH_INFO[pUI->count];
            array_size = (sizeof(VK_IMAGE_VIEW_ATTACH_INFO) * pUI->count);
            memcpy((void*)pUI->pImageViews, ((VK_UPDATE_IMAGES*)pUpdate)->pImageViews, array_size);
            break;
        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
            pUB = new VK_UPDATE_BUFFERS;
            pNewNode = (GENERIC_HEADER*)pUB;
            memcpy(pUB, pUpdate, sizeof(VK_UPDATE_BUFFERS));
            pUB->pBufferViews = new VK_BUFFER_VIEW_ATTACH_INFO[pUB->count];
            array_size = (sizeof(VK_BUFFER_VIEW_ATTACH_INFO) * pUB->count);
            memcpy((void*)pUB->pBufferViews, ((VK_UPDATE_BUFFERS*)pUpdate)->pBufferViews, array_size);
            break;
        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
            pUAC = new VK_UPDATE_AS_COPY;
            pUpdate = (GENERIC_HEADER*)pUAC;
            memcpy(pUAC, pUpdate, sizeof(VK_UPDATE_AS_COPY));
            break;
        default:
            sprintf(str, "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VK_STRUCTURE_TYPE(pUpdate->sType), pUpdate->sType);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS", str);
            return NULL;
    }
    // Make sure that pNext for the end of shadow copy is NULL
    pNewNode->pNext = NULL;
    return pNewNode;
}
// For given ds, update its mapping based on ppUpdateArray
static void dsUpdate(VK_DESCRIPTOR_SET ds, uint32_t updateCount, const void** ppUpdateArray)
{
    SET_NODE* pSet = getSetNode(ds);
    loader_platform_thread_lock_mutex(&globalLock);
    g_lastBoundDescriptorSet = pSet->set;
    LAYOUT_NODE* pLayout = NULL;
    VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pLayoutCI = NULL;
    // TODO : If pCIList is NULL, flag error
    // Perform all updates
    for (uint32_t i = 0; i < updateCount; i++) {
        GENERIC_HEADER* pUpdate = (GENERIC_HEADER*)ppUpdateArray[i];
        pLayout = pSet->pLayout;
        // Make sure that binding is within bounds
        if (pLayout->createInfo.count < getUpdateBinding(pUpdate)) {
            char str[1024];
            sprintf(str, "Descriptor Set %p does not have binding to match update binding %u for update type %s!", ds, getUpdateBinding(pUpdate), string_VK_STRUCTURE_TYPE(pUpdate->sType));
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS", str);
        }
        else {
            // Next verify that update falls within size of given binding
            if (getBindingEndIndex(pLayout, getUpdateBinding(pUpdate)) < getUpdateEndIndex(pLayout, pUpdate)) {
                char str[48*1024]; // TODO : Keep count of layout CI structs and size this string dynamically based on that count
                pLayoutCI = &pLayout->createInfo;
                string DSstr = vk_print_vk_descriptor_set_layout_create_info(pLayoutCI, "{DS}    ");
                sprintf(str, "Descriptor update type of %s is out of bounds for matching binding %u in Layout w/ CI:\n%s!", string_VK_STRUCTURE_TYPE(pUpdate->sType), getUpdateBinding(pUpdate), DSstr.c_str());
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS", str);
            }
            else { // TODO : should we skip update on a type mismatch or force it?
                // Layout bindings match w/ update ok, now verify that update is of the right type
                if (!validateUpdateType(pLayout, pUpdate)) {
                    char str[1024];
                    sprintf(str, "Descriptor update type of %s does not match overlapping binding type!", string_VK_STRUCTURE_TYPE(pUpdate->sType));
                    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS", str);
                }
                else {
                    // Save the update info
                    // TODO : Info message that update successful
                    // Create new update struct for this set's shadow copy
                    GENERIC_HEADER* pNewNode = shadowUpdateNode(pUpdate);
                    if (NULL == pNewNode) {
                        char str[1024];
                        sprintf(str, "Out of memory while attempting to allocate UPDATE struct in vkUpdateDescriptors()");
                        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
                    }
                    else {
                        // Insert shadow node into LL of updates for this set
                        pNewNode->pNext = pSet->pUpdateStructs;
                        pSet->pUpdateStructs = pNewNode;
                        // Now update appropriate descriptor(s) to point to new Update node
                        for (uint32_t j = getUpdateStartIndex(pLayout, pUpdate); j <= getUpdateEndIndex(pLayout, pUpdate); j++) {
                            assert(j<pSet->descriptorCount);
                            pSet->ppDescriptors[j] = pNewNode;
                        }
                    }
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
}
// Free the shadowed update node for this Set
// NOTE : Calls to this function should be wrapped in mutex
static void freeShadowUpdateTree(SET_NODE* pSet)
{
    GENERIC_HEADER* pShadowUpdate = pSet->pUpdateStructs;
    pSet->pUpdateStructs = NULL;
    GENERIC_HEADER* pFreeUpdate = pShadowUpdate;
    // Clear the descriptor mappings as they will now be invalid
    memset(pSet->ppDescriptors, 0, pSet->descriptorCount*sizeof(GENERIC_HEADER*));
    while(pShadowUpdate) {
        pFreeUpdate = pShadowUpdate;
        pShadowUpdate = (GENERIC_HEADER*)pShadowUpdate->pNext;
        uint32_t index = 0;
        VK_UPDATE_SAMPLERS* pUS = NULL;
        VK_UPDATE_SAMPLER_TEXTURES* pUST = NULL;
        VK_UPDATE_IMAGES* pUI = NULL;
        VK_UPDATE_BUFFERS* pUB = NULL;
        void** ppToFree = NULL;
        switch (pFreeUpdate->sType)
        {
            case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                pUS = (VK_UPDATE_SAMPLERS*)pFreeUpdate;
                if (pUS->pSamplers)
                    delete[] pUS->pSamplers;
                break;
            case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                pUST = (VK_UPDATE_SAMPLER_TEXTURES*)pFreeUpdate;
                if (pUST->pSamplerImageViews) {
                    for (index = 0; index < pUST->count; index++) {
                        if (pUST->pSamplerImageViews[index].pImageView) {
                            delete pUST->pSamplerImageViews[index].pImageView;
                        }
                    }
                    delete[] pUST->pSamplerImageViews;
                }
                break;
            case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
                pUI = (VK_UPDATE_IMAGES*)pFreeUpdate;
                if (pUI->pImageViews)
                    delete[] pUI->pImageViews;
                break;
            case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
                pUB = (VK_UPDATE_BUFFERS*)pFreeUpdate;
                if (pUB->pBufferViews)
                    delete[] pUB->pBufferViews;
                break;
            case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
                break;
            default:
                assert(0);
                break;
        }
        delete pFreeUpdate;
    }
}
// Free all DS Pools including their Sets & related sub-structs
// NOTE : Calls to this function should be wrapped in mutex
static void deletePools()
{
    for (unordered_map<VK_DESCRIPTOR_POOL, POOL_NODE*>::iterator ii=poolMap.begin(); ii!=poolMap.end(); ++ii) {
        SET_NODE* pSet = (*ii).second->pSets;
        SET_NODE* pFreeSet = pSet;
        while (pSet) {
            pFreeSet = pSet;
            pSet = pSet->pNext;
            // Freeing layouts handled in deleteLayouts() function
            // Free Update shadow struct tree
            freeShadowUpdateTree(pFreeSet);
            if (pFreeSet->ppDescriptors) {
                delete pFreeSet->ppDescriptors;
            }
            delete pFreeSet;
        }
        if ((*ii).second->createInfo.pTypeCount) {
            delete (*ii).second->createInfo.pTypeCount;
        }
        delete (*ii).second;
    }
}
// WARN : Once deleteLayouts() called, any layout ptrs in Pool/Set data structure will be invalid
// NOTE : Calls to this function should be wrapped in mutex
static void deleteLayouts()
{
    for (unordered_map<VK_DESCRIPTOR_SET_LAYOUT, LAYOUT_NODE*>::iterator ii=layoutMap.begin(); ii!=layoutMap.end(); ++ii) {
        LAYOUT_NODE* pLayout = (*ii).second;
        if (pLayout->createInfo.pBinding) {
            for (uint32_t i=0; i<pLayout->createInfo.count; i++) {
                if (pLayout->createInfo.pBinding[i].pImmutableSamplers)
                    delete[] pLayout->createInfo.pBinding[i].pImmutableSamplers;
            }
            delete[] pLayout->createInfo.pBinding;
        }
        if (pLayout->pTypes) {
            delete pLayout->pTypes;
        }
        delete pLayout;
    }
}
// Currently clearing a set is removing all previous updates to that set
//  TODO : Validate if this is correct clearing behavior
static void clearDescriptorSet(VK_DESCRIPTOR_SET set)
{
    SET_NODE* pSet = getSetNode(set);
    if (!pSet) {
        // TODO : Return error
    }
    else {
        loader_platform_thread_lock_mutex(&globalLock);
        freeShadowUpdateTree(pSet);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
}

static void clearDescriptorPool(VK_DESCRIPTOR_POOL pool)
{
    POOL_NODE* pPool = getPoolNode(pool);
    if (!pPool) {
        char str[1024];
        sprintf(str, "Unable to find pool node for pool %p specified in vkClearDescriptorPool() call", (void*)pool);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pool, 0, DRAWSTATE_INVALID_POOL, "DS", str);
    }
    else
    {
        // For every set off of this pool, clear it
        SET_NODE* pSet = pPool->pSets;
        while (pSet) {
            clearDescriptorSet(pSet->set);
        }
    }
}
// Code here to manage the Cmd buffer LL
static GLOBAL_CB_NODE* getCBNode(VK_CMD_BUFFER cb)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (cmdBufferMap.find(cb) == cmdBufferMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return cmdBufferMap[cb];
}
// Free all CB Nodes
// NOTE : Calls to this function should be wrapped in mutex
static void deleteCmdBuffers()
{
    for (unordered_map<VK_CMD_BUFFER, GLOBAL_CB_NODE*>::iterator ii=cmdBufferMap.begin(); ii!=cmdBufferMap.end(); ++ii) {
        while (!(*ii).second->pCmds.empty()) {
            delete (*ii).second->pCmds.back();
            (*ii).second->pCmds.pop_back();
        }
        delete (*ii).second;
    }
}
static void addCmd(GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd)
{
    CMD_NODE* pCmd = new CMD_NODE;
    if (pCmd) {
        // init cmd node and append to end of cmd LL
        memset(pCmd, 0, sizeof(CMD_NODE));
        pCmd->cmdNumber = ++pCB->numCmds;
        pCmd->type = cmd;
        pCB->pCmds.push_back(pCmd);
    }
    else {
        char str[1024];
        sprintf(str, "Out of memory while attempting to allocate new CMD_NODE for cmdBuffer %p", (void*)pCB->cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pCB->cmdBuffer, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
    }
}
static void resetCB(const VK_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        while (!pCB->pCmds.empty()) {
            delete pCB->pCmds.back();
            pCB->pCmds.pop_back();
        }
        // Reset CB state
        VK_FLAGS saveFlags = pCB->flags;
        uint32_t saveQueueNodeIndex = pCB->queueNodeIndex;
        memset(pCB, 0, sizeof(GLOBAL_CB_NODE));
        pCB->cmdBuffer = cb;
        pCB->flags = saveFlags;
        pCB->queueNodeIndex = saveQueueNodeIndex;
        pCB->lastVtxBinding = MAX_BINDING;
    }
}
// Set the last bound dynamic state of given type
// TODO : Need to track this per cmdBuffer and correlate cmdBuffer for Draw w/ last bound for that cmdBuffer?
static void setLastBoundDynamicState(const VK_CMD_BUFFER cmdBuffer, const VK_DYNAMIC_STATE_OBJECT state, const VK_STATE_BIND_POINT sType)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        loader_platform_thread_lock_mutex(&globalLock);
        addCmd(pCB, CMD_BINDDYNAMICSTATEOBJECT);
        if (dynamicStateMap.find(state) == dynamicStateMap.end()) {
            char str[1024];
            sprintf(str, "Unable to find dynamic state object %p, was it ever created?", (void*)state);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, state, 0, DRAWSTATE_INVALID_DYNAMIC_STATE_OBJECT, "DS", str);
        }
        else {
            pCB->lastBoundDynamicState[sType] = dynamicStateMap[state];
            g_lastBoundDynamicState[sType] = dynamicStateMap[state];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
}
// Print the last bound Gfx Pipeline
static void printPipeline(const VK_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        if (!pPipeTrav) {
            // nothing to print
        }
        else {
            string pipeStr = vk_print_vk_graphics_pipeline_create_info(&pPipeTrav->graphicsPipelineCI, "{DS}").c_str();
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", pipeStr.c_str());
        }
    }
}
// Common Dot dumping code
static void dsCoreDumpDot(const VK_DESCRIPTOR_SET ds, FILE* pOutFile)
{
    SET_NODE* pSet = getSetNode(ds);
    if (pSet) {
        POOL_NODE* pPool = getPoolNode(pSet->pool);
        char tmp_str[4*1024];
        fprintf(pOutFile, "subgraph cluster_DescriptorPool\n{\nlabel=\"Descriptor Pool\"\n");
        sprintf(tmp_str, "Pool (%p)", pPool->pool);
        char* pGVstr = vk_gv_print_vk_descriptor_pool_create_info(&pPool->createInfo, tmp_str);
        fprintf(pOutFile, "%s", pGVstr);
        free(pGVstr);
        fprintf(pOutFile, "subgraph cluster_DescriptorSet\n{\nlabel=\"Descriptor Set (%p)\"\n", pSet->set);
        sprintf(tmp_str, "Descriptor Set (%p)", pSet->set);
        LAYOUT_NODE* pLayout = pSet->pLayout;
        uint32_t layout_index = 0;
        ++layout_index;
        sprintf(tmp_str, "LAYOUT%u", layout_index);
        pGVstr = vk_gv_print_vk_descriptor_set_layout_create_info(&pLayout->createInfo, tmp_str);
        fprintf(pOutFile, "%s", pGVstr);
        free(pGVstr);
        if (pSet->pUpdateStructs) {
            pGVstr = dynamic_gv_display(pSet->pUpdateStructs, "Descriptor Updates");
            fprintf(pOutFile, "%s", pGVstr);
            free(pGVstr);
        }
        if (pSet->ppDescriptors) {
            fprintf(pOutFile, "\"DESCRIPTORS\" [\nlabel=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\"> <TR><TD COLSPAN=\"2\" PORT=\"desc\">DESCRIPTORS</TD></TR>");
            uint32_t i = 0;
            for (i=0; i < pSet->descriptorCount; i++) {
                if (pSet->ppDescriptors[i]) {
                    fprintf(pOutFile, "<TR><TD PORT=\"slot%u\">slot%u</TD><TD>%s</TD></TR>", i, i, string_VK_STRUCTURE_TYPE(pSet->ppDescriptors[i]->sType));
                }
            }
#define NUM_COLORS 7
            vector<string> edgeColors;
            edgeColors.push_back("0000ff");
            edgeColors.push_back("ff00ff");
            edgeColors.push_back("ffff00");
            edgeColors.push_back("00ff00");
            edgeColors.push_back("000000");
            edgeColors.push_back("00ffff");
            edgeColors.push_back("ff0000");
            uint32_t colorIdx = 0;
            fprintf(pOutFile, "</TABLE>>\n];\n");
            // Now add the views that are mapped to active descriptors
            VK_UPDATE_SAMPLERS* pUS = NULL;
            VK_UPDATE_SAMPLER_TEXTURES* pUST = NULL;
            VK_UPDATE_IMAGES* pUI = NULL;
            VK_UPDATE_BUFFERS* pUB = NULL;
            VK_UPDATE_AS_COPY* pUAC = NULL;
            VK_SAMPLER_CREATE_INFO* pSCI = NULL;
            VK_IMAGE_VIEW_CREATE_INFO* pIVCI = NULL;
            VK_BUFFER_VIEW_CREATE_INFO* pBVCI = NULL;
            void** ppNextPtr = NULL;
            void* pSaveNext = NULL;
            for (i=0; i < pSet->descriptorCount; i++) {
                if (pSet->ppDescriptors[i]) {
                    switch (pSet->ppDescriptors[i]->sType)
                    {
                        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                            pUS = (VK_UPDATE_SAMPLERS*)pSet->ppDescriptors[i];
                            pSCI = getSamplerCreateInfo(pUS->pSamplers[i-pUS->arrayIndex]);
                            if (pSCI) {
                                sprintf(tmp_str, "SAMPLER%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vk_sampler_create_info(pSCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                            pUST = (VK_UPDATE_SAMPLER_TEXTURES*)pSet->ppDescriptors[i];
                            pSCI = getSamplerCreateInfo(pUST->pSamplerImageViews[i-pUST->arrayIndex].sampler);
                            if (pSCI) {
                                sprintf(tmp_str, "SAMPLER%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vk_sampler_create_info(pSCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            pIVCI = getImageViewCreateInfo(pUST->pSamplerImageViews[i-pUST->arrayIndex].pImageView->view);
                            if (pIVCI) {
                                sprintf(tmp_str, "IMAGE_VIEW%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vk_image_view_create_info(pIVCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
                            pUI = (VK_UPDATE_IMAGES*)pSet->ppDescriptors[i];
                            pIVCI = getImageViewCreateInfo(pUI->pImageViews[i-pUI->arrayIndex].view);
                            if (pIVCI) {
                                sprintf(tmp_str, "IMAGE_VIEW%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vk_image_view_create_info(pIVCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
                            pUB = (VK_UPDATE_BUFFERS*)pSet->ppDescriptors[i];
                            pBVCI = getBufferViewCreateInfo(pUB->pBufferViews[i-pUB->arrayIndex].view);
                            if (pBVCI) {
                                sprintf(tmp_str, "BUFFER_VIEW%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vk_buffer_view_create_info(pBVCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
                            pUAC = (VK_UPDATE_AS_COPY*)pSet->ppDescriptors[i];
                            // TODO : Need to validate this code
                            // Save off pNext and set to NULL while printing this struct, then restore it
                            ppNextPtr = (void**)&pUAC->pNext;
                            pSaveNext = *ppNextPtr;
                            *ppNextPtr = NULL;
                            sprintf(tmp_str, "UPDATE_AS_COPY%u", i);
                            fprintf(pOutFile, "%s", vk_gv_print_vk_update_as_copy(pUAC, tmp_str));
                            fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            // Restore next ptr
                            *ppNextPtr = pSaveNext;
                            break;
                        default:
                            break;
                    }
                    colorIdx = (colorIdx+1) % NUM_COLORS;
                }
            }
        }
        fprintf(pOutFile, "}\n");
        fprintf(pOutFile, "}\n");
    }
}
// Dump subgraph w/ DS info
static void dsDumpDot(const VK_CMD_BUFFER cb, FILE* pOutFile)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB && pCB->lastBoundDescriptorSet) {
        dsCoreDumpDot(pCB->lastBoundDescriptorSet, pOutFile);
    }
}
// Dump a GraphViz dot file showing the Cmd Buffers
static void cbDumpDotFile(string outFileName)
{
    // Print CB Chain for each CB
    FILE* pOutFile;
    pOutFile = fopen(outFileName.c_str(), "w");
    fprintf(pOutFile, "digraph g {\ngraph [\nrankdir = \"TB\"\n];\nnode [\nfontsize = \"16\"\nshape = \"plaintext\"\n];\nedge [\n];\n");
    fprintf(pOutFile, "subgraph cluster_cmdBuffers\n{\nlabel=\"Command Buffers\"\n");
    GLOBAL_CB_NODE* pCB = NULL;
    for (uint32_t i = 0; i < NUM_COMMAND_BUFFERS_TO_DISPLAY; i++) {
        pCB = g_pLastTouchedCB[i];
        if (pCB) {
            fprintf(pOutFile, "subgraph cluster_cmdBuffer%u\n{\nlabel=\"Command Buffer #%u\"\n", i, i);
            uint32_t instNum = 0;
            for (vector<CMD_NODE*>::iterator ii=pCB->pCmds.begin(); ii!=pCB->pCmds.end(); ++ii) {
                if (instNum) {
                    fprintf(pOutFile, "\"CB%pCMD%u\" -> \"CB%pCMD%u\" [];\n", (void*)pCB->cmdBuffer, instNum-1, (void*)pCB->cmdBuffer, instNum);
                }
                if (pCB == g_lastGlobalCB) {
                    fprintf(pOutFile, "\"CB%pCMD%u\" [\nlabel=<<TABLE BGCOLOR=\"#00FF00\" BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\"> <TR><TD>CMD#</TD><TD>%u</TD></TR><TR><TD>CMD Type</TD><TD>%s</TD></TR></TABLE>>\n];\n", (void*)pCB->cmdBuffer, instNum, instNum, cmdTypeToString((*ii)->type).c_str());
                }
                else {
                    fprintf(pOutFile, "\"CB%pCMD%u\" [\nlabel=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\"> <TR><TD>CMD#</TD><TD>%u</TD></TR><TR><TD>CMD Type</TD><TD>%s</TD></TR></TABLE>>\n];\n", (void*)pCB->cmdBuffer, instNum, instNum, cmdTypeToString((*ii)->type).c_str());
                }
                ++instNum;
            }
            fprintf(pOutFile, "}\n");
        }
    }
    fprintf(pOutFile, "}\n");
    fprintf(pOutFile, "}\n"); // close main graph "g"
    fclose(pOutFile);
}
// Dump a GraphViz dot file showing the pipeline for last bound global state
static void dumpGlobalDotFile(char *outFileName)
{
    PIPELINE_NODE *pPipeTrav = g_lastBoundPipeline;
    if (pPipeTrav) {
        FILE* pOutFile;
        pOutFile = fopen(outFileName, "w");
        fprintf(pOutFile, "digraph g {\ngraph [\nrankdir = \"TB\"\n];\nnode [\nfontsize = \"16\"\nshape = \"plaintext\"\n];\nedge [\n];\n");
        fprintf(pOutFile, "subgraph cluster_dynamicState\n{\nlabel=\"Dynamic State\"\n");
        char* pGVstr = NULL;
        for (uint32_t i = 0; i < VK_NUM_STATE_BIND_POINT; i++) {
            if (g_lastBoundDynamicState[i] && g_lastBoundDynamicState[i]->pCreateInfo) {
                pGVstr = dynamic_gv_display(g_lastBoundDynamicState[i]->pCreateInfo, string_VK_STATE_BIND_POINT((VK_STATE_BIND_POINT)i));
                fprintf(pOutFile, "%s", pGVstr);
                free(pGVstr);
            }
        }
        fprintf(pOutFile, "}\n"); // close dynamicState subgraph
        fprintf(pOutFile, "subgraph cluster_PipelineStateObject\n{\nlabel=\"Pipeline State Object\"\n");
        pGVstr = vk_gv_print_vk_graphics_pipeline_create_info(&pPipeTrav->graphicsPipelineCI, "PSO HEAD");
        fprintf(pOutFile, "%s", pGVstr);
        free(pGVstr);
        fprintf(pOutFile, "}\n");
        dsCoreDumpDot(g_lastBoundDescriptorSet, pOutFile);
        fprintf(pOutFile, "}\n"); // close main graph "g"
        fclose(pOutFile);
    }
}
// Dump a GraphViz dot file showing the pipeline for a given CB
static void dumpDotFile(const VK_CMD_BUFFER cb, string outFileName)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        if (pPipeTrav) {
            FILE* pOutFile;
            pOutFile = fopen(outFileName.c_str(), "w");
            fprintf(pOutFile, "digraph g {\ngraph [\nrankdir = \"TB\"\n];\nnode [\nfontsize = \"16\"\nshape = \"plaintext\"\n];\nedge [\n];\n");
            fprintf(pOutFile, "subgraph cluster_dynamicState\n{\nlabel=\"Dynamic State\"\n");
            char* pGVstr = NULL;
            for (uint32_t i = 0; i < VK_NUM_STATE_BIND_POINT; i++) {
                if (pCB->lastBoundDynamicState[i] && pCB->lastBoundDynamicState[i]->pCreateInfo) {
                    pGVstr = dynamic_gv_display(pCB->lastBoundDynamicState[i]->pCreateInfo, string_VK_STATE_BIND_POINT((VK_STATE_BIND_POINT)i));
                    fprintf(pOutFile, "%s", pGVstr);
                    free(pGVstr);
                }
            }
            fprintf(pOutFile, "}\n"); // close dynamicState subgraph
            fprintf(pOutFile, "subgraph cluster_PipelineStateObject\n{\nlabel=\"Pipeline State Object\"\n");
            pGVstr = vk_gv_print_vk_graphics_pipeline_create_info(&pPipeTrav->graphicsPipelineCI, "PSO HEAD");
            fprintf(pOutFile, "%s", pGVstr);
            free(pGVstr);
            fprintf(pOutFile, "}\n");
            dsDumpDot(cb, pOutFile);
            fprintf(pOutFile, "}\n"); // close main graph "g"
            fclose(pOutFile);
        }
    }
}
// Verify VB Buffer binding
static void validateVBBinding(const VK_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB && pCB->lastBoundPipeline) {
        // First verify that we have a Node for bound pipeline
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        char str[1024];
        if (!pPipeTrav) {
            sprintf(str, "Can't find last bound Pipeline %p!", (void*)pCB->lastBoundPipeline);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NO_PIPELINE_BOUND, "DS", str);
        }
        else {
            // Verify Vtx binding
            if (MAX_BINDING != pCB->lastVtxBinding) {
                if (pCB->lastVtxBinding >= pPipeTrav->vtxBindingCount) {
                    if (0 == pPipeTrav->vtxBindingCount) {
                        sprintf(str, "Vtx Buffer Index %u was bound, but no vtx buffers are attached to PSO.", pCB->lastVtxBinding);
                        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS", str);
                    }
                    else {
                        sprintf(str, "Vtx binding Index of %u exceeds PSO pVertexBindingDescriptions max array index of %u.", pCB->lastVtxBinding, (pPipeTrav->vtxBindingCount - 1));
                        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS", str);
                    }
                }
                else {
                    string tmpStr = vk_print_vk_vertex_input_binding_description(&pPipeTrav->pVertexBindingDescriptions[pCB->lastVtxBinding], "{DS}INFO : ").c_str();
                    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmpStr.c_str());
                }
            }
        }
    }
}
// Print details of DS config to stdout
static void printDSConfig(const VK_CMD_BUFFER cb)
{
    char tmp_str[1024];
    char ds_config_str[1024*256] = {0}; // TODO : Currently making this buffer HUGE w/o overrun protection.  Need to be smarter, start smaller, and grow as needed.
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        SET_NODE* pSet = getSetNode(pCB->lastBoundDescriptorSet);
        POOL_NODE* pPool = getPoolNode(pSet->pool);
        // Print out pool details
        sprintf(tmp_str, "Details for pool %p.", (void*)pPool->pool);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        string poolStr = vk_print_vk_descriptor_pool_create_info(&pPool->createInfo, " ");
        sprintf(ds_config_str, "%s", poolStr.c_str());
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", ds_config_str);
        // Print out set details
        char prefix[10];
        uint32_t index = 0;
        sprintf(tmp_str, "Details for descriptor set %p.", (void*)pSet->set);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        LAYOUT_NODE* pLayout = pSet->pLayout;
        // Print layout details
        sprintf(tmp_str, "Layout #%u, (object %p) for DS %p.", index+1, (void*)pLayout->layout, (void*)pSet->set);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        sprintf(prefix, "  [L%u] ", index);
        string DSLstr = vk_print_vk_descriptor_set_layout_create_info(&pLayout->createInfo, prefix).c_str();
        sprintf(ds_config_str, "%s", DSLstr.c_str());
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", ds_config_str);
        index++;
        GENERIC_HEADER* pUpdate = pSet->pUpdateStructs;
        if (pUpdate) {
            sprintf(tmp_str, "Update Chain [UC] for descriptor set %p:", (void*)pSet->set);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
            sprintf(prefix, "  [UC] ");
            sprintf(ds_config_str, "%s", dynamic_display(pUpdate, prefix).c_str());
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", ds_config_str);
            // TODO : If there is a "view" associated with this update, print CI for that view
        }
        else {
            sprintf(tmp_str, "No Update Chain for descriptor set %p (vkUpdateDescriptors has not been called)", (void*)pSet->set);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        }
    }
}

static void printCB(const VK_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        char str[1024];
        sprintf(str, "Cmds in CB %p", (void*)cb);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", str);
        for (vector<CMD_NODE*>::iterator ii=pCB->pCmds.begin(); ii!=pCB->pCmds.end(); ++ii) {
            sprintf(str, "  CMD#%lu: %s", (*ii)->cmdNumber, cmdTypeToString((*ii)->type).c_str());
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, cb, 0, DRAWSTATE_NONE, "DS", str);
        }
    }
    else {
        // Nothing to print
    }
}


static void synchAndPrintDSConfig(const VK_CMD_BUFFER cb)
{
    printDSConfig(cb);
    printPipeline(cb);
    printDynamicState(cb);
    static int autoDumpOnce = 0;
    if (autoDumpOnce) {
        autoDumpOnce = 0;
        dumpDotFile(cb, "pipeline_dump.dot");
        cbDumpDotFile("cb_dump.dot");
#if defined(_WIN32)
// FIXME: NEED WINDOWS EQUIVALENT
#else // WIN32
        // Convert dot to svg if dot available
        if(access( "/usr/bin/dot", X_OK) != -1) {
            system("/usr/bin/dot pipeline_dump.dot -Tsvg -o pipeline_dump.svg");
        }
#endif // WIN32
    }
}

static void initDrawState(void)
{
    const char *strOpt;
    // initialize DrawState options
    getLayerOptionEnum("DrawStateReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum("DrawStateDebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
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
    vkGetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VK_PHYSICAL_GPU) pCurObj->nextObject);

    vkGetProcAddrType fpGetProcAddr = (vkGetProcAddrType)fpNextGPA((VK_PHYSICAL_GPU) pCurObj->nextObject, (char *) "vkGetProcAddr");
    nextTable.GetProcAddr = fpGetProcAddr;

    if (!globalLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during vkCreateInstance(), and then we
        // can clean it up during vkDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDevice(VK_PHYSICAL_GPU gpu, const VK_DEVICE_CREATE_INFO* pCreateInfo, VK_DEVICE* pDevice)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);
    VK_RESULT result = nextTable.CreateDevice((VK_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDestroyDevice(VK_DEVICE device)
{
    // Free all the memory
    loader_platform_thread_lock_mutex(&globalLock);
    deletePipelines();
    deleteSamplers();
    deleteImages();
    deleteBuffers();
    deleteCmdBuffers();
    deleteDynamicState();
    deletePools();
    deleteLayouts();
    loader_platform_thread_unlock_mutex(&globalLock);
    VK_RESULT result = nextTable.DestroyDevice(device);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetExtensionSupport(VK_PHYSICAL_GPU gpu, const char* pExtName)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
    VK_RESULT result;
    /* This entrypoint is NOT going to init its own dispatch table since loader calls here early */
    if (!strcmp(pExtName, "DrawState") || !strcmp(pExtName, "drawStateDumpDotFile") ||
        !strcmp(pExtName, "drawStateDumpCommandBufferDotFile") || !strcmp(pExtName, "drawStateDumpPngFile"))
    {
        result = VK_SUCCESS;
    } else if (nextTable.GetExtensionSupport != NULL)
    {
        result = nextTable.GetExtensionSupport((VK_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    } else
    {
        result = VK_ERROR_INVALID_EXTENSION;
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkEnumerateLayers(VK_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    if (gpu != NULL)
    {
        VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
        pCurObj = gpuw;
        loader_platform_thread_once(&g_initOnce, initDrawState);
        VK_RESULT result = nextTable.EnumerateLayers((VK_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        return result;
    } else
    {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return VK_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pOutLayerCount = 1;
        strncpy((char *) pOutLayers[0], "DrawState", maxStringSize);
        return VK_SUCCESS;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkQueueSubmit(VK_QUEUE queue, uint32_t cmdBufferCount, const VK_CMD_BUFFER* pCmdBuffers, VK_FENCE fence)
{
    for (uint32_t i=0; i < cmdBufferCount; i++) {
        // Validate that cmd buffers have been updated
    }
    VK_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDestroyObject(VK_OBJECT object)
{
    // TODO : When wrapped objects (such as dynamic state) are destroyed, need to clean up memory
    VK_RESULT result = nextTable.DestroyObject(object);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateBufferView(VK_DEVICE device, const VK_BUFFER_VIEW_CREATE_INFO* pCreateInfo, VK_BUFFER_VIEW* pView)
{
    VK_RESULT result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        BUFFER_NODE* pNewNode = new BUFFER_NODE;
        pNewNode->buffer = *pView;
        pNewNode->createInfo = *pCreateInfo;
        bufferMap[*pView] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateImageView(VK_DEVICE device, const VK_IMAGE_VIEW_CREATE_INFO* pCreateInfo, VK_IMAGE_VIEW* pView)
{
    VK_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        IMAGE_NODE *pNewNode = new IMAGE_NODE;
        pNewNode->image = *pView;
        pNewNode->createInfo = *pCreateInfo;
        imageMap[*pView] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

static void track_pipeline(const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE* pPipeline)
{
    // Create LL HEAD for this Pipeline
    loader_platform_thread_lock_mutex(&globalLock);
    PIPELINE_NODE* pPipeNode = new PIPELINE_NODE;
    memset((void*)pPipeNode, 0, sizeof(PIPELINE_NODE));
    pPipeNode->pipeline = *pPipeline;
    initPipeline(pPipeNode, pCreateInfo);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateGraphicsPipeline(VK_DEVICE device, const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE* pPipeline)
{
    VK_RESULT result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    // Create LL HEAD for this Pipeline
    char str[1024];
    sprintf(str, "Created Gfx Pipeline %p", (void*)*pPipeline);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, *pPipeline, 0, DRAWSTATE_NONE, "DS", str);

    track_pipeline(pCreateInfo, pPipeline);

    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateGraphicsPipelineDerivative(
        VK_DEVICE device,
        const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
        VK_PIPELINE basePipeline,
        VK_PIPELINE* pPipeline)
{
    VK_RESULT result = nextTable.CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
    // Create LL HEAD for this Pipeline
    char str[1024];
    sprintf(str, "Created Gfx Pipeline %p (derived from pipeline %p)", (void*)*pPipeline, basePipeline);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, *pPipeline, 0, DRAWSTATE_NONE, "DS", str);

    track_pipeline(pCreateInfo, pPipeline);

    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateSampler(VK_DEVICE device, const VK_SAMPLER_CREATE_INFO* pCreateInfo, VK_SAMPLER* pSampler)
{
    VK_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        SAMPLER_NODE* pNewNode = new SAMPLER_NODE;
        pNewNode->sampler = *pSampler;
        pNewNode->createInfo = *pCreateInfo;
        sampleMap[*pSampler] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDescriptorSetLayout(VK_DEVICE device, const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo, VK_DESCRIPTOR_SET_LAYOUT* pSetLayout)
{
    VK_RESULT result = nextTable.CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    if (VK_SUCCESS == result) {
        LAYOUT_NODE* pNewNode = new LAYOUT_NODE;
        if (NULL == pNewNode) {
            char str[1024];
            sprintf(str, "Out of memory while attempting to allocate LAYOUT_NODE in vkCreateDescriptorSetLayout()");
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, *pSetLayout, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
        }
        memset(pNewNode, 0, sizeof(LAYOUT_NODE));
        memcpy((void*)&pNewNode->createInfo, pCreateInfo, sizeof(VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO));
        pNewNode->createInfo.pBinding = new VK_DESCRIPTOR_SET_LAYOUT_BINDING[pCreateInfo->count];
        memcpy((void*)pNewNode->createInfo.pBinding, pCreateInfo->pBinding, sizeof(VK_DESCRIPTOR_SET_LAYOUT_BINDING)*pCreateInfo->count);
        uint32_t totalCount = 0;
        for (uint32_t i=0; i<pCreateInfo->count; i++) {
            totalCount += pCreateInfo->pBinding[i].count;
            if (pCreateInfo->pBinding[i].pImmutableSamplers) {
                VK_SAMPLER** ppIS = (VK_SAMPLER**)&pNewNode->createInfo.pBinding[i].pImmutableSamplers;
                *ppIS = new VK_SAMPLER[pCreateInfo->pBinding[i].count];
                memcpy(*ppIS, pCreateInfo->pBinding[i].pImmutableSamplers, pCreateInfo->pBinding[i].count*sizeof(VK_SAMPLER));
            }
        }
        if (totalCount > 0) {
            pNewNode->pTypes = new VK_DESCRIPTOR_TYPE[totalCount];
            uint32_t offset = 0;
            uint32_t j = 0;
            for (uint32_t i=0; i<pCreateInfo->count; i++) {
                for (j = 0; j < pCreateInfo->pBinding[i].count; j++) {
                    pNewNode->pTypes[offset + j] = pCreateInfo->pBinding[i].descriptorType;
                }
                offset += j;
            }
        }
        pNewNode->layout = *pSetLayout;
        pNewNode->startIndex = 0;
        pNewNode->endIndex = pNewNode->startIndex + totalCount - 1;
        assert(pNewNode->endIndex >= pNewNode->startIndex);
        // Put new node at Head of global Layer list
        loader_platform_thread_lock_mutex(&globalLock);
        layoutMap[*pSetLayout] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_RESULT VKAPI vkCreateDescriptorSetLayoutChain(VK_DEVICE device, uint32_t setLayoutArrayCount, const VK_DESCRIPTOR_SET_LAYOUT* pSetLayoutArray, VK_DESCRIPTOR_SET_LAYOUT_CHAIN* pLayoutChain)
{
    VK_RESULT result = nextTable.CreateDescriptorSetLayoutChain(device, setLayoutArrayCount, pSetLayoutArray, pLayoutChain);
    if (VK_SUCCESS == result) {
        // TODO : Need to capture the layout chains
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkBeginDescriptorPoolUpdate(VK_DEVICE device, VK_DESCRIPTOR_UPDATE_MODE updateMode)
{
    VK_RESULT result = nextTable.BeginDescriptorPoolUpdate(device, updateMode);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        POOL_NODE* pPoolNode = poolMap.begin()->second;
        if (!pPoolNode) {
            char str[1024];
            sprintf(str, "Unable to find pool node");
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INTERNAL_ERROR, "DS", str);
        }
        else {
            pPoolNode->updateActive = 1;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkEndDescriptorPoolUpdate(VK_DEVICE device, VK_CMD_BUFFER cmd)
{
    VK_RESULT result = nextTable.EndDescriptorPoolUpdate(device, cmd);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        POOL_NODE* pPoolNode = poolMap.begin()->second;
        if (!pPoolNode) {
            char str[1024];
            sprintf(str, "Unable to find pool node");
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INTERNAL_ERROR, "DS", str);
        }
        else {
            if (!pPoolNode->updateActive) {
                char str[1024];
                sprintf(str, "You must call vkBeginDescriptorPoolUpdate() before this call to vkEndDescriptorPoolUpdate()!");
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_DS_END_WITHOUT_BEGIN, "DS", str);
            }
            else {
                pPoolNode->updateActive = 0;
            }
            pPoolNode->updateActive = 0;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDescriptorPool(VK_DEVICE device, VK_DESCRIPTOR_POOL_USAGE poolUsage, uint32_t maxSets, const VK_DESCRIPTOR_POOL_CREATE_INFO* pCreateInfo, VK_DESCRIPTOR_POOL* pDescriptorPool)
{
    VK_RESULT result = nextTable.CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    if (VK_SUCCESS == result) {
        // Insert this pool into Global Pool LL at head
        char str[1024];
        sprintf(str, "Created Descriptor Pool %p", (void*)*pDescriptorPool);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, (VK_BASE_OBJECT)pDescriptorPool, 0, DRAWSTATE_NONE, "DS", str);
        loader_platform_thread_lock_mutex(&globalLock);
        POOL_NODE* pNewNode = new POOL_NODE;
        if (NULL == pNewNode) {
            char str[1024];
            sprintf(str, "Out of memory while attempting to allocate POOL_NODE in vkCreateDescriptorPool()");
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, (VK_BASE_OBJECT)*pDescriptorPool, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
        }
        else {
            memset(pNewNode, 0, sizeof(POOL_NODE));
            VK_DESCRIPTOR_POOL_CREATE_INFO* pCI = (VK_DESCRIPTOR_POOL_CREATE_INFO*)&pNewNode->createInfo;
            memcpy((void*)pCI, pCreateInfo, sizeof(VK_DESCRIPTOR_POOL_CREATE_INFO));
            if (pNewNode->createInfo.count) {
                size_t typeCountSize = pNewNode->createInfo.count * sizeof(VK_DESCRIPTOR_TYPE_COUNT);
                pNewNode->createInfo.pTypeCount = new VK_DESCRIPTOR_TYPE_COUNT[typeCountSize];
                memcpy((void*)pNewNode->createInfo.pTypeCount, pCreateInfo->pTypeCount, typeCountSize);
            }
            pNewNode->poolUsage  = poolUsage;
            pNewNode->updateActive = 0;
            pNewNode->maxSets      = maxSets;
            pNewNode->pool       = *pDescriptorPool;
            poolMap[*pDescriptorPool] = pNewNode;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    else {
        // Need to do anything if pool create fails?
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkResetDescriptorPool(VK_DESCRIPTOR_POOL descriptorPool)
{
    VK_RESULT result = nextTable.ResetDescriptorPool(descriptorPool);
    if (VK_SUCCESS == result) {
        clearDescriptorPool(descriptorPool);
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkAllocDescriptorSets(VK_DESCRIPTOR_POOL descriptorPool, VK_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const VK_DESCRIPTOR_SET_LAYOUT* pSetLayouts, VK_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount)
{
    VK_RESULT result = nextTable.AllocDescriptorSets(descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    if ((VK_SUCCESS == result) || (*pCount > 0)) {
        POOL_NODE *pPoolNode = getPoolNode(descriptorPool);
        if (!pPoolNode) {
            char str[1024];
            sprintf(str, "Unable to find pool node for pool %p specified in vkAllocDescriptorSets() call", (void*)descriptorPool);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, descriptorPool, 0, DRAWSTATE_INVALID_POOL, "DS", str);
        }
        else {
            for (uint32_t i = 0; i < *pCount; i++) {
                char str[1024];
                sprintf(str, "Created Descriptor Set %p", (void*)pDescriptorSets[i]);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS", str);
                // Create new set node and add to head of pool nodes
                SET_NODE* pNewNode = new SET_NODE;
                if (NULL == pNewNode) {
                    char str[1024];
                    sprintf(str, "Out of memory while attempting to allocate SET_NODE in vkAllocDescriptorSets()");
                    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
                }
                else {
                    memset(pNewNode, 0, sizeof(SET_NODE));
                    // Insert set at head of Set LL for this pool
                    pNewNode->pNext = pPoolNode->pSets;
                    pPoolNode->pSets = pNewNode;
                    LAYOUT_NODE* pLayout = getLayoutNode(pSetLayouts[i]);
                    if (NULL == pLayout) {
                        char str[1024];
                        sprintf(str, "Unable to find set layout node for layout %p specified in vkAllocDescriptorSets() call", (void*)pSetLayouts[i]);
                        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pSetLayouts[i], 0, DRAWSTATE_INVALID_LAYOUT, "DS", str);
                    }
                    pNewNode->pLayout = pLayout;
                    pNewNode->pool = descriptorPool;
                    pNewNode->set = pDescriptorSets[i];
                    pNewNode->setUsage = setUsage;
                    pNewNode->descriptorCount = pLayout->endIndex + 1;
                    if (pNewNode->descriptorCount) {
                        size_t descriptorArraySize = sizeof(GENERIC_HEADER*)*pNewNode->descriptorCount;
                        pNewNode->ppDescriptors = new GENERIC_HEADER*[descriptorArraySize];
                        memset(pNewNode->ppDescriptors, 0, descriptorArraySize);
                    }
                    setMap[pDescriptorSets[i]] = pNewNode;
                }
            }
        }
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkClearDescriptorSets(VK_DESCRIPTOR_POOL descriptorPool, uint32_t count, const VK_DESCRIPTOR_SET* pDescriptorSets)
{
    for (uint32_t i = 0; i < count; i++) {
        clearDescriptorSet(pDescriptorSets[i]);
    }
    nextTable.ClearDescriptorSets(descriptorPool, count, pDescriptorSets);
}

VK_LAYER_EXPORT void VKAPI vkUpdateDescriptors(VK_DESCRIPTOR_SET descriptorSet, uint32_t updateCount, const void** ppUpdateArray)
{
    SET_NODE* pSet = getSetNode(descriptorSet);
    if (!dsUpdateActive(descriptorSet)) {
        char str[1024];
        sprintf(str, "You must call vkBeginDescriptorPoolUpdate() before this call to vkUpdateDescriptors()!");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pSet->pool, 0, DRAWSTATE_UPDATE_WITHOUT_BEGIN, "DS", str);
    }
    else {
        // pUpdateChain is a Linked-list of VK_UPDATE_* structures defining the mappings for the descriptors
        dsUpdate(descriptorSet, updateCount, ppUpdateArray);
    }

    nextTable.UpdateDescriptors(descriptorSet, updateCount, ppUpdateArray);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicViewportState(VK_DEVICE device, const VK_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_VP_STATE_OBJECT* pState)
{
    VK_RESULT result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_VIEWPORT);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicRasterState(VK_DEVICE device, const VK_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_RS_STATE_OBJECT* pState)
{
    VK_RESULT result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_RASTER);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicColorBlendState(VK_DEVICE device, const VK_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_CB_STATE_OBJECT* pState)
{
    VK_RESULT result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_COLOR_BLEND);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicDepthStencilState(VK_DEVICE device, const VK_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_DS_STATE_OBJECT* pState)
{
    VK_RESULT result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_DEPTH_STENCIL);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateCommandBuffer(VK_DEVICE device, const VK_CMD_BUFFER_CREATE_INFO* pCreateInfo, VK_CMD_BUFFER* pCmdBuffer)
{
    VK_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        GLOBAL_CB_NODE* pCB = new GLOBAL_CB_NODE;
        memset(pCB, 0, sizeof(GLOBAL_CB_NODE));
        pCB->cmdBuffer = *pCmdBuffer;
        pCB->flags = pCreateInfo->flags;
        pCB->queueNodeIndex = pCreateInfo->queueNodeIndex;
        pCB->lastVtxBinding = MAX_BINDING;
        cmdBufferMap[*pCmdBuffer] = pCB;
        loader_platform_thread_unlock_mutex(&globalLock);
        updateCBTracking(*pCmdBuffer);
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkBeginCommandBuffer(VK_CMD_BUFFER cmdBuffer, const VK_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    VK_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    if (VK_SUCCESS == result) {
        GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
        if (pCB) {
            if (CB_NEW != pCB->state)
                resetCB(cmdBuffer);
            pCB->state = CB_UPDATE_ACTIVE;
            if (pBeginInfo->pNext) {
                VK_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pCbGfxBI = (VK_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pBeginInfo->pNext;
                if (VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO == pCbGfxBI->sType) {
                    pCB->activeRenderPass = pCbGfxBI->renderPassContinue.renderPass;
                }
            }
        }
        else {
            char str[1024];
            sprintf(str, "In vkBeginCommandBuffer() and unable to find CmdBuffer Node for CB %p!", (void*)cmdBuffer);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
        }
        updateCBTracking(cmdBuffer);
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkEndCommandBuffer(VK_CMD_BUFFER cmdBuffer)
{
    VK_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    if (VK_SUCCESS == result) {
        GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
        if (pCB) {
            pCB->state = CB_UPDATE_COMPLETE;
            printCB(cmdBuffer);
        }
        else {
            char str[1024];
            sprintf(str, "In vkEndCommandBuffer() and unable to find CmdBuffer Node for CB %p!", (void*)cmdBuffer);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
        }
        updateCBTracking(cmdBuffer);
        //cbDumpDotFile("cb_dump.dot");
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkResetCommandBuffer(VK_CMD_BUFFER cmdBuffer)
{
    VK_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    if (VK_SUCCESS == result) {
        resetCB(cmdBuffer);
        updateCBTracking(cmdBuffer);
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, VK_PIPELINE pipeline)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BINDPIPELINE);
        PIPELINE_NODE* pPN = getPipeline(pipeline);
        if (pPN) {
            pCB->lastBoundPipeline = pipeline;
            loader_platform_thread_lock_mutex(&globalLock);
            g_lastBoundPipeline = pPN;
            loader_platform_thread_unlock_mutex(&globalLock);
            validatePipelineState(pCB, pipelineBindPoint, pipeline);
        }
        else {
            char str[1024];
            sprintf(str, "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pipeline, 0, DRAWSTATE_INVALID_PIPELINE, "DS", str);
        }
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(VK_CMD_BUFFER cmdBuffer, VK_STATE_BIND_POINT stateBindPoint, VK_DYNAMIC_STATE_OBJECT state)
{
    setLastBoundDynamicState(cmdBuffer, state, stateBindPoint);
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, VK_DESCRIPTOR_SET_LAYOUT_CHAIN layoutChain, uint32_t layoutChainSlot, uint32_t count, const VK_DESCRIPTOR_SET* pDescriptorSets, const uint32_t* pUserData)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BINDDESCRIPTORSETS);
        for (uint32_t i=0; i<count; i++) {
            if (getSetNode(pDescriptorSets[i])) {
                if (dsUpdateActive(pDescriptorSets[i])) {
                    // TODO : This check here needs to be made at QueueSubmit time
    /*
                    char str[1024];
                    sprintf(str, "You must call vkEndDescriptorPoolUpdate(%p) before this call to vkCmdBindDescriptorSet()!", (void*)descriptorSet);
                    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, descriptorSet, 0, DRAWSTATE_BINDING_DS_NO_END_UPDATE, "DS", str);
    */
                }
                loader_platform_thread_lock_mutex(&globalLock);
                pCB->lastBoundDescriptorSet = pDescriptorSets[i];
                g_lastBoundDescriptorSet = pDescriptorSets[i];
                loader_platform_thread_unlock_mutex(&globalLock);
                char str[1024];
                sprintf(str, "DS %p bound on pipeline %s", (void*)pDescriptorSets[i], string_VK_PIPELINE_BIND_POINT(pipelineBindPoint));
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS", str);
                synchAndPrintDSConfig(cmdBuffer);
            }
            else {
                char str[1024];
                sprintf(str, "Attempt to bind DS %p that doesn't exist!", (void*)pDescriptorSets[i]);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_INVALID_SET, "DS", str);
            }
        }
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layoutChain, layoutChainSlot, count, pDescriptorSets, pUserData);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, VK_INDEX_TYPE indexType)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BINDINDEXBUFFER);
        // TODO : Track idxBuffer binding
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t binding)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BINDVERTEXBUFFER);
        pCB->lastVtxBinding = binding;
        validateVBBinding(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VK_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAW);
        pCB->drawCount[DRAW]++;
        char str[1024];
        sprintf(str, "vkCmdDraw() call #%lu, reporting DS state:", g_drawCount[DRAW]++);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VK_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAWINDEXED);
        pCB->drawCount[DRAW_INDEXED]++;
        char str[1024];
        sprintf(str, "vkCmdDrawIndexed() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED]++);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAWINDIRECT);
        pCB->drawCount[DRAW_INDIRECT]++;
        char str[1024];
        sprintf(str, "vkCmdDrawIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDIRECT]++);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAWINDEXEDINDIRECT);
        pCB->drawCount[DRAW_INDEXED_INDIRECT]++;
        char str[1024];
        sprintf(str, "vkCmdDrawIndexedIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED_INDIRECT]++);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_NONE, "DS", str);
        synchAndPrintDSConfig(cmdBuffer);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VK_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DISPATCH);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DISPATCHINDIRECT);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER srcBuffer, VK_BUFFER destBuffer, uint32_t regionCount, const VK_BUFFER_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_COPYBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VK_CMD_BUFFER cmdBuffer,
                                             VK_IMAGE srcImage,
                                             VK_IMAGE_LAYOUT srcImageLayout,
                                             VK_IMAGE destImage,
                                             VK_IMAGE_LAYOUT destImageLayout,
                                             uint32_t regionCount, const VK_IMAGE_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_COPYIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VK_CMD_BUFFER cmdBuffer,
                                             VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout,
                                             VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout,
                                             uint32_t regionCount, const VK_IMAGE_BLIT* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BLITIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VK_CMD_BUFFER cmdBuffer,
                                                     VK_BUFFER srcBuffer,
                                                     VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout,
                                                     uint32_t regionCount, const VK_BUFFER_IMAGE_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_COPYBUFFERTOIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VK_CMD_BUFFER cmdBuffer,
                                                     VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout,
                                                     VK_BUFFER destBuffer,
                                                     uint32_t regionCount, const VK_BUFFER_IMAGE_COPY* pRegions)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_COPYIMAGETOBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCloneImageData(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_CLONEIMAGEDATA);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset, VK_GPU_SIZE dataSize, const uint32_t* pData)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_UPDATEBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset, VK_GPU_SIZE fillSize, uint32_t data)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_FILLBUFFER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(VK_CMD_BUFFER cmdBuffer,
                                                   VK_IMAGE image, VK_IMAGE_LAYOUT imageLayout,
                                                   VK_CLEAR_COLOR color,
                                                   uint32_t rangeCount, const VK_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_CLEARCOLORIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdClearColorImage(cmdBuffer, image, imageLayout, color, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(VK_CMD_BUFFER cmdBuffer,
                                                     VK_IMAGE image, VK_IMAGE_LAYOUT imageLayout,
                                                     float depth, uint32_t stencil,
                                                     uint32_t rangeCount, const VK_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_CLEARDEPTHSTENCIL);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VK_CMD_BUFFER cmdBuffer,
                                                VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout,
                                                VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout,
                                                uint32_t rectCount, const VK_IMAGE_RESOLVE* pRects)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_RESOLVEIMAGE);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, rectCount, pRects);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VK_CMD_BUFFER cmdBuffer, VK_EVENT event, VK_PIPE_EVENT pipeEvent)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_SETEVENT);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VK_CMD_BUFFER cmdBuffer, VK_EVENT event, VK_PIPE_EVENT pipeEvent)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_RESETEVENT);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdResetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(VK_CMD_BUFFER cmdBuffer, const VK_EVENT_WAIT_INFO* pWaitInfo)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_WAITEVENTS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(VK_CMD_BUFFER cmdBuffer, const VK_PIPELINE_BARRIER* pBarrier)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_PIPELINEBARRIER);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t slot, VK_FLAGS flags)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BEGINQUERY);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t slot)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_ENDQUERY);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_RESETQUERYPOOL);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VK_CMD_BUFFER cmdBuffer, VK_TIMESTAMP_TYPE timestampType, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_WRITETIMESTAMP);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdInitAtomicCounters(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_INITATOMICCOUNTERS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdLoadAtomicCounters(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VK_BUFFER srcBuffer, VK_GPU_SIZE srcOffset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_LOADATOMICCOUNTERS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdSaveAtomicCounters(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_SAVEATOMICCOUNTERS);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateFramebuffer(VK_DEVICE device, const VK_FRAMEBUFFER_CREATE_INFO* pCreateInfo, VK_FRAMEBUFFER* pFramebuffer)
{
    VK_RESULT result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VK_FRAMEBUFFER_CREATE_INFO* localFBCI = new VK_FRAMEBUFFER_CREATE_INFO(*pCreateInfo);
        if (pCreateInfo->pColorAttachments) {
            localFBCI->pColorAttachments = new VK_COLOR_ATTACHMENT_BIND_INFO[localFBCI->colorAttachmentCount];
            memcpy((void*)localFBCI->pColorAttachments, pCreateInfo->pColorAttachments, localFBCI->colorAttachmentCount*sizeof(VK_COLOR_ATTACHMENT_BIND_INFO));
        }
        if (pCreateInfo->pDepthStencilAttachment) {
            localFBCI->pDepthStencilAttachment = new VK_DEPTH_STENCIL_BIND_INFO[localFBCI->colorAttachmentCount];
            memcpy((void*)localFBCI->pDepthStencilAttachment, pCreateInfo->pDepthStencilAttachment, localFBCI->colorAttachmentCount*sizeof(VK_DEPTH_STENCIL_BIND_INFO));
        }
        frameBufferMap[*pFramebuffer] = localFBCI;
    }
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateRenderPass(VK_DEVICE device, const VK_RENDER_PASS_CREATE_INFO* pCreateInfo, VK_RENDER_PASS* pRenderPass)
{
    VK_RESULT result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VK_RENDER_PASS_CREATE_INFO* localRPCI = new VK_RENDER_PASS_CREATE_INFO(*pCreateInfo);
        if (pCreateInfo->pColorLoadOps) {
            localRPCI->pColorLoadOps = new VK_ATTACHMENT_LOAD_OP[localRPCI->colorAttachmentCount];
            memcpy((void*)localRPCI->pColorLoadOps, pCreateInfo->pColorLoadOps, localRPCI->colorAttachmentCount*sizeof(VK_ATTACHMENT_LOAD_OP));
        }
        if (pCreateInfo->pColorStoreOps) {
            localRPCI->pColorStoreOps = new VK_ATTACHMENT_STORE_OP[localRPCI->colorAttachmentCount];
            memcpy((void*)localRPCI->pColorStoreOps, pCreateInfo->pColorStoreOps, localRPCI->colorAttachmentCount*sizeof(VK_ATTACHMENT_STORE_OP));
        }
        if (pCreateInfo->pColorLoadClearValues) {
            localRPCI->pColorLoadClearValues = new VK_CLEAR_COLOR[localRPCI->colorAttachmentCount];
            memcpy((void*)localRPCI->pColorLoadClearValues, pCreateInfo->pColorLoadClearValues, localRPCI->colorAttachmentCount*sizeof(VK_CLEAR_COLOR));
        }
        renderPassMap[*pRenderPass] = localRPCI;
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VK_CMD_BUFFER cmdBuffer, const VK_RENDER_PASS_BEGIN *pRenderPassBegin)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BEGINRENDERPASS);
        pCB->activeRenderPass = pRenderPassBegin->renderPass;
        pCB->framebuffer = pRenderPassBegin->framebuffer;
        if (pCB->lastBoundPipeline) {
            validatePipelineState(pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
        }
    } else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VK_CMD_BUFFER cmdBuffer, VK_RENDER_PASS renderPass)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_ENDRENDERPASS);
        pCB->activeRenderPass = 0;
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdEndRenderPass(cmdBuffer, renderPass);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgRegisterMsgCallback(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    VK_LAYER_DBG_FUNCTION_NODE* pNewDbgFuncNode = new VK_LAYER_DBG_FUNCTION_NODE;
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    // force callbacks if DebugAction hasn't been set already other than initial value
	if (g_actionIsDefault) {
		g_debugAction = VK_DBG_LAYER_ACTION_CALLBACK;
	}
    VK_RESULT result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgUnregisterMsgCallback(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    VK_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
    VK_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;
    while (pTrav) {
        if (pTrav->pfnMsgCallback == pfnMsgCallback) {
            pPrev->pNext = pTrav->pNext;
            if (g_pDbgFunctionHead == pTrav)
                g_pDbgFunctionHead = pTrav->pNext;
            delete pTrav;
            break;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }
    if (g_pDbgFunctionHead == NULL)
    {
        if (g_actionIsDefault)
            g_debugAction = VK_DBG_LAYER_ACTION_LOG_MSG;
        else
            g_debugAction = (VK_LAYER_DBG_ACTION)(g_debugAction & ~((uint32_t)VK_DBG_LAYER_ACTION_CALLBACK));
    }
    VK_RESULT result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VK_CMD_BUFFER cmdBuffer, const char* pMarker)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DBGMARKERBEGIN);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VK_CMD_BUFFER cmdBuffer)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DBGMARKEREND);
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

// TODO : Want to pass in a cmdBuffer here based on which state to display
void drawStateDumpDotFile(char* outFileName)
{
    // TODO : Currently just setting cmdBuffer based on global var
    //dumpDotFile(g_lastDrawStateCmdBuffer, outFileName);
    dumpGlobalDotFile(outFileName);
}

void drawStateDumpCommandBufferDotFile(char* outFileName)
{
    cbDumpDotFile(outFileName);
}

void drawStateDumpPngFile(char* outFileName)
{
#if defined(_WIN32)
// FIXME: NEED WINDOWS EQUIVALENT
        char str[1024];
        sprintf(str, "Cannot execute dot program yet on Windows.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_MISSING_DOT_PROGRAM, "DS", str);
#else // WIN32
    char dotExe[32] = "/usr/bin/dot";
    if( access(dotExe, X_OK) != -1) {
        dumpDotFile(g_lastCmdBuffer[getTIDIndex()], "/tmp/tmp.dot");
        char dotCmd[1024];
        sprintf(dotCmd, "%s /tmp/tmp.dot -Tpng -o %s", dotExe, outFileName);
        system(dotCmd);
        remove("/tmp/tmp.dot");
    }
    else {
        char str[1024];
        sprintf(str, "Cannot execute dot program at (%s) to dump requested %s file.", dotExe, outFileName);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_MISSING_DOT_PROGRAM, "DS", str);
    }
#endif // WIN32
}

VK_LAYER_EXPORT void* VKAPI vkGetProcAddr(VK_PHYSICAL_GPU gpu, const char* funcName)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;

    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initDrawState);

    if (!strcmp(funcName, "vkGetProcAddr"))
        return (void *) vkGetProcAddr;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (void*) vkDestroyDevice;
    if (!strcmp(funcName, "vkGetExtensionSupport"))
        return (void*) vkGetExtensionSupport;
    if (!strcmp(funcName, "vkEnumerateLayers"))
        return (void*) vkEnumerateLayers;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (void*) vkQueueSubmit;
    if (!strcmp(funcName, "vkDestroyObject"))
        return (void*) vkDestroyObject;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (void*) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (void*) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateGraphicsPipeline"))
        return (void*) vkCreateGraphicsPipeline;
    if (!strcmp(funcName, "vkCreateGraphicsPipelineDerivative"))
        return (void*) vkCreateGraphicsPipelineDerivative;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (void*) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (void*) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayoutChain"))
        return (void*) vkCreateDescriptorSetLayoutChain;
    if (!strcmp(funcName, "vkBeginDescriptorPoolUpdate"))
        return (void*) vkBeginDescriptorPoolUpdate;
    if (!strcmp(funcName, "vkEndDescriptorPoolUpdate"))
        return (void*) vkEndDescriptorPoolUpdate;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (void*) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (void*) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocDescriptorSets"))
        return (void*) vkAllocDescriptorSets;
    if (!strcmp(funcName, "vkClearDescriptorSets"))
        return (void*) vkClearDescriptorSets;
    if (!strcmp(funcName, "vkUpdateDescriptors"))
        return (void*) vkUpdateDescriptors;
    if (!strcmp(funcName, "vkCreateDynamicViewportState"))
        return (void*) vkCreateDynamicViewportState;
    if (!strcmp(funcName, "vkCreateDynamicRasterState"))
        return (void*) vkCreateDynamicRasterState;
    if (!strcmp(funcName, "vkCreateDynamicColorBlendState"))
        return (void*) vkCreateDynamicColorBlendState;
    if (!strcmp(funcName, "vkCreateDynamicDepthStencilState"))
        return (void*) vkCreateDynamicDepthStencilState;
    if (!strcmp(funcName, "vkCreateCommandBuffer"))
        return (void*) vkCreateCommandBuffer;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (void*) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (void*) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (void*) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (void*) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdBindDynamicStateObject"))
        return (void*) vkCmdBindDynamicStateObject;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (void*) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffer"))
        return (void*) vkCmdBindVertexBuffer;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (void*) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDraw"))
        return (void*) vkCmdDraw;
    if (!strcmp(funcName, "vkCmdDrawIndexed"))
        return (void*) vkCmdDrawIndexed;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (void*) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (void*) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatch"))
        return (void*) vkCmdDispatch;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (void*) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (void*) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (void*) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (void*) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (void*) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdCloneImageData"))
        return (void*) vkCmdCloneImageData;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (void*) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (void*) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (void*) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdClearDepthStencil"))
        return (void*) vkCmdClearDepthStencil;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (void*) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdSetEvent"))
        return (void*) vkCmdSetEvent;
    if (!strcmp(funcName, "vkCmdResetEvent"))
        return (void*) vkCmdResetEvent;
    if (!strcmp(funcName, "vkCmdWaitEvents"))
        return (void*) vkCmdWaitEvents;
    if (!strcmp(funcName, "vkCmdPipelineBarrier"))
        return (void*) vkCmdPipelineBarrier;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (void*) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (void*) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (void*) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkCmdWriteTimestamp"))
        return (void*) vkCmdWriteTimestamp;
    if (!strcmp(funcName, "vkCmdInitAtomicCounters"))
        return (void*) vkCmdInitAtomicCounters;
    if (!strcmp(funcName, "vkCmdLoadAtomicCounters"))
        return (void*) vkCmdLoadAtomicCounters;
    if (!strcmp(funcName, "vkCmdSaveAtomicCounters"))
        return (void*) vkCmdSaveAtomicCounters;
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (void*) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (void*) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (void*) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdEndRenderPass"))
        return (void*) vkCmdEndRenderPass;
    if (!strcmp(funcName, "vkDbgRegisterMsgCallback"))
        return (void*) vkDbgRegisterMsgCallback;
    if (!strcmp(funcName, "vkDbgUnregisterMsgCallback"))
        return (void*) vkDbgUnregisterMsgCallback;
    if (!strcmp(funcName, "vkCmdDbgMarkerBegin"))
        return (void*) vkCmdDbgMarkerBegin;
    if (!strcmp(funcName, "vkCmdDbgMarkerEnd"))
        return (void*) vkCmdDbgMarkerEnd;
    if (!strcmp("drawStateDumpDotFile", funcName))
        return (void*) drawStateDumpDotFile;
    if (!strcmp("drawStateDumpCommandBufferDotFile", funcName))
        return (void*) drawStateDumpCommandBufferDotFile;
    if (!strcmp("drawStateDumpPngFile", funcName))
        return (void*) drawStateDumpPngFile;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VK_PHYSICAL_GPU)gpuw->nextObject, funcName);
    }
}
