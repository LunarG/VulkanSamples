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
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#include "vk_struct_graphviz_helper.h"
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "vk_struct_size_helper.h"
#include "draw_state.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"
#include "layers_msg.h"

unordered_map<VkSampler, SAMPLER_NODE*> sampleMap;
unordered_map<VkImageView, IMAGE_NODE*> imageMap;
unordered_map<VkBufferView, BUFFER_NODE*> bufferMap;
unordered_map<VkDynamicStateObject, DYNAMIC_STATE_NODE*> dynamicStateMap;
unordered_map<VkPipeline, PIPELINE_NODE*> pipelineMap;
unordered_map<VkDescriptorPool, POOL_NODE*> poolMap;
unordered_map<VkDescriptorSet, SET_NODE*> setMap;
unordered_map<VkDescriptorSetLayout, LAYOUT_NODE*> layoutMap;
// Map for layout chains
unordered_map<VkCmdBuffer, GLOBAL_CB_NODE*> cmdBufferMap;
unordered_map<VkRenderPass, VkRenderPassCreateInfo*> renderPassMap;
unordered_map<VkFramebuffer, VkFramebufferCreateInfo*> frameBufferMap;

static std::unordered_map<void *, VkLayerDispatchTable *> tableMap;
static std::unordered_map<void *, VkLayerInstanceDispatchTable *> tableInstanceMap;

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
static VkCmdBuffer    g_lastCmdBuffer[MAX_TID] = {NULL};
// Track the last group of CBs touched for displaying to dot file
static GLOBAL_CB_NODE*   g_pLastTouchedCB[NUM_COMMAND_BUFFERS_TO_DISPLAY] = {NULL};
static uint32_t g_lastTouchedCBIndex = 0;
// Track the last global DrawState of interest touched by any thread
static GLOBAL_CB_NODE*        g_lastGlobalCB = NULL;
static PIPELINE_NODE*         g_lastBoundPipeline = NULL;
static DYNAMIC_STATE_NODE*    g_lastBoundDynamicState[VK_NUM_STATE_BIND_POINT] = {NULL};
static VkDescriptorSet     g_lastBoundDescriptorSet = NULL;
#define MAX_BINDING 0xFFFFFFFF // Default vtxBinding value in CB Node to identify if no vtxBinding set

//static DYNAMIC_STATE_NODE* g_pDynamicStateHead[VK_NUM_STATE_BIND_POINT] = {0};

static void insertDynamicState(const VkDynamicStateObject state, const GENERIC_HEADER* pCreateInfo, VkStateBindPoint bindPoint)
{
    VkDynamicVpStateCreateInfo* pVPCI = NULL;
    size_t scSize = 0;
    size_t vpSize = 0;
    loader_platform_thread_lock_mutex(&globalLock);
    DYNAMIC_STATE_NODE* pStateNode = new DYNAMIC_STATE_NODE;
    pStateNode->stateObj = state;
    switch (pCreateInfo->sType) {
        case VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VkDynamicVpStateCreateInfo));
            pVPCI = (VkDynamicVpStateCreateInfo*)pCreateInfo;
            pStateNode->create_info.vpci.pScissors = new VkRect[pStateNode->create_info.vpci.viewportAndScissorCount];
            pStateNode->create_info.vpci.pViewports = new VkViewport[pStateNode->create_info.vpci.viewportAndScissorCount];
            scSize = pVPCI->viewportAndScissorCount * sizeof(VkRect);
            vpSize = pVPCI->viewportAndScissorCount * sizeof(VkViewport);
            memcpy((void*)pStateNode->create_info.vpci.pScissors, pVPCI->pScissors, scSize);
            memcpy((void*)pStateNode->create_info.vpci.pViewports, pVPCI->pViewports, vpSize);
            break;
        case VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VkDynamicRsStateCreateInfo));
            break;
        case VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VkDynamicCbStateCreateInfo));
            break;
        case VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
            memcpy(&pStateNode->create_info, pCreateInfo, sizeof(VkDynamicDsStateCreateInfo));
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
    if (dynamicStateMap.size() <= 0)
        return;
    for (unordered_map<VkDynamicStateObject, DYNAMIC_STATE_NODE*>::iterator ii=dynamicStateMap.begin(); ii!=dynamicStateMap.end(); ++ii) {
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
    if (sampleMap.size() <= 0)
        return;
    for (unordered_map<VkSampler, SAMPLER_NODE*>::iterator ii=sampleMap.begin(); ii!=sampleMap.end(); ++ii) {
        delete (*ii).second;
    }
}
static VkImageViewCreateInfo* getImageViewCreateInfo(VkImageView view)
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
    if (imageMap.size() <= 0)
        return;
    for (unordered_map<VkImageView, IMAGE_NODE*>::iterator ii=imageMap.begin(); ii!=imageMap.end(); ++ii) {
        delete (*ii).second;
    }
}
static VkBufferViewCreateInfo* getBufferViewCreateInfo(VkBufferView view)
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
    if (bufferMap.size() <= 0)
        return;
    for (unordered_map<VkBufferView, BUFFER_NODE*>::iterator ii=bufferMap.begin(); ii!=bufferMap.end(); ++ii) {
        delete (*ii).second;
    }
}
static GLOBAL_CB_NODE* getCBNode(VkCmdBuffer cb);

static void updateCBTracking(VkCmdBuffer cb)
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
// Check object status for selected flag state
static bool32_t validate_status(VkCmdBuffer cb, CBStatusFlags enable_mask, CBStatusFlags status_mask, CBStatusFlags status_flag, VK_DBG_MSG_TYPE error_level, DRAW_STATE_ERROR error_code, const char* fail_msg) {
    if (cmdBufferMap.find(cb) != cmdBufferMap.end()) {
        GLOBAL_CB_NODE* pNode = cmdBufferMap[cb];
        // If non-zero enable mask is present, check it against status but if enable_mask
        //  is 0 then no enable required so we should always just check status
        if ((!enable_mask) || (enable_mask & pNode->status)) {
            if ((pNode->status & status_mask) != status_flag) {
                char str[1024];
                sprintf(str, "CB object 0x%" PRIxLEAST64 ": %s", reinterpret_cast<VkUintPtrLeast64>(cb), fail_msg);
                layerCbMsg(error_level, VK_VALIDATION_LEVEL_0, cb, 0, error_code, "DS", str);
                return VK_FALSE;
            }
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        char str[1024];
        sprintf(str, "Unable to obtain status for non-existent CB object 0x%" PRIxLEAST64, reinterpret_cast<VkUintPtrLeast64>(cb));
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cb, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
        return VK_FALSE;
    }
}
static bool32_t validate_draw_state_flags(VkCmdBuffer cb) {
    bool32_t result1, result2, result3, result4;
    result1 = validate_status(cb, CBSTATUS_NONE, CBSTATUS_VIEWPORT_BOUND, CBSTATUS_VIEWPORT_BOUND, VK_DBG_MSG_ERROR, DRAWSTATE_VIEWPORT_NOT_BOUND, "Viewport object not bound to this command buffer");
    result2 = validate_status(cb, CBSTATUS_NONE, CBSTATUS_RASTER_BOUND,   CBSTATUS_RASTER_BOUND,   VK_DBG_MSG_ERROR, DRAWSTATE_RASTER_NOT_BOUND,   "Raster object not bound to this command buffer");
    result3 = validate_status(cb, CBSTATUS_COLOR_BLEND_WRITE_ENABLE, CBSTATUS_COLOR_BLEND_BOUND,   CBSTATUS_COLOR_BLEND_BOUND,   VK_DBG_MSG_ERROR,  DRAWSTATE_COLOR_BLEND_NOT_BOUND,   "Color-blend object not bound to this command buffer");
    result4 = validate_status(cb, CBSTATUS_DEPTH_STENCIL_WRITE_ENABLE, CBSTATUS_DEPTH_STENCIL_BOUND, CBSTATUS_DEPTH_STENCIL_BOUND, VK_DBG_MSG_ERROR,  DRAWSTATE_DEPTH_STENCIL_NOT_BOUND, "Depth-stencil object not bound to this command buffer");
    return ((result1 == VK_TRUE) && (result2 == VK_TRUE) && (result3 == VK_TRUE) && (result4 == VK_TRUE));
}
// Print the last bound dynamic state
static void printDynamicState(const VkCmdBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        loader_platform_thread_lock_mutex(&globalLock);
        char str[4*1024];
        for (uint32_t i = 0; i < VK_NUM_STATE_BIND_POINT; i++) {
            if (pCB->lastBoundDynamicState[i]) {
                sprintf(str, "Reporting CreateInfo for currently bound %s object %p", string_VkStateBindPoint((VkStateBindPoint)i), pCB->lastBoundDynamicState[i]->stateObj);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pCB->lastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", str);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pCB->lastBoundDynamicState[i]->stateObj, 0, DRAWSTATE_NONE, "DS", dynamic_display(pCB->lastBoundDynamicState[i]->pCreateInfo, "  ").c_str());
                break;
            }
            else {
                sprintf(str, "No dynamic state of type %s bound", string_VkStateBindPoint((VkStateBindPoint)i));
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
static PIPELINE_NODE* getPipeline(VkPipeline pipeline)
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
static VkSamplerCreateInfo* getSamplerCreateInfo(const VkSampler sampler)
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
static void initPipeline(PIPELINE_NODE* pPipeline, const VkGraphicsPipelineCreateInfo* pCreateInfo)
{
    // First init create info, we'll shadow the structs as we go down the tree
    // TODO : Validate that no create info is incorrectly replicated
    memcpy(&pPipeline->graphicsPipelineCI, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo));
    GENERIC_HEADER* pTrav = (GENERIC_HEADER*)pCreateInfo->pNext;
    GENERIC_HEADER* pPrev = (GENERIC_HEADER*)&pPipeline->graphicsPipelineCI; // Hold prev ptr to tie chain of structs together
    size_t bufferSize = 0;
    VkPipelineVertexInputCreateInfo* pVICI = NULL;
    VkPipelineCbStateCreateInfo*     pCBCI = NULL;
    VkPipelineShaderStageCreateInfo* pTmpPSSCI = NULL;
    while (pTrav) {
        switch (pTrav->sType) {
            case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
                pTmpPSSCI = (VkPipelineShaderStageCreateInfo*)pTrav;
                switch (pTmpPSSCI->shader.stage) {
                    case VK_SHADER_STAGE_VERTEX:
                        pPrev->pNext = &pPipeline->vsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->vsCI;
                        memcpy(&pPipeline->vsCI, pTmpPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                        break;
                    case VK_SHADER_STAGE_TESS_CONTROL:
                        pPrev->pNext = &pPipeline->tcsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->tcsCI;
                        memcpy(&pPipeline->tcsCI, pTmpPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                        break;
                    case VK_SHADER_STAGE_TESS_EVALUATION:
                        pPrev->pNext = &pPipeline->tesCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->tesCI;
                        memcpy(&pPipeline->tesCI, pTmpPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                        break;
                    case VK_SHADER_STAGE_GEOMETRY:
                        pPrev->pNext = &pPipeline->gsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->gsCI;
                        memcpy(&pPipeline->gsCI, pTmpPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                        break;
                    case VK_SHADER_STAGE_FRAGMENT:
                        pPrev->pNext = &pPipeline->fsCI;
                        pPrev = (GENERIC_HEADER*)&pPipeline->fsCI;
                        memcpy(&pPipeline->fsCI, pTmpPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                        break;
                    case VK_SHADER_STAGE_COMPUTE:
                        // TODO : Flag error, CS is specified through VkComputePipelineCreateInfo
                        break;
                    default:
                        // TODO : Flag error
                        break;
                }
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
                pPrev->pNext = &pPipeline->vertexInputCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->vertexInputCI;
                memcpy((void*)&pPipeline->vertexInputCI, pTrav, sizeof(VkPipelineVertexInputCreateInfo));
                // Copy embedded ptrs
                pVICI = (VkPipelineVertexInputCreateInfo*)pTrav;
                pPipeline->vtxBindingCount = pVICI->bindingCount;
                if (pPipeline->vtxBindingCount) {
                    pPipeline->pVertexBindingDescriptions = new VkVertexInputBindingDescription[pPipeline->vtxBindingCount];
                    bufferSize = pPipeline->vtxBindingCount * sizeof(VkVertexInputBindingDescription);
                    memcpy((void*)pPipeline->pVertexBindingDescriptions, ((VkPipelineVertexInputCreateInfo*)pTrav)->pVertexBindingDescriptions, bufferSize);
                }
                pPipeline->vtxAttributeCount = pVICI->attributeCount;
                if (pPipeline->vtxAttributeCount) {
                    pPipeline->pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[pPipeline->vtxAttributeCount];
                    bufferSize = pPipeline->vtxAttributeCount * sizeof(VkVertexInputAttributeDescription);
                    memcpy((void*)pPipeline->pVertexAttributeDescriptions, ((VkPipelineVertexInputCreateInfo*)pTrav)->pVertexAttributeDescriptions, bufferSize);
                }
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->iaStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->iaStateCI;
                memcpy((void*)&pPipeline->iaStateCI, pTrav, sizeof(VkPipelineIaStateCreateInfo));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->tessStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->tessStateCI;
                memcpy((void*)&pPipeline->tessStateCI, pTrav, sizeof(VkPipelineTessStateCreateInfo));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->vpStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->vpStateCI;
                memcpy((void*)&pPipeline->vpStateCI, pTrav, sizeof(VkPipelineVpStateCreateInfo));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->rsStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->rsStateCI;
                memcpy((void*)&pPipeline->rsStateCI, pTrav, sizeof(VkPipelineRsStateCreateInfo));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->msStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->msStateCI;
                memcpy((void*)&pPipeline->msStateCI, pTrav, sizeof(VkPipelineMsStateCreateInfo));
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->cbStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->cbStateCI;
                memcpy((void*)&pPipeline->cbStateCI, pTrav, sizeof(VkPipelineCbStateCreateInfo));
                // Copy embedded ptrs
                pCBCI = (VkPipelineCbStateCreateInfo*)pTrav;
                pPipeline->attachmentCount = pCBCI->attachmentCount;
                if (pPipeline->attachmentCount) {
                    pPipeline->pAttachments = new VkPipelineCbAttachmentState[pPipeline->attachmentCount];
                    bufferSize = pPipeline->attachmentCount * sizeof(VkPipelineCbAttachmentState);
                    memcpy((void*)pPipeline->pAttachments, ((VkPipelineCbStateCreateInfo*)pTrav)->pAttachments, bufferSize);
                }
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
                pPrev->pNext = &pPipeline->dsStateCI;
                pPrev = (GENERIC_HEADER*)&pPipeline->dsStateCI;
                memcpy((void*)&pPipeline->dsStateCI, pTrav, sizeof(VkPipelineDsStateCreateInfo));
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
    if (pipelineMap.size() <= 0)
        return;
    for (unordered_map<VkPipeline, PIPELINE_NODE*>::iterator ii=pipelineMap.begin(); ii!=pipelineMap.end(); ++ii) {
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
static uint32_t getNumSamples(const VkPipeline pipeline)
{
    PIPELINE_NODE* pPipe = pipelineMap[pipeline];
    if (VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO == pPipe->msStateCI.sType) {
        if (pPipe->msStateCI.multisampleEnable)
            return pPipe->msStateCI.samples;
    }
    return 1;
}
// Validate state related to the PSO
static void validatePipelineState(const GLOBAL_CB_NODE* pCB, const VkPipelineBindPoint pipelineBindPoint, const VkPipeline pipeline)
{
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
        // Verify that any MSAA request in PSO matches sample# in bound FB
        uint32_t psoNumSamples = getNumSamples(pipeline);
        if (pCB->activeRenderPass) {
            VkRenderPassCreateInfo* pRPCI = renderPassMap[pCB->activeRenderPass];
            VkFramebufferCreateInfo* pFBCI = frameBufferMap[pCB->framebuffer];
            if ((psoNumSamples != pFBCI->sampleCount) || (psoNumSamples != pRPCI->sampleCount)) {
                char str[1024];
                sprintf(str, "Num samples mismatch! Binding PSO (%p) with %u samples while current RenderPass (%p) w/ %u samples uses FB (%p) with %u samples!", (void*)pipeline, psoNumSamples, (void*)pCB->activeRenderPass, pRPCI->sampleCount, (void*)pCB->framebuffer, pFBCI->sampleCount);
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
static POOL_NODE* getPoolNode(VkDescriptorPool pool)
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
static SET_NODE* getSetNode(VkDescriptorSet set)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (setMap.find(set) == setMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return setMap[set];
}

static LAYOUT_NODE* getLayoutNode(const VkDescriptorSetLayout layout) {
    loader_platform_thread_lock_mutex(&globalLock);
    if (layoutMap.find(layout) == layoutMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return layoutMap[layout];
}
// Return 1 if update struct is of valid type, 0 otherwise
static bool32_t validUpdateStruct(const GENERIC_HEADER* pUpdateStruct)
{
    char str[1024];
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            return 1;
        default:
            sprintf(str, "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS", str);
            return 0;
    }
}
// For given update struct, return binding
static uint32_t getUpdateBinding(const GENERIC_HEADER* pUpdateStruct)
{
    char str[1024];
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            return ((VkWriteDescriptorSet*)pUpdateStruct)->destBinding;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            return ((VkCopyDescriptorSet*)pUpdateStruct)->destBinding;
        default:
            sprintf(str, "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS", str);
            return 0xFFFFFFFF;
    }
}
// Return count for given update struct
static uint32_t getUpdateArrayIndex(const GENERIC_HEADER* pUpdateStruct)
{
    char str[1024];
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            return ((VkWriteDescriptorSet*)pUpdateStruct)->destArrayElement;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            // TODO : Need to understand this case better and make sure code is correct
            return ((VkCopyDescriptorSet*)pUpdateStruct)->destArrayElement;
        default:
            sprintf(str, "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS", str);
            return 0;
    }
}
// Return count for given update struct
static uint32_t getUpdateCount(const GENERIC_HEADER* pUpdateStruct)
{
    char str[1024];
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            return ((VkWriteDescriptorSet*)pUpdateStruct)->count;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            // TODO : Need to understand this case better and make sure code is correct
            return ((VkCopyDescriptorSet*)pUpdateStruct)->count;
        default:
            sprintf(str, "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS", str);
            return 0;
    }
}
// For given Layout Node and binding, return index where that binding begins
static uint32_t getBindingStartIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i<binding; i++) {
        offsetIndex += pLayout->createInfo.pBinding[i].arraySize;
    }
    return offsetIndex;
}
// For given layout node and binding, return last index that is updated
static uint32_t getBindingEndIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i<=binding; i++) {
        offsetIndex += pLayout->createInfo.pBinding[i].arraySize;
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
    VkDescriptorType actualType;
    uint32_t i = 0;
    char str[1024];
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            actualType = ((VkWriteDescriptorSet*)pUpdateStruct)->descriptorType;
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            /* no need to validate */
            return 1;
            break;
        default:
            sprintf(str, "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS", str);
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
    VkWriteDescriptorSet* pWDS = NULL;
    VkCopyDescriptorSet* pCDS = NULL;
    size_t array_size = 0;
    size_t base_array_size = 0;
    size_t total_array_size = 0;
    size_t baseBuffAddr = 0;
    char str[1024];
    switch (pUpdate->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            pWDS = new VkWriteDescriptorSet;
            pNewNode = (GENERIC_HEADER*)pWDS;
            memcpy(pWDS, pUpdate, sizeof(VkWriteDescriptorSet));
            pWDS->pDescriptors = new VkDescriptorInfo[pWDS->count];
            array_size = sizeof(VkDescriptorInfo) * pWDS->count;
            memcpy((void*)pWDS->pDescriptors, ((VkWriteDescriptorSet*)pUpdate)->pDescriptors, array_size);
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            pCDS = new VkCopyDescriptorSet;
            pUpdate = (GENERIC_HEADER*)pCDS;
            memcpy(pCDS, pUpdate, sizeof(VkCopyDescriptorSet));
            break;
        default:
            sprintf(str, "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdate->sType), pUpdate->sType);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS", str);
            return NULL;
    }
    // Make sure that pNext for the end of shadow copy is NULL
    pNewNode->pNext = NULL;
    return pNewNode;
}
// update DS mappings based on ppUpdateArray
static bool32_t dsUpdate(VkStructureType type, uint32_t updateCount, const void* pUpdateArray)
{
    const VkWriteDescriptorSet *pWDS = NULL;
    const VkCopyDescriptorSet *pCDS = NULL;
    bool32_t result = 1;

    if (type == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
        pWDS = (const VkWriteDescriptorSet *) pUpdateArray;
    else
        pCDS = (const VkCopyDescriptorSet *) pUpdateArray;

    loader_platform_thread_lock_mutex(&globalLock);
    LAYOUT_NODE* pLayout = NULL;
    VkDescriptorSetLayoutCreateInfo* pLayoutCI = NULL;
    // TODO : If pCIList is NULL, flag error
    // Perform all updates
    for (uint32_t i = 0; i < updateCount; i++) {
        VkDescriptorSet ds = (pWDS) ? pWDS->destSet : pCDS->destSet;
        SET_NODE* pSet = setMap[ds]; // getSetNode() without locking
        g_lastBoundDescriptorSet = pSet->set;
        GENERIC_HEADER* pUpdate = (pWDS) ? (GENERIC_HEADER*) &pWDS[i] : (GENERIC_HEADER*) &pCDS[i];
        pLayout = pSet->pLayout;
        // First verify valid update struct
        if (!validUpdateStruct(pUpdate)) {
            result = 0;
            break;
        }
        // Make sure that binding is within bounds
        if (pLayout->createInfo.count < getUpdateBinding(pUpdate)) {
            char str[1024];
            sprintf(str, "Descriptor Set %p does not have binding to match update binding %u for update type %s!", ds, getUpdateBinding(pUpdate), string_VkStructureType(pUpdate->sType));
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS", str);
            result = 0;
        }
        else {
            // Next verify that update falls within size of given binding
            if (getBindingEndIndex(pLayout, getUpdateBinding(pUpdate)) < getUpdateEndIndex(pLayout, pUpdate)) {
                char str[48*1024]; // TODO : Keep count of layout CI structs and size this string dynamically based on that count
                pLayoutCI = &pLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                sprintf(str, "Descriptor update type of %s is out of bounds for matching binding %u in Layout w/ CI:\n%s!", string_VkStructureType(pUpdate->sType), getUpdateBinding(pUpdate), DSstr.c_str());
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS", str);
                result = 0;
            }
            else { // TODO : should we skip update on a type mismatch or force it?
                // Layout bindings match w/ update ok, now verify that update is of the right type
                if (!validateUpdateType(pLayout, pUpdate)) {
                    char str[1024];
                    sprintf(str, "Descriptor update type of %s does not match overlapping binding type!", string_VkStructureType(pUpdate->sType));
                    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, ds, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS", str);
                    result = 0;
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
                        result = 0;
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
    return result;
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
        VkWriteDescriptorSet * pWDS = NULL;
        VkCopyDescriptorSet * pCDS = NULL;
        void** ppToFree = NULL;
        switch (pFreeUpdate->sType)
        {
            case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
                pWDS = (VkWriteDescriptorSet*)pFreeUpdate;
                if (pWDS->pDescriptors)
                    delete[] pWDS->pDescriptors;
                break;
            case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
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
    if (poolMap.size() <= 0)
        return;
    for (unordered_map<VkDescriptorPool, POOL_NODE*>::iterator ii=poolMap.begin(); ii!=poolMap.end(); ++ii) {
        SET_NODE* pSet = (*ii).second->pSets;
        SET_NODE* pFreeSet = pSet;
        while (pSet) {
            pFreeSet = pSet;
            pSet = pSet->pNext;
            // Freeing layouts handled in deleteLayouts() function
            // Free Update shadow struct tree
            freeShadowUpdateTree(pFreeSet);
            if (pFreeSet->ppDescriptors) {
                delete[] pFreeSet->ppDescriptors;
            }
            delete pFreeSet;
        }
        if ((*ii).second->createInfo.pTypeCount) {
            delete[] (*ii).second->createInfo.pTypeCount;
        }
        delete (*ii).second;
    }
}
// WARN : Once deleteLayouts() called, any layout ptrs in Pool/Set data structure will be invalid
// NOTE : Calls to this function should be wrapped in mutex
static void deleteLayouts()
{
    if (layoutMap.size() <= 0)
        return;
    for (unordered_map<VkDescriptorSetLayout, LAYOUT_NODE*>::iterator ii=layoutMap.begin(); ii!=layoutMap.end(); ++ii) {
        LAYOUT_NODE* pLayout = (*ii).second;
        if (pLayout->createInfo.pBinding) {
            for (uint32_t i=0; i<pLayout->createInfo.count; i++) {
                if (pLayout->createInfo.pBinding[i].pImmutableSamplers)
                    delete[] pLayout->createInfo.pBinding[i].pImmutableSamplers;
            }
            delete[] pLayout->createInfo.pBinding;
        }
        if (pLayout->pTypes) {
            delete[] pLayout->pTypes;
        }
        delete pLayout;
    }
}
// Currently clearing a set is removing all previous updates to that set
//  TODO : Validate if this is correct clearing behavior
static void clearDescriptorSet(VkDescriptorSet set)
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

static void clearDescriptorPool(VkDescriptorPool pool)
{
    POOL_NODE* pPool = getPoolNode(pool);
    if (!pPool) {
        char str[1024];
        sprintf(str, "Unable to find pool node for pool %p specified in vkResetDescriptorPool() call", (void*)pool);
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
static GLOBAL_CB_NODE* getCBNode(VkCmdBuffer cb)
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
    if (cmdBufferMap.size() <= 0)
        return;
    for (unordered_map<VkCmdBuffer, GLOBAL_CB_NODE*>::iterator ii=cmdBufferMap.begin(); ii!=cmdBufferMap.end(); ++ii) {
        vector<CMD_NODE*> cmd_node_list = (*ii).second->pCmds;
        while (!cmd_node_list.empty()) {
            CMD_NODE* cmd_node = cmd_node_list.back();
            delete cmd_node;
            cmd_node_list.pop_back();
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
static void resetCB(const VkCmdBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        vector<CMD_NODE*> cmd_list = pCB->pCmds;
        while (!cmd_list.empty()) {
            delete cmd_list.back();
            cmd_list.pop_back();
        }
        pCB->pCmds.clear();
        // Reset CB state
        VkFlags saveFlags = pCB->flags;
        uint32_t saveQueueNodeIndex = pCB->queueNodeIndex;
        memset(pCB, 0, sizeof(GLOBAL_CB_NODE));
        pCB->cmdBuffer = cb;
        pCB->flags = saveFlags;
        pCB->queueNodeIndex = saveQueueNodeIndex;
        pCB->lastVtxBinding = MAX_BINDING;
    }
}
// Set PSO-related status bits for CB
static void set_cb_pso_status(GLOBAL_CB_NODE* pCB, const PIPELINE_NODE* pPipe)
{
    for (uint32_t i = 0; i < pPipe->cbStateCI.attachmentCount; i++) {
        if (0 != pPipe->pAttachments[i].channelWriteMask) {
            pCB->status |= CBSTATUS_COLOR_BLEND_WRITE_ENABLE;
        }
    }
    if (pPipe->dsStateCI.depthWriteEnable) {
        pCB->status |= CBSTATUS_DEPTH_STENCIL_WRITE_ENABLE;
    }
}
// Set dyn-state related status bits for an object node
static void set_cb_dyn_status(GLOBAL_CB_NODE* pNode, VkStateBindPoint stateBindPoint) {
    if (stateBindPoint == VK_STATE_BIND_POINT_VIEWPORT) {
        pNode->status |= CBSTATUS_VIEWPORT_BOUND;
    } else if (stateBindPoint == VK_STATE_BIND_POINT_RASTER) {
        pNode->status |= CBSTATUS_RASTER_BOUND;
    } else if (stateBindPoint == VK_STATE_BIND_POINT_COLOR_BLEND) {
        pNode->status |= CBSTATUS_COLOR_BLEND_BOUND;
    } else if (stateBindPoint == VK_STATE_BIND_POINT_DEPTH_STENCIL) {
        pNode->status |= CBSTATUS_DEPTH_STENCIL_BOUND;
    }
}
// Set the last bound dynamic state of given type
static void setLastBoundDynamicState(const VkCmdBuffer cmdBuffer, const VkDynamicStateObject state, const VkStateBindPoint sType)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        loader_platform_thread_lock_mutex(&globalLock);
        set_cb_dyn_status(pCB, sType);
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
static void printPipeline(const VkCmdBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        if (!pPipeTrav) {
            // nothing to print
        }
        else {
            string pipeStr = vk_print_vkgraphicspipelinecreateinfo(&pPipeTrav->graphicsPipelineCI, "{DS}").c_str();
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", pipeStr.c_str());
        }
    }
}
// Common Dot dumping code
static void dsCoreDumpDot(const VkDescriptorSet ds, FILE* pOutFile)
{
#if 0
    SET_NODE* pSet = getSetNode(ds);
    if (pSet) {
        POOL_NODE* pPool = getPoolNode(pSet->pool);
        char tmp_str[4*1024];
        fprintf(pOutFile, "subgraph cluster_DescriptorPool\n{\nlabel=\"Descriptor Pool\"\n");
        sprintf(tmp_str, "Pool (%p)", pPool->pool);
        char* pGVstr = vk_gv_print_vkdescriptorpoolcreateinfo(&pPool->createInfo, tmp_str);
        fprintf(pOutFile, "%s", pGVstr);
        free(pGVstr);
        fprintf(pOutFile, "subgraph cluster_DescriptorSet\n{\nlabel=\"Descriptor Set (%p)\"\n", pSet->set);
        sprintf(tmp_str, "Descriptor Set (%p)", pSet->set);
        LAYOUT_NODE* pLayout = pSet->pLayout;
        uint32_t layout_index = 0;
        ++layout_index;
        sprintf(tmp_str, "LAYOUT%u", layout_index);
        pGVstr = vk_gv_print_vkdescriptorsetlayoutcreateinfo(&pLayout->createInfo, tmp_str);
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
                    fprintf(pOutFile, "<TR><TD PORT=\"slot%u\">slot%u</TD><TD>%s</TD></TR>", i, i, string_VkStructureType(pSet->ppDescriptors[i]->sType));
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
            VkUpdateSamplers* pUS = NULL;
            VkUpdateSamplerTextures* pUST = NULL;
            VkUpdateImages* pUI = NULL;
            VkUpdateBuffers* pUB = NULL;
            VkUpdateAsCopy* pUAC = NULL;
            VkSamplerCreateInfo* pSCI = NULL;
            VkImageViewCreateInfo* pIVCI = NULL;
            VkBufferViewCreateInfo* pBVCI = NULL;
            void** ppNextPtr = NULL;
            void* pSaveNext = NULL;
            for (i=0; i < pSet->descriptorCount; i++) {
                if (pSet->ppDescriptors[i]) {
                    switch (pSet->ppDescriptors[i]->sType)
                    {
                        case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                            pUS = (VkUpdateSamplers*)pSet->ppDescriptors[i];
                            pSCI = getSamplerCreateInfo(pUS->pSamplers[i-pUS->arrayIndex]);
                            if (pSCI) {
                                sprintf(tmp_str, "SAMPLER%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vksamplercreateinfo(pSCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                            pUST = (VkUpdateSamplerTextures*)pSet->ppDescriptors[i];
                            pSCI = getSamplerCreateInfo(pUST->pSamplerImageViews[i-pUST->arrayIndex].sampler);
                            if (pSCI) {
                                sprintf(tmp_str, "SAMPLER%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vksamplercreateinfo(pSCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            pIVCI = getImageViewCreateInfo(pUST->pSamplerImageViews[i-pUST->arrayIndex].pImageView->view);
                            if (pIVCI) {
                                sprintf(tmp_str, "IMAGE_VIEW%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vkimageviewcreateinfo(pIVCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
                            pUI = (VkUpdateImages*)pSet->ppDescriptors[i];
                            pIVCI = getImageViewCreateInfo(pUI->pImageViews[i-pUI->arrayIndex].view);
                            if (pIVCI) {
                                sprintf(tmp_str, "IMAGE_VIEW%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vkimageviewcreateinfo(pIVCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
                            pUB = (VkUpdateBuffers*)pSet->ppDescriptors[i];
                            pBVCI = getBufferViewCreateInfo(pUB->pBufferViews[i-pUB->arrayIndex].view);
                            if (pBVCI) {
                                sprintf(tmp_str, "BUFFER_VIEW%u", i);
                                fprintf(pOutFile, "%s", vk_gv_print_vkbufferviewcreateinfo(pBVCI, tmp_str));
                                fprintf(pOutFile, "\"DESCRIPTORS\":slot%u -> \"%s\" [color=\"#%s\"];\n", i, tmp_str, edgeColors[colorIdx].c_str());
                            }
                            break;
                        case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
                            pUAC = (VkUpdateAsCopy*)pSet->ppDescriptors[i];
                            // TODO : Need to validate this code
                            // Save off pNext and set to NULL while printing this struct, then restore it
                            ppNextPtr = (void**)&pUAC->pNext;
                            pSaveNext = *ppNextPtr;
                            *ppNextPtr = NULL;
                            sprintf(tmp_str, "UPDATE_AS_COPY%u", i);
                            fprintf(pOutFile, "%s", vk_gv_print_vkupdateascopy(pUAC, tmp_str));
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
#endif
}
// Dump subgraph w/ DS info
static void dsDumpDot(const VkCmdBuffer cb, FILE* pOutFile)
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
        if (pCB && pCB->pCmds.size() > 0) {
            fprintf(pOutFile, "subgraph cluster_cmdBuffer%u\n{\nlabel=\"Command Buffer #%u\"\n", i, i);
            uint32_t instNum = 0;
            vector<CMD_NODE*> cmd_list = pCB->pCmds;
            for (vector<CMD_NODE*>::iterator ii= cmd_list.begin(); ii!= cmd_list.end(); ++ii) {
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
                pGVstr = dynamic_gv_display(g_lastBoundDynamicState[i]->pCreateInfo, string_VkStateBindPoint((VkStateBindPoint)i));
                fprintf(pOutFile, "%s", pGVstr);
                free(pGVstr);
            }
        }
        fprintf(pOutFile, "}\n"); // close dynamicState subgraph
        fprintf(pOutFile, "subgraph cluster_PipelineStateObject\n{\nlabel=\"Pipeline State Object\"\n");
        pGVstr = vk_gv_print_vkgraphicspipelinecreateinfo(&pPipeTrav->graphicsPipelineCI, "PSO HEAD");
        fprintf(pOutFile, "%s", pGVstr);
        free(pGVstr);
        fprintf(pOutFile, "}\n");
        dsCoreDumpDot(g_lastBoundDescriptorSet, pOutFile);
        fprintf(pOutFile, "}\n"); // close main graph "g"
        fclose(pOutFile);
    }
}
// Dump a GraphViz dot file showing the pipeline for a given CB
static void dumpDotFile(const VkCmdBuffer cb, string outFileName)
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
                    pGVstr = dynamic_gv_display(pCB->lastBoundDynamicState[i]->pCreateInfo, string_VkStateBindPoint((VkStateBindPoint)i));
                    fprintf(pOutFile, "%s", pGVstr);
                    free(pGVstr);
                }
            }
            fprintf(pOutFile, "}\n"); // close dynamicState subgraph
            fprintf(pOutFile, "subgraph cluster_PipelineStateObject\n{\nlabel=\"Pipeline State Object\"\n");
            pGVstr = vk_gv_print_vkgraphicspipelinecreateinfo(&pPipeTrav->graphicsPipelineCI, "PSO HEAD");
            fprintf(pOutFile, "%s", pGVstr);
            free(pGVstr);
            fprintf(pOutFile, "}\n");
            dsDumpDot(cb, pOutFile);
            fprintf(pOutFile, "}\n"); // close main graph "g"
            fclose(pOutFile);
        }
    }
}
// Verify bound Pipeline State Object
static bool validateBoundPipeline(const VkCmdBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB && pCB->lastBoundPipeline) {
        // First verify that we have a Node for bound pipeline
        PIPELINE_NODE *pPipeTrav = getPipeline(pCB->lastBoundPipeline);
        char str[1024];
        if (!pPipeTrav) {
            sprintf(str, "Can't find last bound Pipeline %p!", (void*)pCB->lastBoundPipeline);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NO_PIPELINE_BOUND, "DS", str);
            return false;
        }
        else {
            // Verify Vtx binding
            if (MAX_BINDING != pCB->lastVtxBinding) {
                if (pCB->lastVtxBinding >= pPipeTrav->vtxBindingCount) {
                    if (0 == pPipeTrav->vtxBindingCount) {
                        sprintf(str, "Vtx Buffer Index %u was bound, but no vtx buffers are attached to PSO.", pCB->lastVtxBinding);
                        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS", str);
                        return false;
                    }
                    else {
                        sprintf(str, "Vtx binding Index of %u exceeds PSO pVertexBindingDescriptions max array index of %u.", pCB->lastVtxBinding, (pPipeTrav->vtxBindingCount - 1));
                        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS", str);
                        return false;
                    }
                }
                else {
                    string tmpStr = vk_print_vkvertexinputbindingdescription(&pPipeTrav->pVertexBindingDescriptions[pCB->lastVtxBinding], "{DS}INFO : ").c_str();
                    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmpStr.c_str());
                }
            }
        }
        return true;
    }
    return false;
}
// Print details of DS config to stdout
static void printDSConfig(const VkCmdBuffer cb)
{
    char tmp_str[1024];
    char ds_config_str[1024*256] = {0}; // TODO : Currently making this buffer HUGE w/o overrun protection.  Need to be smarter, start smaller, and grow as needed.
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB && pCB->lastBoundDescriptorSet) {
        SET_NODE* pSet = getSetNode(pCB->lastBoundDescriptorSet);
        POOL_NODE* pPool = getPoolNode(pSet->pool);
        // Print out pool details
        sprintf(tmp_str, "Details for pool %p.", (void*)pPool->pool);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
        string poolStr = vk_print_vkdescriptorpoolcreateinfo(&pPool->createInfo, " ");
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
        string DSLstr = vk_print_vkdescriptorsetlayoutcreateinfo(&pLayout->createInfo, prefix).c_str();
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
            if (0 != pSet->descriptorCount) {
                sprintf(tmp_str, "No Update Chain for descriptor set %p which has %u descriptors (vkUpdateDescriptors has not been called)", (void*)pSet->set, pSet->descriptorCount);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
            }
            else {
                sprintf(tmp_str, "FYI: No descriptors in descriptor set %p.", (void*)pSet->set);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", tmp_str);
            }
        }
    }
}

static void printCB(const VkCmdBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cb);
    if (pCB && pCB->pCmds.size() > 0) {
        char str[1024];
        sprintf(str, "Cmds in CB %p", (void*)cb);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_NONE, "DS", str);
        vector<CMD_NODE*> pCmds = pCB->pCmds;
        for (vector<CMD_NODE*>::iterator ii=pCmds.begin(); ii!=pCmds.end(); ++ii) {
            sprintf(str, "  CMD#%lu: %s", (*ii)->cmdNumber, cmdTypeToString((*ii)->type).c_str());
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, cb, 0, DRAWSTATE_NONE, "DS", str);
        }
    }
    else {
        // Nothing to print
    }
}


static void synchAndPrintDSConfig(const VkCmdBuffer cb)
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
            int retval = system("/usr/bin/dot pipeline_dump.dot -Tsvg -o pipeline_dump.svg");
            assert(retval != -1);
        }
#endif // WIN32
    }
}

static VkLayerDispatchTable * initDeviceTable(const VkBaseLayerObject *devw)
{
    VkLayerDispatchTable *pTable;

    assert(devw);
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) (devw->baseObject);

    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) *ppDisp);
    if (it == tableMap.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap[(void *) *ppDisp] = pTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, (PFN_vkGetDeviceProcAddr) devw->pGPA, (VkDevice) devw->nextObject);

    return pTable;
}

static VkLayerInstanceDispatchTable * initInstanceTable(const VkBaseLayerObject *instw)
{
    VkLayerInstanceDispatchTable *pTable;
    assert(instw);
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instw->baseObject;

    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap.find((void *) *ppDisp);
    if (it == tableInstanceMap.end())
    {
        pTable =  new VkLayerInstanceDispatchTable;
        tableInstanceMap[(void *) *ppDisp] = pTable;
    } else
    {
        return it->second;
    }

    layer_init_instance_dispatch_table(pTable, (PFN_vkGetInstanceProcAddr) instw->pGPA, (VkInstance) instw->nextObject);

    return pTable;
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

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{
   VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable *pTable = tableInstanceMap[pDisp];
    VkResult res = pTable->DestroyInstance(instance);
    tableInstanceMap.erase(pDisp);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) gpu;
    VkLayerInstanceDispatchTable* pInstTable = tableInstanceMap[*ppDisp];
    VkResult result = pInstTable->CreateDevice(gpu, pCreateInfo, pDevice);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
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

    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->DestroyDevice(device);
    tableMap.erase(pDisp);
    return result;
}

struct extProps {
    uint32_t version;
    const char * const name;
};
#define DRAW_STATE_LAYER_EXT_ARRAY_SIZE 5
static const struct extProps dsExts[DRAW_STATE_LAYER_EXT_ARRAY_SIZE] = {
    // TODO what is the version?
    0x10, "DrawState",
    0x10, "Validation",
    0x10, "drawStateDumpDotFile",
    0x10, "drawStateDumpCommandBufferDotFile",
    0x10, "drawStateDumpPngFile"
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
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
            *count = DRAW_STATE_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= DRAW_STATE_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = dsExts[extensionIndex].version;
            strncpy(ext_props->extName, dsExts[extensionIndex].name,
                                        VK_MAX_EXTENSION_NAME);
            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalDevice gpu, size_t maxStringSize, size_t* pLayerCount, char* const* pOutLayers, void* pReserved)
{
    if (gpu != NULL)
    {
        VkLayerInstanceDispatchTable* pTable = initInstanceTable((const VkBaseLayerObject *) gpu);

        VkResult result = pTable->EnumerateLayers(gpu, maxStringSize, pLayerCount, pOutLayers, pReserved);
        return result;
    } else {
        if (pLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return VK_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pLayerCount = 1;
        strncpy((char *) pOutLayers[0], "DrawState", maxStringSize);
        return VK_SUCCESS;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t cmdBufferCount, const VkCmdBuffer* pCmdBuffers, VkFence fence)
{
    GLOBAL_CB_NODE* pCB = NULL;
    for (uint32_t i=0; i < cmdBufferCount; i++) {
        // Validate that cmd buffers have been updated
        pCB = getCBNode(pCmdBuffers[i]);
        loader_platform_thread_lock_mutex(&globalLock);
        if (CB_UPDATE_COMPLETE != pCB->state) {
            // Flag error for using CB w/o vkEndCommandBuffer() called
            char str[1024];
            sprintf(str, "You must call vkEndCommandBuffer() on CB %p before this call to vkQueueSubmit()!", pCB->cmdBuffer);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pCB->cmdBuffer, 0, DRAWSTATE_NO_END_CMD_BUFFER, "DS", str);
            loader_platform_thread_unlock_mutex(&globalLock);
            return VK_ERROR_UNKNOWN;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) queue;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyObject(VkDevice device, VkObjectType objType, VkObject object)
{
    // TODO : When wrapped objects (such as dynamic state) are destroyed, need to clean up memory
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->DestroyObject(device, objType, object);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateBufferView(device, pCreateInfo, pView);
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateImageView(device, pCreateInfo, pView);
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

static void track_pipeline(const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    // Create LL HEAD for this Pipeline
    loader_platform_thread_lock_mutex(&globalLock);
    PIPELINE_NODE* pPipeNode = new PIPELINE_NODE;
    memset((void*)pPipeNode, 0, sizeof(PIPELINE_NODE));
    pPipeNode->pipeline = *pPipeline;
    initPipeline(pPipeNode, pCreateInfo);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    // Create LL HEAD for this Pipeline
    char str[1024];
    sprintf(str, "Created Gfx Pipeline %p", (void*)*pPipeline);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, *pPipeline, 0, DRAWSTATE_NONE, "DS", str);

    track_pipeline(pCreateInfo, pPipeline);

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(
        VkDevice device,
        const VkGraphicsPipelineCreateInfo* pCreateInfo,
        VkPipeline basePipeline,
        VkPipeline* pPipeline)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
    // Create LL HEAD for this Pipeline
    char str[1024];
    sprintf(str, "Created Gfx Pipeline %p (derived from pipeline %p)", (void*)*pPipeline, basePipeline);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, *pPipeline, 0, DRAWSTATE_NONE, "DS", str);

    track_pipeline(pCreateInfo, pPipeline);

    loader_platform_thread_unlock_mutex(&globalLock);

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateSampler(device, pCreateInfo, pSampler);
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    if (VK_SUCCESS == result) {
        LAYOUT_NODE* pNewNode = new LAYOUT_NODE;
        if (NULL == pNewNode) {
            char str[1024];
            sprintf(str, "Out of memory while attempting to allocate LAYOUT_NODE in vkCreateDescriptorSetLayout()");
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, *pSetLayout, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
        }
        memset(pNewNode, 0, sizeof(LAYOUT_NODE));
        memcpy((void*)&pNewNode->createInfo, pCreateInfo, sizeof(VkDescriptorSetLayoutCreateInfo));
        pNewNode->createInfo.pBinding = new VkDescriptorSetLayoutBinding[pCreateInfo->count];
        memcpy((void*)pNewNode->createInfo.pBinding, pCreateInfo->pBinding, sizeof(VkDescriptorSetLayoutBinding)*pCreateInfo->count);
        uint32_t totalCount = 0;
        for (uint32_t i=0; i<pCreateInfo->count; i++) {
            totalCount += pCreateInfo->pBinding[i].arraySize;
            if (pCreateInfo->pBinding[i].pImmutableSamplers) {
                VkSampler** ppIS = (VkSampler**)&pNewNode->createInfo.pBinding[i].pImmutableSamplers;
                *ppIS = new VkSampler[pCreateInfo->pBinding[i].arraySize];
                memcpy(*ppIS, pCreateInfo->pBinding[i].pImmutableSamplers, pCreateInfo->pBinding[i].arraySize*sizeof(VkSampler));
            }
        }
        if (totalCount > 0) {
            pNewNode->pTypes = new VkDescriptorType[totalCount];
            uint32_t offset = 0;
            uint32_t j = 0;
            for (uint32_t i=0; i<pCreateInfo->count; i++) {
                for (j = 0; j < pCreateInfo->pBinding[i].arraySize; j++) {
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

VkResult VKAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout* pPipelineLayout)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);
    if (VK_SUCCESS == result) {
        // TODO : Need to capture the pipeline layout
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    if (VK_SUCCESS == result) {
        // Insert this pool into Global Pool LL at head
        char str[1024];
        sprintf(str, "Created Descriptor Pool %p", (void*)*pDescriptorPool);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, (VkObject)pDescriptorPool, 0, DRAWSTATE_NONE, "DS", str);
        loader_platform_thread_lock_mutex(&globalLock);
        POOL_NODE* pNewNode = new POOL_NODE;
        if (NULL == pNewNode) {
            char str[1024];
            sprintf(str, "Out of memory while attempting to allocate POOL_NODE in vkCreateDescriptorPool()");
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, (VkObject)*pDescriptorPool, 0, DRAWSTATE_OUT_OF_MEMORY, "DS", str);
        }
        else {
            memset(pNewNode, 0, sizeof(POOL_NODE));
            VkDescriptorPoolCreateInfo* pCI = (VkDescriptorPoolCreateInfo*)&pNewNode->createInfo;
            memcpy((void*)pCI, pCreateInfo, sizeof(VkDescriptorPoolCreateInfo));
            if (pNewNode->createInfo.count) {
                size_t typeCountSize = pNewNode->createInfo.count * sizeof(VkDescriptorTypeCount);
                pNewNode->createInfo.pTypeCount = new VkDescriptorTypeCount[typeCountSize];
                memcpy((void*)pNewNode->createInfo.pTypeCount, pCreateInfo->pTypeCount, typeCountSize);
            }
            pNewNode->poolUsage  = poolUsage;
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

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->ResetDescriptorPool(device, descriptorPool);
    if (VK_SUCCESS == result) {
        clearDescriptorPool(descriptorPool);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets, uint32_t* pCount)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->AllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
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

VK_LAYER_EXPORT void VKAPI vkClearDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    for (uint32_t i = 0; i < count; i++) {
        clearDescriptorSet(pDescriptorSets[i]);
    }
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->ClearDescriptorSets(device, descriptorPool, count, pDescriptorSets);
}

VK_LAYER_EXPORT VkResult VKAPI vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t copyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    if (dsUpdate(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, writeCount, pDescriptorWrites) &&
        dsUpdate(VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET, copyCount, pDescriptorCopies)) {
        VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
        VkLayerDispatchTable *pTable = tableMap[pDisp];
        return pTable->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
    }
    return VK_ERROR_UNKNOWN;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(VkDevice device, const VkDynamicVpStateCreateInfo* pCreateInfo, VkDynamicVpState* pState)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateDynamicViewportState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_POINT_VIEWPORT);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(VkDevice device, const VkDynamicRsStateCreateInfo* pCreateInfo, VkDynamicRsState* pState)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateDynamicRasterState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_POINT_RASTER);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(VkDevice device, const VkDynamicCbStateCreateInfo* pCreateInfo, VkDynamicCbState* pState)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateDynamicColorBlendState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_POINT_COLOR_BLEND);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(VkDevice device, const VkDynamicDsStateCreateInfo* pCreateInfo, VkDynamicDsState* pState)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    insertDynamicState(*pState, (GENERIC_HEADER*)pCreateInfo, VK_STATE_BIND_POINT_DEPTH_STENCIL);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
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

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    if (VK_SUCCESS == result) {
        GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
        if (pCB) {
            if (CB_NEW != pCB->state)
                resetCB(cmdBuffer);
            pCB->state = CB_UPDATE_ACTIVE;
            if (pBeginInfo->pNext) {
                VkCmdBufferGraphicsBeginInfo* pCbGfxBI = (VkCmdBufferGraphicsBeginInfo*)pBeginInfo->pNext;
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

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCmdBuffer cmdBuffer)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->EndCommandBuffer(cmdBuffer);
    if (VK_SUCCESS == result) {
        GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
        if (pCB) {
            pCB->state = CB_UPDATE_COMPLETE;
            // Reset CB status flags
            pCB->status = 0;
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

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCmdBuffer cmdBuffer)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->ResetCommandBuffer(cmdBuffer);
    if (VK_SUCCESS == result) {
        resetCB(cmdBuffer);
        updateCBTracking(cmdBuffer);
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
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
            VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
            VkLayerDispatchTable *pTable = tableMap[pDisp];
            pTable->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
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
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(VkCmdBuffer cmdBuffer, VkStateBindPoint stateBindPoint, VkDynamicStateObject state)
{
    setLastBoundDynamicState(cmdBuffer, state, stateBindPoint);
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BINDDESCRIPTORSETS);
        if (validateBoundPipeline(cmdBuffer)) {
            for (uint32_t i=0; i<setCount; i++) {
                if (getSetNode(pDescriptorSets[i])) {
                    loader_platform_thread_lock_mutex(&globalLock);
                    pCB->lastBoundDescriptorSet = pDescriptorSets[i];
                    pCB->boundDescriptorSets.push_back(pDescriptorSets[i]);
                    g_lastBoundDescriptorSet = pDescriptorSets[i];
                    loader_platform_thread_unlock_mutex(&globalLock);
                    char str[1024];
                    sprintf(str, "DS %p bound on pipeline %s", (void*)pDescriptorSets[i], string_VkPipelineBindPoint(pipelineBindPoint));
                    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS", str);
                }
                else {
                    char str[1024];
                    sprintf(str, "Attempt to bind DS %p that doesn't exist!", (void*)pDescriptorSets[i]);
                    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pDescriptorSets[i], 0, DRAWSTATE_INVALID_SET, "DS", str);
                }
            }
            VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
            VkLayerDispatchTable *pTable = tableMap[pDisp];
            pTable->CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
        }
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    if (pCB) {
        /* TODO: Need to track all the vertex buffers, not just last one */
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_BINDVERTEXBUFFER);
        pCB->lastVtxBinding = startBinding + bindingCount -1;
        if (validateBoundPipeline(cmdBuffer)) {
            VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
            VkLayerDispatchTable *pTable = tableMap[pDisp];
            pTable->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
        }
    } else {
        char str[1024];
        sprintf(str, "Attempt to use CmdBuffer %p that doesn't exist!", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS", str);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VkCmdBuffer cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    bool32_t valid = VK_FALSE;
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAW);
        pCB->drawCount[DRAW]++;
        loader_platform_thread_lock_mutex(&globalLock);
        valid = validate_draw_state_flags(cmdBuffer);
        loader_platform_thread_unlock_mutex(&globalLock);
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
    if (valid) {
        VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
        VkLayerDispatchTable *pTable = tableMap[pDisp];
        pTable->CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VkCmdBuffer cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    bool32_t valid = VK_FALSE;
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAWINDEXED);
        pCB->drawCount[DRAW_INDEXED]++;
        loader_platform_thread_lock_mutex(&globalLock);
        valid = validate_draw_state_flags(cmdBuffer);
        loader_platform_thread_unlock_mutex(&globalLock);
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
    if (valid) {
        VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
        VkLayerDispatchTable *pTable = tableMap[pDisp];
        pTable->CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    bool32_t valid = VK_FALSE;
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAWINDIRECT);
        pCB->drawCount[DRAW_INDIRECT]++;
        loader_platform_thread_lock_mutex(&globalLock);
        valid = validate_draw_state_flags(cmdBuffer);
        loader_platform_thread_unlock_mutex(&globalLock);
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
    if (valid) {
        VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
        VkLayerDispatchTable *pTable = tableMap[pDisp];
        pTable->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    GLOBAL_CB_NODE* pCB = getCBNode(cmdBuffer);
    bool32_t valid = VK_FALSE;
    if (pCB) {
        updateCBTracking(cmdBuffer);
        addCmd(pCB, CMD_DRAWINDEXEDINDIRECT);
        pCB->drawCount[DRAW_INDEXED_INDIRECT]++;
        loader_platform_thread_lock_mutex(&globalLock);
        valid = validate_draw_state_flags(cmdBuffer);
        loader_platform_thread_unlock_mutex(&globalLock);
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
    if (valid) {
        VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
        VkLayerDispatchTable *pTable = tableMap[pDisp];
        pTable->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VkCmdBuffer cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdDispatch(cmdBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VkCmdBuffer cmdBuffer,
                                             VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkImage destImage,
                                             VkImageLayout destImageLayout,
                                             uint32_t regionCount, const VkImageCopy* pRegions)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VkCmdBuffer cmdBuffer,
                                             VkImage srcImage, VkImageLayout srcImageLayout,
                                             VkImage destImage, VkImageLayout destImageLayout,
                                             uint32_t regionCount, const VkImageBlit* pRegions,
                                             VkTexFilter filter)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCmdBuffer cmdBuffer,
                                                     VkBuffer srcBuffer,
                                                     VkImage destImage, VkImageLayout destImageLayout,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCmdBuffer cmdBuffer,
                                                     VkImage srcImage, VkImageLayout srcImageLayout,
                                                     VkBuffer destBuffer,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize dataSize, const uint32_t* pData)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize fillSize, uint32_t data)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(
        VkCmdBuffer cmdBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearColor *pColor,
        uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(VkCmdBuffer cmdBuffer,
                                                     VkImage image, VkImageLayout imageLayout,
                                                     float depth, uint32_t stencil,
                                                     uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VkCmdBuffer cmdBuffer,
                                                VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkImage destImage, VkImageLayout destImageLayout,
                                                uint32_t regionCount, const VkImageResolve* pRegions)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdSetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdResetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t eventCount, const VkEvent* pEvents, uint32_t memBarrierCount, const void** ppMemBarriers)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdWaitEvents(cmdBuffer, waitEvent, eventCount, pEvents, memBarrierCount, ppMemBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t pipeEventCount, const VkPipeEvent* pPipeEvents, uint32_t memBarrierCount, const void** ppMemBarriers)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdPipelineBarrier(cmdBuffer, waitEvent, pipeEventCount, pPipeEvents, memBarrierCount, ppMemBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCmdBuffer cmdBuffer, VkTimestampType timestampType, VkBuffer destBuffer, VkDeviceSize destOffset)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdInitAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdLoadAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer srcBuffer, VkDeviceSize srcOffset)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdSaveAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer destBuffer, VkDeviceSize destOffset)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VkFramebufferCreateInfo* localFBCI = new VkFramebufferCreateInfo(*pCreateInfo);
        if (pCreateInfo->pColorAttachments) {
            localFBCI->pColorAttachments = new VkColorAttachmentBindInfo[localFBCI->colorAttachmentCount];
            memcpy((void*)localFBCI->pColorAttachments, pCreateInfo->pColorAttachments, localFBCI->colorAttachmentCount*sizeof(VkColorAttachmentBindInfo));
        }
        if (pCreateInfo->pDepthStencilAttachment) {
            localFBCI->pDepthStencilAttachment = new VkDepthStencilBindInfo[localFBCI->colorAttachmentCount];
            memcpy((void*)localFBCI->pDepthStencilAttachment, pCreateInfo->pDepthStencilAttachment, localFBCI->colorAttachmentCount*sizeof(VkDepthStencilBindInfo));
        }
        frameBufferMap[*pFramebuffer] = localFBCI;
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult result = pTable->CreateRenderPass(device, pCreateInfo, pRenderPass);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VkRenderPassCreateInfo* localRPCI = new VkRenderPassCreateInfo(*pCreateInfo);
        if (pCreateInfo->pColorLoadOps) {
            localRPCI->pColorLoadOps = new VkAttachmentLoadOp[localRPCI->colorAttachmentCount];
            memcpy((void*)localRPCI->pColorLoadOps, pCreateInfo->pColorLoadOps, localRPCI->colorAttachmentCount*sizeof(VkAttachmentLoadOp));
        }
        if (pCreateInfo->pColorStoreOps) {
            localRPCI->pColorStoreOps = new VkAttachmentStoreOp[localRPCI->colorAttachmentCount];
            memcpy((void*)localRPCI->pColorStoreOps, pCreateInfo->pColorStoreOps, localRPCI->colorAttachmentCount*sizeof(VkAttachmentStoreOp));
        }
        if (pCreateInfo->pColorLoadClearValues) {
            localRPCI->pColorLoadClearValues = new VkClearColor[localRPCI->colorAttachmentCount];
            memcpy((void*)localRPCI->pColorLoadClearValues, pCreateInfo->pColorLoadClearValues, localRPCI->colorAttachmentCount*sizeof(VkClearColor));
        }
        renderPassMap[*pRenderPass] = localRPCI;
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCmdBuffer cmdBuffer, const VkRenderPassBegin *pRenderPassBegin)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VkCmdBuffer cmdBuffer, VkRenderPass renderPass)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdEndRenderPass(cmdBuffer, renderPass);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgRegisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    VK_LAYER_DBG_FUNCTION_NODE* pNewDbgFuncNode = new VK_LAYER_DBG_FUNCTION_NODE;
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    // force callbacks if DebugAction hasn't been set already other than initial value
    if (g_actionIsDefault) {
        g_debugAction = VK_DBG_LAYER_ACTION_CALLBACK;
    }

    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable* pInstTable = tableInstanceMap[*ppDisp];
    VkResult result = pInstTable->DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgUnregisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
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

    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable* pInstTable = tableInstanceMap[*ppDisp];
    VkResult result = pInstTable->DbgUnregisterMsgCallback(instance, pfnMsgCallback);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VkCmdBuffer cmdBuffer, const char* pMarker)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VkCmdBuffer cmdBuffer)
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
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    pTable->CmdDbgMarkerEnd(cmdBuffer);
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
        int retval = system(dotCmd);
        assert(retval != -1);
        remove("/tmp/tmp.dot");
    }
    else {
        char str[1024];
        sprintf(str, "Cannot execute dot program at (%s) to dump requested %s file.", dotExe, outFileName);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, DRAWSTATE_MISSING_DOT_PROGRAM, "DS", str);
    }
#endif // WIN32
}

VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(VkDevice dev, const char* funcName)
{
    VkBaseLayerObject* devw = (VkBaseLayerObject *) dev;

    if (dev == NULL)
        return NULL;

    loader_platform_thread_once(&g_initOnce, initDrawState);
    initDeviceTable((const VkBaseLayerObject *) dev);

    if (!strcmp(funcName, "vkGetDeviceProcAddr"))
        return (void *) vkGetDeviceProcAddr;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (void*) vkDestroyDevice;
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
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (void*) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (void*) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (void*) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocDescriptorSets"))
        return (void*) vkAllocDescriptorSets;
    if (!strcmp(funcName, "vkClearDescriptorSets"))
        return (void*) vkClearDescriptorSets;
    if (!strcmp(funcName, "vkUpdateDescriptorSets"))
        return (void*) vkUpdateDescriptorSets;
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
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (void*) vkCmdBindVertexBuffers;
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
        if (devw->pGPA == NULL)
            return NULL;
        return devw->pGPA((VkObject)devw->nextObject, funcName);
    }
}

VK_LAYER_EXPORT void * VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    VkBaseLayerObject* instw = (VkBaseLayerObject *) instance;
    if (instance == NULL)
        return NULL;

    loader_platform_thread_once(&g_initOnce, initDrawState);
    initInstanceTable((const VkBaseLayerObject *) instance);

    if (!strcmp(funcName, "vkGetInstanceProcAddr"))
        return (void *) vkGetInstanceProcAddr;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (void *) vkDestroyInstance;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(funcName, "vkEnumerateLayers"))
        return (void*) vkEnumerateLayers;
    if (!strcmp(funcName, "vkDbgRegisterMsgCallback"))
        return (void*) vkDbgRegisterMsgCallback;
    if (!strcmp(funcName, "vkDbgUnregisterMsgCallback"))
        return (void*) vkDbgUnregisterMsgCallback;
    else {
        if (instw->pGPA == NULL)
            return NULL;
        return instw->pGPA((VkObject) instw->nextObject, funcName);
    }
}
