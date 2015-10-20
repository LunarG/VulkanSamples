/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 * Copyright (C) 2015 Google, Inc.
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
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "vk_struct_size_helper.h"
#include "draw_state.h"
#include "vk_layer_config.h"
#include "vk_debug_marker_layer.h"
#include "vk_layer_table.h"
#include "vk_layer_debug_marker_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"

struct devExts {
    bool debug_marker_enabled;
};

struct layer_data {
    debug_report_data *report_data;
    // TODO: put instance data here
    std::vector<VkDbgMsgCallback> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    devExts device_extensions;
    // Layer specific data
    unordered_map<uint64_t, unique_ptr<SAMPLER_NODE>> sampleMap;
    unordered_map<uint64_t, unique_ptr<VkImageViewCreateInfo>> imageViewMap;
    unordered_map<uint64_t, unique_ptr<BUFFER_NODE>> bufferViewMap;
    unordered_map<uint64_t, PIPELINE_NODE*> pipelineMap;
    unordered_map<uint64_t, POOL_NODE*> poolMap;
    unordered_map<uint64_t, SET_NODE*> setMap;
    unordered_map<uint64_t, LAYOUT_NODE*> layoutMap;
    unordered_map<uint64_t, PIPELINE_LAYOUT_NODE> pipelineLayoutMap;
    // Map for layout chains
    unordered_map<void*, GLOBAL_CB_NODE*> cmdBufferMap;
    unordered_map<uint64_t, VkRenderPassCreateInfo*> renderPassMap;
    unordered_map<uint64_t, VkFramebufferCreateInfo*> frameBufferMap;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr),
        device_extensions()
    {};
};
// TODO : Do we need to guard access to layer_data_map w/ lock?
static std::unordered_map<void *, layer_data *> layer_data_map;

static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;
#define MAX_TID 513
static loader_platform_thread_id g_tidMapping[MAX_TID] = {0};
static uint32_t g_maxTID = 0;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

debug_report_data *mdd(void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    return my_data->report_data;
}

debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    return my_data->report_data;
}
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
        case CMD_SETVIEWPORTSTATE:
            return "CMD_SETVIEWPORTSTATE";
        case CMD_SETLINEWIDTHSTATE:
            return "CMD_SETLINEWIDTHSTATE";
        case CMD_SETDEPTHBIASSTATE:
            return "CMD_SETDEPTHBIASSTATE";
        case CMD_SETBLENDSTATE:
            return "CMD_SETBLENDSTATE";
        case CMD_SETDEPTHBOUNDSSTATE:
            return "CMD_SETDEPTHBOUNDSSTATE";
        case CMD_SETSTENCILREADMASKSTATE:
            return "CMD_SETSTENCILREADMASKSTATE";
        case CMD_SETSTENCILWRITEMASKSTATE:
            return "CMD_SETSTENCILWRITEMASKSTATE";
        case CMD_SETSTENCILREFERENCESTATE:
            return "CMD_SETSTENCILREFERENCESTATE";
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
        case CMD_CLEARCOLORATTACHMENT:
            return "CMD_CLEARCOLORATTACHMENT";
        case CMD_CLEARDEPTHSTENCILIMAGE:
            return "CMD_CLEARDEPTHSTENCILIMAGE";
        case CMD_CLEARDEPTHSTENCILATTACHMENT:
            return "CMD_CLEARDEPTHSTENCILATTACHMENT";
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
        case CMD_COPYQUERYPOOLRESULTS:
            return "CMD_COPYQUERYPOOLRESULTS";
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
static GLOBAL_CB_NODE* g_pLastTouchedCB[NUM_COMMAND_BUFFERS_TO_DISPLAY] = {NULL};
static uint32_t        g_lastTouchedCBIndex = 0;
// Track the last global DrawState of interest touched by any thread
static GLOBAL_CB_NODE* g_lastGlobalCB = NULL;
static PIPELINE_NODE*  g_lastBoundPipeline = NULL;
static VkDescriptorSet g_lastBoundDescriptorSet = VK_NULL_HANDLE;
#define MAX_BINDING 0xFFFFFFFF // Default vtxBinding value in CB Node to identify if no vtxBinding set
// prototype
static GLOBAL_CB_NODE* getCBNode(layer_data*, const VkCmdBuffer);
// Update global ptrs to reflect that specified cmdBuffer has been used
static void updateCBTracking(GLOBAL_CB_NODE* pCB)
{
    g_lastCmdBuffer[getTIDIndex()] = pCB->cmdBuffer;
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
static VkBool32 hasDrawCmd(GLOBAL_CB_NODE* pCB)
{
    for (uint32_t i=0; i<NUM_DRAW_TYPES; i++) {
        if (pCB->drawCount[i])
            return VK_TRUE;
    }
    return VK_FALSE;
}
// Check object status for selected flag state
static VkBool32 validate_status(GLOBAL_CB_NODE* pNode, CBStatusFlags enable_mask, CBStatusFlags status_mask, CBStatusFlags status_flag, VkFlags msg_flags, DRAW_STATE_ERROR error_code, const char* fail_msg)
{
    // If non-zero enable mask is present, check it against status but if enable_mask
    //  is 0 then no enable required so we should always just check status
    if ((!enable_mask) || (enable_mask & pNode->status)) {
        if ((pNode->status & status_mask) != status_flag) {
            // TODO : How to pass dispatchable objects as srcObject? Here src obj should be cmd buffer
            return log_msg(mdd(pNode->cmdBuffer), msg_flags, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, error_code, "DS",
                    "CB object %#" PRIxLEAST64 ": %s", reinterpret_cast<uint64_t>(pNode->cmdBuffer), fail_msg);
        }
    }
    return VK_FALSE;
}
// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_NODE* getPipeline(layer_data* my_data, const VkPipeline pipeline)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->pipelineMap.find(pipeline.handle) == my_data->pipelineMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->pipelineMap[pipeline.handle];
}
// Return VK_TRUE if for a given PSO, the given state enum is dynamic, else return VK_FALSE
static VkBool32 isDynamic(const PIPELINE_NODE* pPipeline, const VkDynamicState state)
{
    if (pPipeline && pPipeline->graphicsPipelineCI.pDynamicState) {
        for (uint32_t i=0; i<pPipeline->graphicsPipelineCI.pDynamicState->dynamicStateCount; i++) {
            if (state == pPipeline->graphicsPipelineCI.pDynamicState->pDynamicStates[i])
                return VK_TRUE;
        }
    }
    return VK_FALSE;
}
// Validate state stored as flags at time of draw call
static VkBool32 validate_draw_state_flags(GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    VkBool32 result;
    result = validate_status(pCB, CBSTATUS_NONE, CBSTATUS_VIEWPORT_SET, CBSTATUS_VIEWPORT_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_VIEWPORT_NOT_BOUND, "Dynamic viewport state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_NONE, CBSTATUS_SCISSOR_SET, CBSTATUS_SCISSOR_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic scissor state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_NONE, CBSTATUS_LINE_WIDTH_SET, CBSTATUS_LINE_WIDTH_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_LINE_WIDTH_NOT_BOUND, "Dynamic line width state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_NONE, CBSTATUS_DEPTH_BIAS_SET, CBSTATUS_DEPTH_BIAS_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_DEPTH_BIAS_NOT_BOUND, "Dynamic depth bias state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_COLOR_BLEND_WRITE_ENABLE, CBSTATUS_BLEND_SET, CBSTATUS_BLEND_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_BLEND_NOT_BOUND, "Dynamic blend object state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_DEPTH_WRITE_ENABLE, CBSTATUS_DEPTH_BOUNDS_SET, CBSTATUS_DEPTH_BOUNDS_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND, "Dynamic depth bounds state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_READ_MASK_SET, CBSTATUS_STENCIL_READ_MASK_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil read mask state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_WRITE_MASK_SET, CBSTATUS_STENCIL_WRITE_MASK_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil write mask state not set for this command buffer");
    result |= validate_status(pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_REFERENCE_SET, CBSTATUS_STENCIL_REFERENCE_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil reference state not set for this command buffer");
    if (indexedDraw)
        result |= validate_status(pCB, CBSTATUS_NONE, CBSTATUS_INDEX_BUFFER_BOUND, CBSTATUS_INDEX_BUFFER_BOUND, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_INDEX_BUFFER_NOT_BOUND, "Index buffer object not bound to this command buffer when Indexed Draw attempted");
    return result;
}
// Validate overall state at the time of a draw call
static VkBool32 validate_draw_state(layer_data* my_data, GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    // First check flag states
    VkBool32 result = validate_draw_state_flags(pCB, indexedDraw);
    PIPELINE_NODE* pPipe = getPipeline(my_data, pCB->lastBoundPipeline);
    // Now complete other state checks
    // TODO : Currently only performing next check if *something* was bound (non-zero last bound)
    //  There is probably a better way to gate when this check happens, and to know if something *should* have been bound
    //  We should have that check separately and then gate this check based on that check
    if (pPipe && (pCB->lastBoundPipelineLayout) && (pCB->lastBoundPipelineLayout != pPipe->graphicsPipelineCI.layout)) {
        result = VK_FALSE;
        result |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE_LAYOUT, pCB->lastBoundPipelineLayout.handle, 0, DRAWSTATE_PIPELINE_LAYOUT_MISMATCH, "DS",
                "Pipeline layout from last vkCmdBindDescriptorSets() (%#" PRIxLEAST64 ") does not match PSO Pipeline layout (%#" PRIxLEAST64 ") ", pCB->lastBoundPipelineLayout.handle, pPipe->graphicsPipelineCI.layout.handle);
    }
    // Verify Vtx binding
    if (MAX_BINDING != pCB->lastVtxBinding) {
        if (pCB->lastVtxBinding >= pPipe->vtxBindingCount) {
            if (0 == pPipe->vtxBindingCount) {
                result |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                        "Vtx Buffer Index %u was bound, but no vtx buffers are attached to PSO.", pCB->lastVtxBinding);
            }
            else {
                result |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                        "Vtx binding Index of %u exceeds PSO pVertexBindingDescriptions max array index of %u.", pCB->lastVtxBinding, (pPipe->vtxBindingCount - 1));
            }
        }
    }
    // If Viewport or scissors are dynamic, verify that dynamic count matches PSO count
    VkBool32 dynViewport = isDynamic(pPipe, VK_DYNAMIC_STATE_VIEWPORT);
    VkBool32 dynScissor = isDynamic(pPipe, VK_DYNAMIC_STATE_SCISSOR);
    if (dynViewport) {
        if (pCB->viewports.size() != pPipe->graphicsPipelineCI.pViewportState->viewportCount) {
            result |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Dynamic viewportCount from vkCmdSetViewport() is %u, but PSO viewportCount is %u. These counts must match.", pCB->viewports.size(), pPipe->graphicsPipelineCI.pViewportState->viewportCount);
        }
    }
    if (dynScissor) {
        if (pCB->scissors.size() != pPipe->graphicsPipelineCI.pViewportState->scissorCount) {
            result |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Dynamic scissorCount from vkCmdSetScissor() is %u, but PSO scissorCount is %u. These counts must match.", pCB->scissors.size(), pPipe->graphicsPipelineCI.pViewportState->scissorCount);
        }
    }
    return result;
}
// Verify that create state for a pipeline is valid
static VkBool32 verifyPipelineCreateState(const VkDevice device, const PIPELINE_NODE* pPipeline)
{
    VkBool32 skipCall = VK_FALSE;
    // VS is required
    if (!(pPipeline->active_shaders & VK_SHADER_STAGE_VERTEX_BIT)) {
        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Vtx Shader required");
    }
    // Either both or neither TC/TE shaders should be defined
    if (((pPipeline->active_shaders & VK_SHADER_STAGE_TESS_CONTROL_BIT) == 0) !=
         ((pPipeline->active_shaders & VK_SHADER_STAGE_TESS_EVALUATION_BIT) == 0) ) {
        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: TE and TC shaders must be included or excluded as a pair");
    }
    // Compute shaders should be specified independent of Gfx shaders
    if ((pPipeline->active_shaders & VK_SHADER_STAGE_COMPUTE_BIT) &&
        (pPipeline->active_shaders & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESS_CONTROL_BIT |
                                     VK_SHADER_STAGE_TESS_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT |
                                     VK_SHADER_STAGE_FRAGMENT_BIT))) {
        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Do not specify Compute Shader for Gfx Pipeline");
    }
    // VK_PRIMITIVE_TOPOLOGY_PATCH primitive topology is only valid for tessellation pipelines.
    // Mismatching primitive topology and tessellation fails graphics pipeline creation.
    if (pPipeline->active_shaders & (VK_SHADER_STAGE_TESS_CONTROL_BIT | VK_SHADER_STAGE_TESS_EVALUATION_BIT) &&
        (pPipeline->iaStateCI.topology != VK_PRIMITIVE_TOPOLOGY_PATCH)) {
        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH must be set as IA topology for tessellation pipelines");
    }
    if (pPipeline->iaStateCI.topology == VK_PRIMITIVE_TOPOLOGY_PATCH) {
        if (~pPipeline->active_shaders & VK_SHADER_STAGE_TESS_CONTROL_BIT) {
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH primitive topology is only valid for tessellation pipelines");
        }
        if (!pPipeline->tessStateCI.patchControlPoints || (pPipeline->tessStateCI.patchControlPoints > 32)) {
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                    "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH primitive topology used with patchControlPoints value %u."
                    " patchControlPoints should be >0 and <=32.", pPipeline->tessStateCI.patchControlPoints);
        }
    }
    // Viewport state must be included and viewport and scissor counts should always match
    // NOTE : Even if these are flagged as dynamic, counts need to be set correctly for shader compiler
    if (!pPipeline->graphicsPipelineCI.pViewportState) {
        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
            "Gfx Pipeline pViewportState is null. Even if viewport and scissors are dynamic PSO must include viewportCount and scissorCount in pViewportState.");
    } else if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount != pPipeline->graphicsPipelineCI.pViewportState->viewportCount) {
        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Gfx Pipeline viewport count (%u) must match scissor count (%u).", pPipeline->vpStateCI.viewportCount, pPipeline->vpStateCI.scissorCount);
    } else {
        // If viewport or scissor are not dynamic, then verify that data is appropriate for count
        VkBool32 dynViewport = isDynamic(pPipeline, VK_DYNAMIC_STATE_VIEWPORT);
        VkBool32 dynScissor = isDynamic(pPipeline, VK_DYNAMIC_STATE_SCISSOR);
        if (!dynViewport) {
            if (pPipeline->graphicsPipelineCI.pViewportState->viewportCount && !pPipeline->graphicsPipelineCI.pViewportState->pViewports) {
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline viewportCount is %u, but pViewports is NULL. For non-zero viewportCount, you must either include pViewports data, or include viewport in pDynamicState and set it with vkCmdSetViewport().", pPipeline->graphicsPipelineCI.pViewportState->viewportCount);
            }
        }
        if (!dynScissor) {
            if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount && !pPipeline->graphicsPipelineCI.pViewportState->pScissors) {
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline scissorCount is %u, but pScissors is NULL. For non-zero scissorCount, you must either include pScissors data, or include scissor in pDynamicState and set it with vkCmdSetScissor().", pPipeline->graphicsPipelineCI.pViewportState->scissorCount);
            }
        }
    }
    return skipCall;
}
// Init the pipeline mapping info based on pipeline create info LL tree
//  Threading note : Calls to this function should wrapped in mutex
static PIPELINE_NODE* initPipeline(const VkGraphicsPipelineCreateInfo* pCreateInfo, PIPELINE_NODE* pBasePipeline)
{
    PIPELINE_NODE* pPipeline = new PIPELINE_NODE;
    if (pBasePipeline) {
        memcpy((void*)pPipeline, (void*)pBasePipeline, sizeof(PIPELINE_NODE));
    } else {
        memset((void*)pPipeline, 0, sizeof(PIPELINE_NODE));
    }
    // First init create info
    memcpy(&pPipeline->graphicsPipelineCI, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo));

    size_t bufferSize = 0;
    const VkPipelineVertexInputStateCreateInfo* pVICI = NULL;
    const VkPipelineColorBlendStateCreateInfo*  pCBCI = NULL;

    for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
        const VkPipelineShaderStageCreateInfo *pPSSCI = &pCreateInfo->pStages[i];

        switch (pPSSCI->stage) {
            case VK_SHADER_STAGE_VERTEX:
                memcpy(&pPipeline->vsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case VK_SHADER_STAGE_TESS_CONTROL:
                memcpy(&pPipeline->tcsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESS_CONTROL_BIT;
                break;
            case VK_SHADER_STAGE_TESS_EVALUATION:
                memcpy(&pPipeline->tesCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESS_EVALUATION_BIT;
                break;
            case VK_SHADER_STAGE_GEOMETRY:
                memcpy(&pPipeline->gsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case VK_SHADER_STAGE_FRAGMENT:
                memcpy(&pPipeline->fsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case VK_SHADER_STAGE_COMPUTE:
                // TODO : Flag error, CS is specified through VkComputePipelineCreateInfo
                pPipeline->active_shaders |= VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                // TODO : Flag error
                break;
        }
    }
    // Copy over GraphicsPipelineCreateInfo structure embedded pointers
    if (pCreateInfo->stageCount != 0) {
        pPipeline->graphicsPipelineCI.pStages = new VkPipelineShaderStageCreateInfo[pCreateInfo->stageCount];
        bufferSize =  pCreateInfo->stageCount * sizeof(VkPipelineShaderStageCreateInfo);
        memcpy((void*)pPipeline->graphicsPipelineCI.pStages, pCreateInfo->pStages, bufferSize);
    }
    if (pCreateInfo->pVertexInputState != NULL) {
        memcpy((void*)&pPipeline->vertexInputCI, pCreateInfo->pVertexInputState , sizeof(VkPipelineVertexInputStateCreateInfo));
        // Copy embedded ptrs
        pVICI = pCreateInfo->pVertexInputState;
        pPipeline->vtxBindingCount = pVICI->bindingCount;
        if (pPipeline->vtxBindingCount) {
            pPipeline->pVertexBindingDescriptions = new VkVertexInputBindingDescription[pPipeline->vtxBindingCount];
            bufferSize = pPipeline->vtxBindingCount * sizeof(VkVertexInputBindingDescription);
            memcpy((void*)pPipeline->pVertexBindingDescriptions, pVICI->pVertexBindingDescriptions, bufferSize);
        }
        pPipeline->vtxAttributeCount = pVICI->attributeCount;
        if (pPipeline->vtxAttributeCount) {
            pPipeline->pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[pPipeline->vtxAttributeCount];
            bufferSize = pPipeline->vtxAttributeCount * sizeof(VkVertexInputAttributeDescription);
            memcpy((void*)pPipeline->pVertexAttributeDescriptions, pVICI->pVertexAttributeDescriptions, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pVertexInputState = &pPipeline->vertexInputCI;
    }
    if (pCreateInfo->pInputAssemblyState != NULL) {
        memcpy((void*)&pPipeline->iaStateCI, pCreateInfo->pInputAssemblyState, sizeof(VkPipelineInputAssemblyStateCreateInfo));
        pPipeline->graphicsPipelineCI.pInputAssemblyState = &pPipeline->iaStateCI;
    }
    if (pCreateInfo->pTessellationState != NULL) {
        memcpy((void*)&pPipeline->tessStateCI, pCreateInfo->pTessellationState, sizeof(VkPipelineTessellationStateCreateInfo));
        pPipeline->graphicsPipelineCI.pTessellationState = &pPipeline->tessStateCI;
    }
    if (pCreateInfo->pViewportState != NULL) {
        memcpy((void*)&pPipeline->vpStateCI, pCreateInfo->pViewportState, sizeof(VkPipelineViewportStateCreateInfo));
        pPipeline->graphicsPipelineCI.pViewportState = &pPipeline->vpStateCI;
    }
    if (pCreateInfo->pRasterState != NULL) {
        memcpy((void*)&pPipeline->rsStateCI, pCreateInfo->pRasterState, sizeof(VkPipelineRasterStateCreateInfo));
        pPipeline->graphicsPipelineCI.pRasterState = &pPipeline->rsStateCI;
    }
    if (pCreateInfo->pMultisampleState != NULL) {
        memcpy((void*)&pPipeline->msStateCI, pCreateInfo->pMultisampleState, sizeof(VkPipelineMultisampleStateCreateInfo));
        pPipeline->graphicsPipelineCI.pMultisampleState = &pPipeline->msStateCI;
    }
    if (pCreateInfo->pDepthStencilState != NULL) {
        memcpy((void*)&pPipeline->dsStateCI, pCreateInfo->pDepthStencilState, sizeof(VkPipelineDepthStencilStateCreateInfo));
        pPipeline->graphicsPipelineCI.pDepthStencilState = &pPipeline->dsStateCI;
    }
    if (pCreateInfo->pColorBlendState != NULL) {
        memcpy((void*)&pPipeline->cbStateCI, pCreateInfo->pColorBlendState, sizeof(VkPipelineColorBlendStateCreateInfo));
        // Copy embedded ptrs
        pCBCI = pCreateInfo->pColorBlendState;
        pPipeline->attachmentCount = pCBCI->attachmentCount;
        if (pPipeline->attachmentCount) {
            pPipeline->pAttachments = new VkPipelineColorBlendAttachmentState[pPipeline->attachmentCount];
            bufferSize = pPipeline->attachmentCount * sizeof(VkPipelineColorBlendAttachmentState);
            memcpy((void*)pPipeline->pAttachments, pCBCI->pAttachments, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pColorBlendState = &pPipeline->cbStateCI;
    }
    if (pCreateInfo->pDynamicState != NULL) {
        memcpy((void*)&pPipeline->dynStateCI, pCreateInfo->pDynamicState, sizeof(VkPipelineDynamicStateCreateInfo));
        if (pPipeline->dynStateCI.dynamicStateCount) {
            pPipeline->dynStateCI.pDynamicStates = new VkDynamicState[pPipeline->dynStateCI.dynamicStateCount];
            bufferSize = pPipeline->dynStateCI.dynamicStateCount * sizeof(VkDynamicState);
            memcpy((void*)pPipeline->dynStateCI.pDynamicStates, pCreateInfo->pDynamicState->pDynamicStates, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pDynamicState = &pPipeline->dynStateCI;
    }

    return pPipeline;
}
// Free the Pipeline nodes
static void deletePipelines(layer_data* my_data)
{
    if (my_data->pipelineMap.size() <= 0)
        return;
    for (auto ii=my_data->pipelineMap.begin(); ii!=my_data->pipelineMap.end(); ++ii) {
        if ((*ii).second->graphicsPipelineCI.stageCount != 0) {
            delete[] (*ii).second->graphicsPipelineCI.pStages;
        }
        if ((*ii).second->pVertexBindingDescriptions) {
            delete[] (*ii).second->pVertexBindingDescriptions;
        }
        if ((*ii).second->pVertexAttributeDescriptions) {
            delete[] (*ii).second->pVertexAttributeDescriptions;
        }
        if ((*ii).second->pAttachments) {
            delete[] (*ii).second->pAttachments;
        }
        if ((*ii).second->dynStateCI.dynamicStateCount != 0) {
            delete[] (*ii).second->dynStateCI.pDynamicStates;
        }
        delete (*ii).second;
    }
    my_data->pipelineMap.clear();
}
// For given pipeline, return number of MSAA samples, or one if MSAA disabled
static uint32_t getNumSamples(layer_data* my_data, const VkPipeline pipeline)
{
    PIPELINE_NODE* pPipe = my_data->pipelineMap[pipeline.handle];
    if (VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO == pPipe->msStateCI.sType) {
        return pPipe->msStateCI.rasterSamples;
    }
    return 1;
}
// Validate state related to the PSO
static VkBool32 validatePipelineState(layer_data* my_data, const GLOBAL_CB_NODE* pCB, const VkPipelineBindPoint pipelineBindPoint, const VkPipeline pipeline)
{
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
        // Verify that any MSAA request in PSO matches sample# in bound FB
        uint32_t psoNumSamples = getNumSamples(my_data, pipeline);
        if (pCB->activeRenderPass) {
            const VkRenderPassCreateInfo* pRPCI = my_data->renderPassMap[pCB->activeRenderPass.handle];
            const VkSubpassDescription* pSD = &pRPCI->pSubpasses[pCB->activeSubpass];
            int subpassNumSamples = 0;
            uint32_t i;

            for (i = 0; i < pSD->colorCount; i++) {
                uint32_t samples;

                if (pSD->pColorAttachments[i].attachment == VK_ATTACHMENT_UNUSED)
                    continue;

                samples = pRPCI->pAttachments[pSD->pColorAttachments[i].attachment].samples;
                if (subpassNumSamples == 0) {
                    subpassNumSamples = samples;
                } else if (subpassNumSamples != samples) {
                    subpassNumSamples = -1;
                    break;
                }
            }
            if (pSD->depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED) {
                const uint32_t samples = pRPCI->pAttachments[pSD->depthStencilAttachment.attachment].samples;
                if (subpassNumSamples == 0)
                    subpassNumSamples = samples;
                else if (subpassNumSamples != samples)
                    subpassNumSamples = -1;
            }

            if (psoNumSamples != subpassNumSamples) {
                return log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, pipeline.handle, 0, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                        "Num samples mismatch! Binding PSO (%#" PRIxLEAST64 ") with %u samples while current RenderPass (%#" PRIxLEAST64 ") w/ %u samples!",
                        pipeline.handle, psoNumSamples, pCB->activeRenderPass.handle, subpassNumSamples);
            }
        } else {
            // TODO : I believe it's an error if we reach this point and don't have an activeRenderPass
            //   Verify and flag error as appropriate
        }
        // TODO : Add more checks here
    } else {
        // TODO : Validate non-gfx pipeline updates
    }
    return VK_FALSE;
}

// Block of code at start here specifically for managing/tracking DSs

// Return Pool node ptr for specified pool or else NULL
static POOL_NODE* getPoolNode(layer_data* my_data, const VkDescriptorPool pool)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->poolMap.find(pool.handle) == my_data->poolMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->poolMap[pool.handle];
}
// Return Set node ptr for specified set or else NULL
static SET_NODE* getSetNode(layer_data* my_data, const VkDescriptorSet set)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->setMap.find(set.handle) == my_data->setMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->setMap[set.handle];
}

static LAYOUT_NODE* getLayoutNode(layer_data* my_data, const VkDescriptorSetLayout layout) {
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->layoutMap.find(layout.handle) == my_data->layoutMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->layoutMap[layout.handle];
}
// Return VK_FALSE if update struct is of valid type, otherwise flag error and return code from callback
static VkBool32 validUpdateStruct(const VkDevice device, const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            return VK_FALSE;
        default:
            return log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
}
// For given update struct, return binding
static VkBool32 getUpdateBinding(const VkDevice device, const GENERIC_HEADER* pUpdateStruct, uint32_t* binding)
{
    VkBool32 skipCall = VK_FALSE;
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            *binding = ((VkWriteDescriptorSet*)pUpdateStruct)->destBinding;
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            *binding = ((VkCopyDescriptorSet*)pUpdateStruct)->destBinding;
            break;
        default:
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            *binding = 0xFFFFFFFF;
    }
    return skipCall;
}
// Set arrayIndex for given update struct in the last parameter
// Return value of skipCall, which is only VK_TRUE is error occurs and callback signals execution to cease
static uint32_t getUpdateArrayIndex(const VkDevice device, const GENERIC_HEADER* pUpdateStruct, uint32_t* arrayIndex)
{
    VkBool32 skipCall = VK_FALSE;
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            *arrayIndex = ((VkWriteDescriptorSet*)pUpdateStruct)->destArrayElement;
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            // TODO : Need to understand this case better and make sure code is correct
            *arrayIndex = ((VkCopyDescriptorSet*)pUpdateStruct)->destArrayElement;
            break;
        default:
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            *arrayIndex = 0;
    }
    return skipCall;
}
// Set count for given update struct in the last parameter
// Return value of skipCall, which is only VK_TRUE is error occurs and callback signals execution to cease
static uint32_t getUpdateCount(const VkDevice device, const GENERIC_HEADER* pUpdateStruct, uint32_t* count)
{
    VkBool32 skipCall = VK_FALSE;
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            *count = ((VkWriteDescriptorSet*)pUpdateStruct)->count;
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            // TODO : Need to understand this case better and make sure code is correct
            *count = ((VkCopyDescriptorSet*)pUpdateStruct)->count;
            break;
        default:
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
            *count = 0;
    }
    return skipCall;
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
static VkBool32 getUpdateStartIndex(const VkDevice device, const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct, uint32_t* startIndex)
{
    uint32_t binding = 0, arrayIndex = 0;
    VkBool32 skipCall = getUpdateBinding(device, pUpdateStruct, &binding);
    skipCall |= getUpdateArrayIndex(device, pUpdateStruct, &arrayIndex);
    if (VK_FALSE == skipCall)
        *startIndex = getBindingStartIndex(pLayout, binding)+arrayIndex;
    return skipCall;
}
// For given layout and update, return the last overall index of the layout that is update
static VkBool32 getUpdateEndIndex(const VkDevice device, const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct, uint32_t* endIndex)
{
    uint32_t binding = 0, arrayIndex = 0, count = 0;
    VkBool32 skipCall = getUpdateBinding(device, pUpdateStruct, &binding);
    skipCall |= getUpdateArrayIndex(device, pUpdateStruct, &arrayIndex);
    skipCall |= getUpdateCount(device, pUpdateStruct, &count);
    if (VK_FALSE == skipCall)
        *endIndex = getBindingStartIndex(pLayout, binding)+arrayIndex+count-1;
    return skipCall;
}
// Verify that the descriptor type in the update struct matches what's expected by the layout
static VkBool32 validateUpdateType(const VkDevice device, const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct)
{
    // First get actual type of update
    VkBool32 skipCall = VK_FALSE;
    VkDescriptorType actualType;
    uint32_t i = 0, startIndex = 0, endIndex = 0;
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            actualType = ((VkWriteDescriptorSet*)pUpdateStruct)->descriptorType;
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            /* no need to validate */
            return VK_FALSE;
            break;
        default:
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
    skipCall |= getUpdateStartIndex(device, pLayout, pUpdateStruct, &startIndex);
    skipCall |= getUpdateEndIndex(device, pLayout, pUpdateStruct, &endIndex);
    if (VK_FALSE == skipCall) {
        for (i = startIndex; i <= endIndex; i++) {
            if (pLayout->descriptorTypes[i] != actualType) {
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                        "Descriptor update type of %s has descriptor type %s that does not match overlapping binding descriptor type of %s!",
                        string_VkStructureType(pUpdateStruct->sType), string_VkDescriptorType(actualType), string_VkDescriptorType(pLayout->descriptorTypes[i]));
            }
        }
    }
    return skipCall;
}
// Determine the update type, allocate a new struct of that type, shadow the given pUpdate
//   struct into the pNewNode param. Return VK_TRUE if error condition encountered and callback signals early exit.
// NOTE : Calls to this function should be wrapped in mutex
static VkBool32 shadowUpdateNode(const VkDevice device, GENERIC_HEADER* pUpdate, GENERIC_HEADER** pNewNode)
{
    VkBool32 skipCall = VK_FALSE;
    VkWriteDescriptorSet* pWDS = NULL;
    VkCopyDescriptorSet* pCDS = NULL;
    size_t array_size = 0;
    size_t base_array_size = 0;
    size_t total_array_size = 0;
    size_t baseBuffAddr = 0;
    switch (pUpdate->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            pWDS = new VkWriteDescriptorSet;
            *pNewNode = (GENERIC_HEADER*)pWDS;
            memcpy(pWDS, pUpdate, sizeof(VkWriteDescriptorSet));
            pWDS->pDescriptors = new VkDescriptorInfo[pWDS->count];
            array_size = sizeof(VkDescriptorInfo) * pWDS->count;
            memcpy((void*)pWDS->pDescriptors, ((VkWriteDescriptorSet*)pUpdate)->pDescriptors, array_size);
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            pCDS = new VkCopyDescriptorSet;
            *pNewNode = (GENERIC_HEADER*)pCDS;
            memcpy(pCDS, pUpdate, sizeof(VkCopyDescriptorSet));
            break;
        default:
            if (log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdate->sType), pUpdate->sType))
                return VK_TRUE;
    }
    // Make sure that pNext for the end of shadow copy is NULL
    (*pNewNode)->pNext = NULL;
    return skipCall;
}

static VkBool32 validateDescriptorSetImageView(VkDevice device, uint32_t writeDsCount, const VkWriteDescriptorSet *pWDS)
{
    VkBool32    skipCall = VK_FALSE;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Check ImageAspects of each descriptorSet in each writeDescriptorSet array
    for (uint32_t i = 0; i < writeDsCount; i++) {
        for (uint32_t j = 0; j < pWDS[i].count; j++) {
            const VkDescriptorInfo *dInfo = &pWDS[i].pDescriptors[j];
            auto imageViewItem = dev_data->imageViewMap.find(dInfo->imageView.handle);
            if (imageViewItem != dev_data->imageViewMap.end()) {
                VkImageAspectFlags flags = ((*imageViewItem).second)->subresourceRange.aspectMask;
                if ((flags & VK_IMAGE_ASPECT_DEPTH_BIT) &&
                    (flags & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                    skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, dInfo->imageView.handle, 0,
                        DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: DesriptorSet[%d] in WriteDesriptorSet[%d] "
                        "has ImageView with both STENCIL and DEPTH aspects set", j, i);
                }
            }
        }
    }
    return skipCall;
}

// update DS mappings based on ppUpdateArray
// TODO : copy updates are completely broken
// TODO : Validate that actual VkDescriptorInfo in pDescriptors matches type:
//   pImageInfo array should be used for each descriptor if type is:
//    VK_DESCRIPTOR_TYPE_SAMPLER:
//      - uses sampler field of VkDescriptorImageInfo,
//    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
//       - uses sampler if binding doesn't use immutable sampler,
//       - uses imageView & imageLayout
//    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
//    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
//       - all use imageView & imageLayout
//   pTexelBuffer array should be used for each descriptor if type is:
//    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
//   pBufferInfo array should be used for each descriptor if type is:
//    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
// TODO : Verify we validate this spec req: All consecutive bindings updated via a single
//  VkWriteDescriptorSet structure must have identical descriptorType and stageFlags,
//  and must all either use immutable samplers or none must use immutable samplers.
static VkBool32 dsUpdate(layer_data* my_data, VkDevice device, VkStructureType type, uint32_t updateCount, const void* pUpdateArray)
{
    const VkWriteDescriptorSet *pWDS = NULL;
    const VkCopyDescriptorSet *pCDS = NULL;
    VkBool32 skipCall = VK_FALSE;

    if (type == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET) {
        pWDS = (const VkWriteDescriptorSet *)pUpdateArray;
        skipCall |= validateDescriptorSetImageView(device, updateCount, pWDS);
    } else {
        pCDS = (const VkCopyDescriptorSet *)pUpdateArray;
    }

    loader_platform_thread_lock_mutex(&globalLock);
    LAYOUT_NODE* pLayout = NULL;
    VkDescriptorSetLayoutCreateInfo* pLayoutCI = NULL;
    // TODO : If pCIList is NULL, flag error
    // Perform all updates
    for (uint32_t i = 0; i < updateCount; i++) {
        VkDescriptorSet ds = (pWDS) ? pWDS[i].destSet : pCDS[i].destSet;
        SET_NODE* pSet = my_data->setMap[ds.handle]; // getSetNode() without locking
        g_lastBoundDescriptorSet = pSet->set;
        GENERIC_HEADER* pUpdate = (pWDS) ? (GENERIC_HEADER*) &pWDS[i] : (GENERIC_HEADER*) &pCDS[i];
        pLayout = pSet->pLayout;
        // First verify valid update struct
        if ((skipCall = validUpdateStruct(device, pUpdate)) == VK_TRUE) {
            break;
        }
        // Make sure that binding is within bounds
        uint32_t binding = 0, endIndex = 0;
        skipCall |= getUpdateBinding(device, pUpdate, &binding);
        if (pLayout->createInfo.count < binding) {
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, ds.handle, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                    "Descriptor Set %p does not have binding to match update binding %u for update type %s!", ds, binding, string_VkStructureType(pUpdate->sType));
        } else {
            // Next verify that update falls within size of given binding
            skipCall |= getUpdateBinding(device, pUpdate, &binding);
            skipCall |= getUpdateEndIndex(device, pLayout, pUpdate, &endIndex);
            if (getBindingEndIndex(pLayout, binding) < endIndex) {
                // TODO : Keep count of layout CI structs and size this string dynamically based on that count
                pLayoutCI = &pLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, ds.handle, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                        "Descriptor update type of %s is out of bounds for matching binding %u in Layout w/ CI:\n%s!", string_VkStructureType(pUpdate->sType), binding, DSstr.c_str());
            } else { // TODO : should we skip update on a type mismatch or force it?
                // Layout bindings match w/ update ok, now verify that update is of the right type
                if ((skipCall = validateUpdateType(device, pLayout, pUpdate)) == VK_FALSE) {
                    // Save the update info
                    // TODO : Info message that update successful
                    // Create new update struct for this set's shadow copy
                    GENERIC_HEADER* pNewNode = NULL;
                    skipCall |= shadowUpdateNode(device, pUpdate, &pNewNode);
                    if (NULL == pNewNode) {
                        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, ds.handle, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                                "Out of memory while attempting to allocate UPDATE struct in vkUpdateDescriptors()");
                    } else {
                        // Insert shadow node into LL of updates for this set
                        pNewNode->pNext = pSet->pUpdateStructs;
                        pSet->pUpdateStructs = pNewNode;
                        // Now update appropriate descriptor(s) to point to new Update node
                        skipCall |= getUpdateEndIndex(device, pLayout, pUpdate, &endIndex);
                        uint32_t startIndex;
                        skipCall |= getUpdateStartIndex(device, pLayout, pUpdate, &startIndex);
                        for (uint32_t j = startIndex; j <= endIndex; j++) {
                            assert(j<pSet->descriptorCount);
                            pSet->ppDescriptors[j] = pNewNode;
                        }
                    }
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return skipCall;
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
static void deletePools(layer_data* my_data)
{
    if (my_data->poolMap.size() <= 0)
        return;
    for (auto ii=my_data->poolMap.begin(); ii!=my_data->poolMap.end(); ++ii) {
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
    my_data->poolMap.clear();
}
// WARN : Once deleteLayouts() called, any layout ptrs in Pool/Set data structure will be invalid
// NOTE : Calls to this function should be wrapped in mutex
static void deleteLayouts(layer_data* my_data)
{
    if (my_data->layoutMap.size() <= 0)
        return;
    for (auto ii=my_data->layoutMap.begin(); ii!=my_data->layoutMap.end(); ++ii) {
        LAYOUT_NODE* pLayout = (*ii).second;
        if (pLayout->createInfo.pBinding) {
            for (uint32_t i=0; i<pLayout->createInfo.count; i++) {
                if (pLayout->createInfo.pBinding[i].pImmutableSamplers)
                    delete[] pLayout->createInfo.pBinding[i].pImmutableSamplers;
            }
            delete[] pLayout->createInfo.pBinding;
        }
        delete pLayout;
    }
    my_data->layoutMap.clear();
}
// Currently clearing a set is removing all previous updates to that set
//  TODO : Validate if this is correct clearing behavior
static void clearDescriptorSet(layer_data* my_data, VkDescriptorSet set)
{
    SET_NODE* pSet = getSetNode(my_data, set);
    if (!pSet) {
        // TODO : Return error
    } else {
        loader_platform_thread_lock_mutex(&globalLock);
        freeShadowUpdateTree(pSet);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
}

static void clearDescriptorPool(layer_data* my_data, const VkDevice device, const VkDescriptorPool pool)
{
    POOL_NODE* pPool = getPoolNode(my_data, pool);
    if (!pPool) {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, pool.handle, 0, DRAWSTATE_INVALID_POOL, "DS",
                "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkResetDescriptorPool() call", pool.handle);
    } else {
        // For every set off of this pool, clear it
        SET_NODE* pSet = pPool->pSets;
        while (pSet) {
            clearDescriptorSet(my_data, pSet->set);
        }
    }
}
// For given CB object, fetch associated CB Node from map
static GLOBAL_CB_NODE* getCBNode(layer_data* my_data, const VkCmdBuffer cb)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->cmdBufferMap.find(cb) == my_data->cmdBufferMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        // TODO : How to pass cb as srcObj here?
        log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS",
                "Attempt to use CmdBuffer %#" PRIxLEAST64 " that doesn't exist!", reinterpret_cast<uint64_t>(cb));
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->cmdBufferMap[cb];
}
// Free all CB Nodes
// NOTE : Calls to this function should be wrapped in mutex
static void deleteCmdBuffers(layer_data* my_data)
{
    if (my_data->cmdBufferMap.size() <= 0)
        return;
    for (auto ii=my_data->cmdBufferMap.begin(); ii!=my_data->cmdBufferMap.end(); ++ii) {
        vector<CMD_NODE*> cmd_node_list = (*ii).second->pCmds;
        while (!cmd_node_list.empty()) {
            CMD_NODE* cmd_node = cmd_node_list.back();
            delete cmd_node;
            cmd_node_list.pop_back();
        }
        delete (*ii).second;
    }
    my_data->cmdBufferMap.clear();
}
static VkBool32 report_error_no_cb_begin(const VkCmdBuffer cb, const char* caller_name)
{
    // TODO : How to pass cb as srcObj here?
    return log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NO_BEGIN_CMD_BUFFER, "DS",
            "You must call vkBeginCommandBuffer() before this call to %s", (void*)caller_name);
}
static VkBool32 addCmd(GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd)
{
    VkBool32 skipCall = VK_FALSE;
    CMD_NODE* pCmd = new CMD_NODE;
    if (pCmd) {
        // init cmd node and append to end of cmd LL
        memset(pCmd, 0, sizeof(CMD_NODE));
        pCmd->cmdNumber = ++pCB->numCmds;
        pCmd->type = cmd;
        pCB->pCmds.push_back(pCmd);
    } else {
        // TODO : How to pass cb as srcObj here?
        skipCall |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                "Out of memory while attempting to allocate new CMD_NODE for cmdBuffer %#" PRIxLEAST64, reinterpret_cast<uint64_t>(pCB->cmdBuffer));
    }
    return skipCall;
}
static void resetCB(layer_data* my_data, const VkCmdBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB) {
        vector<CMD_NODE*> cmd_list = pCB->pCmds;
        while (!cmd_list.empty()) {
            delete cmd_list.back();
            cmd_list.pop_back();
        }
        pCB->pCmds.clear();
        // Reset CB state (need to save createInfo)
        VkCmdBufferCreateInfo saveCBCI = pCB->createInfo;
        memset(pCB, 0, sizeof(GLOBAL_CB_NODE));
        pCB->cmdBuffer = cb;
        pCB->createInfo = saveCBCI;
        pCB->lastVtxBinding = MAX_BINDING;
    }
}
// Set PSO-related status bits for CB, including dynamic state set via PSO
static void set_cb_pso_status(GLOBAL_CB_NODE* pCB, const PIPELINE_NODE* pPipe)
{
    for (uint32_t i = 0; i < pPipe->cbStateCI.attachmentCount; i++) {
        if (0 != pPipe->pAttachments[i].channelWriteMask) {
            pCB->status |= CBSTATUS_COLOR_BLEND_WRITE_ENABLE;
        }
    }
    if (pPipe->dsStateCI.depthWriteEnable) {
        pCB->status |= CBSTATUS_DEPTH_WRITE_ENABLE;
    }
    if (pPipe->dsStateCI.stencilTestEnable) {
        pCB->status |= CBSTATUS_STENCIL_TEST_ENABLE;
    }
    // Account for any dynamic state not set via this PSO
    if (!pPipe->dynStateCI.dynamicStateCount) { // All state is static
        pCB->status = CBSTATUS_ALL;
    } else {
        // First consider all state on
        // Then unset any state that's noted as dynamic in PSO
        // Finally OR that into CB statemask
        CBStatusFlags psoDynStateMask = CBSTATUS_ALL;
        for (uint32_t i=0; i < pPipe->dynStateCI.dynamicStateCount; i++) {
            switch (pPipe->dynStateCI.pDynamicStates[i]) {
                case VK_DYNAMIC_STATE_VIEWPORT:
                    psoDynStateMask &= ~CBSTATUS_VIEWPORT_SET;
                    break;
                case VK_DYNAMIC_STATE_SCISSOR:
                    psoDynStateMask &= ~CBSTATUS_SCISSOR_SET;
                    break;
                case VK_DYNAMIC_STATE_LINE_WIDTH:
                    psoDynStateMask &= ~CBSTATUS_LINE_WIDTH_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BIAS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BIAS_SET;
                    break;
                case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
                    psoDynStateMask &= ~CBSTATUS_BLEND_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BOUNDS_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_READ_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_WRITE_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_REFERENCE_SET;
                    break;
                default:
                    // TODO : Flag error here
                    break;
            }
        }
        pCB->status |= psoDynStateMask;
    }
}
// Print the last bound Gfx Pipeline
static VkBool32 printPipeline(layer_data* my_data, const VkCmdBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(my_data, pCB->lastBoundPipeline);
        if (!pPipeTrav) {
            // nothing to print
        } else {
            skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    vk_print_vkgraphicspipelinecreateinfo(&pPipeTrav->graphicsPipelineCI, "{DS}").c_str());
        }
    }
    return skipCall;
}
// Print details of DS config to stdout
static VkBool32 printDSConfig(layer_data* my_data, const VkCmdBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    char ds_config_str[1024*256] = {0}; // TODO : Currently making this buffer HUGE w/o overrun protection.  Need to be smarter, start smaller, and grow as needed.
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->lastBoundDescriptorSet) {
        SET_NODE* pSet = getSetNode(my_data, pCB->lastBoundDescriptorSet);
        POOL_NODE* pPool = getPoolNode(my_data, pSet->pool);
        // Print out pool details
        skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Details for pool %#" PRIxLEAST64 ".", pPool->pool.handle);
        string poolStr = vk_print_vkdescriptorpoolcreateinfo(&pPool->createInfo, " ");
        skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "%s", poolStr.c_str());
        // Print out set details
        char prefix[10];
        uint32_t index = 0;
        skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Details for descriptor set %#" PRIxLEAST64 ".", pSet->set.handle);
        LAYOUT_NODE* pLayout = pSet->pLayout;
        // Print layout details
        skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Layout #%u, (object %#" PRIxLEAST64 ") for DS %#" PRIxLEAST64 ".", index+1, (void*)pLayout->layout.handle, (void*)pSet->set.handle);
        sprintf(prefix, "  [L%u] ", index);
        string DSLstr = vk_print_vkdescriptorsetlayoutcreateinfo(&pLayout->createInfo, prefix).c_str();
        skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "%s", DSLstr.c_str());
        index++;
        GENERIC_HEADER* pUpdate = pSet->pUpdateStructs;
        if (pUpdate) {
            skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    "Update Chain [UC] for descriptor set %#" PRIxLEAST64 ":", pSet->set.handle);
            sprintf(prefix, "  [UC] ");
            skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    dynamic_display(pUpdate, prefix).c_str());
            // TODO : If there is a "view" associated with this update, print CI for that view
        } else {
            if (0 != pSet->descriptorCount) {
                skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                        "No Update Chain for descriptor set %#" PRIxLEAST64 " which has %u descriptors (vkUpdateDescriptors has not been called)", pSet->set.handle, pSet->descriptorCount);
            } else {
                skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                        "FYI: No descriptors in descriptor set %#" PRIxLEAST64 ".", pSet->set.handle);
            }
        }
    }
    return skipCall;
}

static void printCB(layer_data* my_data, const VkCmdBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->pCmds.size() > 0) {
        log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Cmds in CB %p", (void*)cb);
        vector<CMD_NODE*> pCmds = pCB->pCmds;
        for (auto ii=pCmds.begin(); ii!=pCmds.end(); ++ii) {
            // TODO : Need to pass cb as srcObj here
            log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                "  CMD#%lu: %s", (*ii)->cmdNumber, cmdTypeToString((*ii)->type).c_str());
        }
    } else {
        // Nothing to print
    }
}

static VkBool32 synchAndPrintDSConfig(layer_data* my_data, const VkCmdBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    if (!(mdd(cb)->active_flags & VK_DBG_REPORT_INFO_BIT)) {
        return skipCall;
    }
    skipCall |= printDSConfig(my_data, cb);
    skipCall |= printPipeline(my_data, cb);
    return skipCall;
}

// Flags validation error if the associated call is made inside a render pass. The apiName
// routine should ONLY be called outside a render pass.
static VkBool32 insideRenderPass(GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 inside = VK_FALSE;
    if (pCB->activeRenderPass) {
        inside = log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                         (uint64_t)pCB->cmdBuffer, 0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                         "%s: It is invalid to issue this call inside an active render pass (%#" PRIxLEAST64 ")",
                         apiName, pCB->activeRenderPass.handle);
    }
    return inside;
}

// Flags validation error if the associated call is made outside a render pass. The apiName
// routine should ONLY be called inside a render pass.
static VkBool32 outsideRenderPass(GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 outside = VK_FALSE;
    if (!pCB->activeRenderPass) {
        outside = log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                          (uint64_t)pCB->cmdBuffer, 0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
                          "%s: This call must be issued inside an active render pass.", apiName);
    }
    return outside;
}

static void init_draw_state(layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDbgMsgCallback callback;
    // initialize DrawState options
    report_flags = getLayerOptionFlags("DrawStateReportFlags", 0);
    getLayerOptionEnum("DrawStateDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("DrawStateLogFilename");
        log_output = getLayerLogOutput(option_str, "DrawState");
        layer_create_msg_callback(my_data->report_data, report_flags, log_callback, (void *) log_output, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        layer_create_msg_callback(my_data->report_data, report_flags, win32_debug_output_msg, NULL, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (!globalLockInitialized)
    {
        // This mutex may be deleted by vkDestroyInstance of last instance.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult result = pTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->extensionCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        init_draw_state(my_data);
    }
    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT void VKAPI vkDestroyInstance(VkInstance instance)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDbgMsgCallback callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
    // TODO : Potential race here with separate threads creating/destroying instance
    if (layer_data_map.empty()) {
        // Release mutex when destroying last instance.
        loader_platform_thread_delete_mutex(&globalLock);
        globalLockInitialized = 0;
    }
}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_extensions.debug_marker_enabled = false;

    for (i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], DEBUG_MARKER_EXTENSION_NAME) == 0) {
            /* Found a matching extension name, mark it enabled and init dispatch table*/
            initDebugMarkerTable(device);
            dev_data->device_extensions.debug_marker_enabled = true;
        }
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDevice(gpu, pCreateInfo, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        dev_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    }
    return result;
}
// prototype
static void deleteRenderPasses(layer_data*);
VK_LAYER_EXPORT void VKAPI vkDestroyDevice(VkDevice device)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data* dev_data = get_my_data_ptr(key, layer_data_map);
    // Free all the memory
    loader_platform_thread_lock_mutex(&globalLock);
    deletePipelines(dev_data);
    deleteRenderPasses(dev_data);
    deleteCmdBuffers(dev_data);
    deletePools(dev_data);
    deleteLayouts(dev_data);
    loader_platform_thread_unlock_mutex(&globalLock);

    dev_data->device_dispatch_table->DestroyDevice(device);
    tableDebugMarkerMap.erase(key);
    delete dev_data->device_dispatch_table;
    layer_data_map.erase(key);
}

static const VkLayerProperties ds_global_layers[] = {
    {
        "DrawState",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: DrawState",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* DrawState does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(ds_global_layers),
                                   ds_global_layers,
                                   pCount, pProperties);
}

static const VkExtensionProperties ds_device_extensions[] = {
    {
        DEBUG_MARKER_EXTENSION_NAME,
        VK_MAKE_VERSION(0, 1, 0),
    }
};

static const VkLayerProperties ds_device_layers[] = {
    {
        "DrawState",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: DrawState",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* Mem tracker does not have any physical device extensions */
    return util_GetExtensionProperties(ARRAY_SIZE(ds_device_extensions), ds_device_extensions,
                                       pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* Mem tracker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(ds_device_layers), ds_device_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t cmdBufferCount, const VkCmdBuffer* pCmdBuffers, VkFence fence)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = NULL;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    for (uint32_t i=0; i < cmdBufferCount; i++) {
        // Validate that cmd buffers have been updated
        pCB = getCBNode(dev_data, pCmdBuffers[i]);
        loader_platform_thread_lock_mutex(&globalLock);
        pCB->submitCount++; // increment submit count
        if ((pCB->beginInfo.flags & VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT) && (pCB->submitCount > 1)) {
            skipCall |= log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_CMD_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                    "CB %#" PRIxLEAST64 " was begun w/ VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT set, but has been submitted %#" PRIxLEAST64 " times.", reinterpret_cast<uint64_t>(pCB->cmdBuffer), pCB->submitCount);
        }
        if (CB_UPDATE_COMPLETE != pCB->state) {
            // Flag error for using CB w/o vkEndCommandBuffer() called
            // TODO : How to pass cb as srcObj?
            skipCall |= log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NO_END_CMD_BUFFER, "DS",
                    "You must call vkEndCommandBuffer() on CB %#" PRIxLEAST64 " before this call to vkQueueSubmit()!", reinterpret_cast<uint64_t>(pCB->cmdBuffer));
            loader_platform_thread_unlock_mutex(&globalLock);
            return VK_ERROR_VALIDATION_FAILED;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    if (VK_FALSE == skipCall)
        return dev_data->device_dispatch_table->QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT void VKAPI vkDestroyFence(VkDevice device, VkFence fence)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyFence(device, fence);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroySemaphore(device, semaphore);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyEvent(VkDevice device, VkEvent event)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyEvent(device, event);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyQueryPool(device, queryPool);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyBuffer(device, buffer);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyBufferView(device, bufferView);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyImage(VkDevice device, VkImage image)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyImage(device, image);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyImageView(VkDevice device, VkImageView imageView)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyImageView(device, imageView);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyShaderModule(device, shaderModule);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyShader(VkDevice device, VkShader shader)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyShader(device, shader);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipeline(device, pipeline);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipelineLayout(device, pipelineLayout);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroySampler(VkDevice device, VkSampler sampler)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroySampler(device, sampler);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorSetLayout(device, descriptorSetLayout);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorPool(device, descriptorPool);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyCommandBuffer(VkDevice device, VkCmdBuffer commandBuffer)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyCommandBuffer(device, commandBuffer);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyFramebuffer(device, framebuffer);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyRenderPass(device, renderPass);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateBufferView(device, pCreateInfo, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->bufferViewMap[pView->handle] = unique_ptr<BUFFER_NODE>(new BUFFER_NODE(pView, pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateImageView(device, pCreateInfo, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->imageViewMap[pView->handle] = unique_ptr<VkImageViewCreateInfo>(new VkImageViewCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

//TODO handle pipeline caches
VkResult VKAPI vkCreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    VkPipelineCache*                            pPipelineCache)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineCache(device, pCreateInfo, pPipelineCache);
    return result;
}

void VKAPI vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyPipelineCache(device, pipelineCache);
}

size_t VKAPI vkGetPipelineCacheSize(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    size_t size = dev_data->device_dispatch_table->GetPipelineCacheSize(device, pipelineCache);
    return size;
}

VkResult VKAPI vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    void*                                       pData)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetPipelineCacheData(device, pipelineCache, pData);
    return result;
}

VkResult VKAPI vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             destCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->MergePipelineCaches(device, destCache, srcCacheCount, pSrcCaches);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines)
{
    VkResult result = VK_SUCCESS;
    //TODO What to do with pipelineCache?
    // The order of operations here is a little convoluted but gets the job done
    //  1. Pipeline create state is first shadowed into PIPELINE_NODE struct
    //  2. Create state is then validated (which uses flags setup during shadowing)
    //  3. If everything looks good, we'll then create the pipeline and add NODE to pipelineMap
    VkBool32 skipCall = VK_FALSE;
    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_NODE*> pPipeNode(count);
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    uint32_t i=0;
    loader_platform_thread_lock_mutex(&globalLock);
    for (i=0; i<count; i++) {
        pPipeNode[i] = initPipeline(&pCreateInfos[i], NULL);
        skipCall |= verifyPipelineCreateState(device, pPipeNode[i]);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = dev_data->device_dispatch_table->CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
        loader_platform_thread_lock_mutex(&globalLock);
        for (i=0; i<count; i++) {
            pPipeNode[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeNode[i]->pipeline.handle] = pPipeNode[i];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        for (i=0; i<count; i++) {
            if (pPipeNode[i]) {
                // If we allocated a pipeNode, need to clean it up here
                delete[] pPipeNode[i]->pVertexBindingDescriptions;
                delete[] pPipeNode[i]->pVertexAttributeDescriptions;
                delete[] pPipeNode[i]->pAttachments;
                delete pPipeNode[i];
            }
        }
        return VK_ERROR_VALIDATION_FAILED;
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSampler(device, pCreateInfo, pSampler);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->sampleMap[pSampler->handle] = unique_ptr<SAMPLER_NODE>(new SAMPLER_NODE(pSampler, pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    if (VK_SUCCESS == result) {
        LAYOUT_NODE* pNewNode = new LAYOUT_NODE;
        if (NULL == pNewNode) {
            if (log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (*pSetLayout).handle, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate LAYOUT_NODE in vkCreateDescriptorSetLayout()"))
                return VK_ERROR_VALIDATION_FAILED;
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
            pNewNode->descriptorTypes.reserve(totalCount);
            uint32_t offset = 0;
            uint32_t j = 0;
            for (uint32_t i=0; i<pCreateInfo->count; i++) {
                for (j = 0; j < pCreateInfo->pBinding[i].arraySize; j++) {
                    pNewNode->descriptorTypes[offset + j] = pCreateInfo->pBinding[i].descriptorType;
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
        dev_data->layoutMap[pSetLayout->handle] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VkResult VKAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout* pPipelineLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);
    if (VK_SUCCESS == result) {
        PIPELINE_LAYOUT_NODE plNode = dev_data->pipelineLayoutMap[pPipelineLayout->handle];
        plNode.descriptorSetLayouts.reserve(pCreateInfo->descriptorSetCount);
        uint32_t i = 0;
        for (i=0; i<pCreateInfo->descriptorSetCount; ++i) {
            plNode.descriptorSetLayouts[i] = pCreateInfo->pSetLayouts[i];
        }
        plNode.pushConstantRanges.reserve(pCreateInfo->pushConstantRangeCount);
        for (i=0; i<pCreateInfo->pushConstantRangeCount; ++i) {
            plNode.pushConstantRanges[i] = pCreateInfo->pPushConstantRanges[i];
        }
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorPool(device, pCreateInfo, pDescriptorPool);
    if (VK_SUCCESS == result) {
        // Insert this pool into Global Pool LL at head
        if (log_msg(mdd(device), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (*pDescriptorPool).handle, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                "Created Descriptor Pool %#" PRIxLEAST64, (*pDescriptorPool).handle))
            return VK_ERROR_VALIDATION_FAILED;
        loader_platform_thread_lock_mutex(&globalLock);
        POOL_NODE* pNewNode = new POOL_NODE;
        if (NULL == pNewNode) {
            if (log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (*pDescriptorPool).handle, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate POOL_NODE in vkCreateDescriptorPool()"))
                return VK_ERROR_VALIDATION_FAILED;
        } else {
            memset(pNewNode, 0, sizeof(POOL_NODE));
            VkDescriptorPoolCreateInfo* pCI = (VkDescriptorPoolCreateInfo*)&pNewNode->createInfo;
            memcpy((void*)pCI, pCreateInfo, sizeof(VkDescriptorPoolCreateInfo));
            if (pNewNode->createInfo.count) {
                size_t typeCountSize = pNewNode->createInfo.count * sizeof(VkDescriptorTypeCount);
                pNewNode->createInfo.pTypeCount = new VkDescriptorTypeCount[typeCountSize];
                memcpy((void*)pNewNode->createInfo.pTypeCount, pCreateInfo->pTypeCount, typeCountSize);
            }
            pNewNode->poolUsage  = pCreateInfo->poolUsage;
            pNewNode->maxSets    = pCreateInfo->maxSets;
            pNewNode->pool       = *pDescriptorPool;
            dev_data->poolMap[pDescriptorPool->handle] = pNewNode;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        // Need to do anything if pool create fails?
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->ResetDescriptorPool(device, descriptorPool);
    if (VK_SUCCESS == result) {
        clearDescriptorPool(dev_data, device, descriptorPool);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->AllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets);
    if (VK_SUCCESS == result) {
        POOL_NODE *pPoolNode = getPoolNode(dev_data, descriptorPool);
        if (!pPoolNode) {
            log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, descriptorPool.handle, 0, DRAWSTATE_INVALID_POOL, "DS",
                    "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkAllocDescriptorSets() call", descriptorPool.handle);
        } else {
            if (count == 0) {
                log_msg(mdd(device), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, count, 0, DRAWSTATE_NONE, "DS",
                        "AllocDescriptorSets called with 0 count");
            }
            for (uint32_t i = 0; i < count; i++) {
                log_msg(mdd(device), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, pDescriptorSets[i].handle, 0, DRAWSTATE_NONE, "DS",
                        "Created Descriptor Set %#" PRIxLEAST64, pDescriptorSets[i].handle);
                // Create new set node and add to head of pool nodes
                SET_NODE* pNewNode = new SET_NODE;
                if (NULL == pNewNode) {
                    if (log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, pDescriptorSets[i].handle, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                            "Out of memory while attempting to allocate SET_NODE in vkAllocDescriptorSets()"))
                        return VK_ERROR_VALIDATION_FAILED;
                } else {
                    memset(pNewNode, 0, sizeof(SET_NODE));
                    // TODO : Pool should store a total count of each type of Descriptor available
                    //  When descriptors are allocated, decrement the count and validate here
                    //  that the count doesn't go below 0. One reset/free need to bump count back up.
                    // Insert set at head of Set LL for this pool
                    pNewNode->pNext = pPoolNode->pSets;
                    pPoolNode->pSets = pNewNode;
                    LAYOUT_NODE* pLayout = getLayoutNode(dev_data, pSetLayouts[i]);
                    if (NULL == pLayout) {
                        if (log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, pSetLayouts[i].handle, 0, DRAWSTATE_INVALID_LAYOUT, "DS",
                                "Unable to find set layout node for layout %#" PRIxLEAST64 " specified in vkAllocDescriptorSets() call", pSetLayouts[i].handle))
                            return VK_ERROR_VALIDATION_FAILED;
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
                    dev_data->setMap[pDescriptorSets[i].handle] = pNewNode;
                }
            }
        }
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    POOL_NODE *pPoolNode = getPoolNode(dev_data, descriptorPool);
    if (pPoolNode && (VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT == pPoolNode->poolUsage)) {
        // Can't Free from a ONE_SHOT pool
        skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, (uint64_t)device, 0, DRAWSTATE_CANT_FREE_FROM_ONE_SHOT_POOL, "DS",
                    "It is invalid to call vkFreeDescriptorSets() with a pool created with usage type VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT.");
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
    VkResult result = dev_data->device_dispatch_table->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    // TODO : Clean up any internal data structures using this obj.
    return result;
}

VK_LAYER_EXPORT void VKAPI vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t copyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    // dsUpdate will return VK_TRUE only if a bailout error occurs, so we want to call down tree when both updates return VK_FALSE
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    if (!dsUpdate(dev_data, device, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, writeCount, pDescriptorWrites) &&
        !dsUpdate(dev_data, device, VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET, copyCount, pDescriptorCopies)) {
        dev_data->device_dispatch_table->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        GLOBAL_CB_NODE* pCB = new GLOBAL_CB_NODE;
        memset(pCB, 0, sizeof(GLOBAL_CB_NODE));
        pCB->cmdBuffer = *pCmdBuffer;
        pCB->createInfo = *pCreateInfo;
        pCB->lastVtxBinding = MAX_BINDING;
        pCB->level = pCreateInfo->level;
        dev_data->cmdBufferMap[*pCmdBuffer] = pCB;
        loader_platform_thread_unlock_mutex(&globalLock);
        updateCBTracking(pCB);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    VkBool32 skipCall = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    // Validate command buffer level
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->level == VK_CMD_BUFFER_LEVEL_PRIMARY) {
            if (pBeginInfo->renderPass.handle || pBeginInfo->framebuffer.handle) {
                // These should be NULL for a Primary CB
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                    "vkCreateCommandBuffer():  Primary Command Buffer (%p) may not specify framebuffer or renderpass parameters", (void*)cmdBuffer);
            }
        } else {
            if (!pBeginInfo->renderPass.handle || !pBeginInfo->framebuffer.handle) {
                // These should NOT be null for an Secondary CB
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                    "vkCreateCommandBuffer():  Secondary Command Buffers (%p) must specify framebuffer and renderpass parameters", (void*)cmdBuffer);
            }
        }
        pCB->beginInfo = *pBeginInfo;
    } else {
        // TODO : Need to pass cmdBuffer as objType here
        skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_CMD_BUFFER, "DS",
                "In vkBeginCommandBuffer() and unable to find CmdBuffer Node for CB %p!", (void*)cmdBuffer);
    }
    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED;
    }
    VkResult result = dev_data->device_dispatch_table->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    if (VK_SUCCESS == result) {
        if (CB_NEW != pCB->state)
            resetCB(dev_data, cmdBuffer);
        pCB->state = CB_UPDATE_ACTIVE;
        updateCBTracking(pCB);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCmdBuffer cmdBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    VkResult result = VK_SUCCESS;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    /* TODO: preference is to always call API function after reporting any validation errors */
    if (pCB) {
        if (pCB->state != CB_UPDATE_ACTIVE) {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkEndCommandBuffer()");
        }
    }
    if (VK_FALSE == skipCall) {
        result = dev_data->device_dispatch_table->EndCommandBuffer(cmdBuffer);
        if (VK_SUCCESS == result) {
            updateCBTracking(pCB);
            pCB->state = CB_UPDATE_COMPLETE;
            // Reset CB status flags
            pCB->status = 0;
            printCB(dev_data, cmdBuffer);
        }
    } else {
        result = VK_ERROR_VALIDATION_FAILED;
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCmdBuffer cmdBuffer, VkCmdBufferResetFlags flags)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->ResetCommandBuffer(cmdBuffer, flags);
    if (VK_SUCCESS == result) {
        resetCB(dev_data, cmdBuffer);
        updateCBTracking(getCBNode(dev_data, cmdBuffer));
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_BINDPIPELINE);
            if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, pipeline.handle,
                                    0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                                    "Incorrectly binding compute pipeline (%#" PRIxLEAST64 ") during active RenderPass (%#" PRIxLEAST64 ")",
                                    pipeline.handle, pCB->activeRenderPass.handle);
            } else if ((VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) && (!pCB->activeRenderPass)) {
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, pipeline.handle,
                                    0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS", "Incorrectly binding graphics pipeline "
                                    " (%#" PRIxLEAST64 ") without an active RenderPass", pipeline.handle);
            } else {
                PIPELINE_NODE* pPN = getPipeline(dev_data, pipeline);
                if (pPN) {
                    pCB->lastBoundPipeline = pipeline;
                    loader_platform_thread_lock_mutex(&globalLock);
                    set_cb_pso_status(pCB, pPN);
                    g_lastBoundPipeline = pPN;
                    loader_platform_thread_unlock_mutex(&globalLock);
                    skipCall |= validatePipelineState(dev_data, pCB, pipelineBindPoint, pipeline);
                } else {
                    skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, pipeline.handle,
                                        0, DRAWSTATE_INVALID_PIPELINE, "DS",
                                        "Attempt to bind Pipeline %#" PRIxLEAST64 " that doesn't exist!", (void*)pipeline.handle);
                }
            }
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdBindPipeline()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetViewport(
    VkCmdBuffer                         cmdBuffer,
    uint32_t                            viewportCount,
    const VkViewport*                   pViewports)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETVIEWPORTSTATE);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_VIEWPORT_SET;
            pCB->viewports.resize(viewportCount);
            memcpy(pCB->viewports.data(), pViewports, viewportCount * sizeof(VkViewport));
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetViewport()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetViewport(cmdBuffer, viewportCount, pViewports);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetScissor(
    VkCmdBuffer                         cmdBuffer,
    uint32_t                            scissorCount,
    const VkRect2D*                     pScissors)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETSCISSORSTATE);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_SCISSOR_SET;
            pCB->scissors.resize(scissorCount);
            memcpy(pCB->scissors.data(), pScissors, scissorCount * sizeof(VkRect2D));
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetScissor()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetScissor(cmdBuffer, scissorCount, pScissors);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetLineWidth(VkCmdBuffer cmdBuffer, float lineWidth)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETLINEWIDTHSTATE);
            /* TODO: Do we still need this lock? */
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_LINE_WIDTH_SET;
            pCB->lineWidth = lineWidth;
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdBindDynamicLineWidthState()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetLineWidth(cmdBuffer, lineWidth);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetDepthBias(
    VkCmdBuffer                         cmdBuffer,
    float                               depthBias,
    float                               depthBiasClamp,
    float                               slopeScaledDepthBias)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETDEPTHBIASSTATE);
            pCB->status |= CBSTATUS_DEPTH_BIAS_SET;
            pCB->depthBias = depthBias;
            pCB->depthBiasClamp = depthBiasClamp;
            pCB->slopeScaledDepthBias = slopeScaledDepthBias;
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetDepthBias()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBias(cmdBuffer, depthBias, depthBiasClamp, slopeScaledDepthBias);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetBlendConstants(VkCmdBuffer cmdBuffer, const float blendConst[4])
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETBLENDSTATE);
            pCB->status |= CBSTATUS_BLEND_SET;
            memcpy(pCB->blendConst, blendConst, 4 * sizeof(float));
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetBlendConstants()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetBlendConstants(cmdBuffer, blendConst);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetDepthBounds(
    VkCmdBuffer                         cmdBuffer,
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETDEPTHBOUNDSSTATE);
            pCB->status |= CBSTATUS_DEPTH_BOUNDS_SET;
            pCB->minDepthBounds = minDepthBounds;
            pCB->maxDepthBounds = maxDepthBounds;
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetDepthBounds()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBounds(cmdBuffer, minDepthBounds, maxDepthBounds);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetStencilCompareMask(
    VkCmdBuffer                         cmdBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            stencilCompareMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETSTENCILREADMASKSTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.stencilCompareMask = stencilCompareMask;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.stencilCompareMask = stencilCompareMask;
            }
            /* TODO: Do we need to track front and back separately? */
            /* TODO: We aren't capturing the faceMask, do we need to? */
            pCB->status |= CBSTATUS_STENCIL_READ_MASK_SET;
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetStencilCompareMask()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilCompareMask(cmdBuffer, faceMask, stencilCompareMask);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetStencilWriteMask(
    VkCmdBuffer                         cmdBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            stencilWriteMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETSTENCILWRITEMASKSTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.stencilWriteMask = stencilWriteMask;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.stencilWriteMask = stencilWriteMask;
            }
            pCB->status |= CBSTATUS_STENCIL_WRITE_MASK_SET;
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetStencilWriteMask()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilWriteMask(cmdBuffer, faceMask, stencilWriteMask);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetStencilReference(
    VkCmdBuffer                         cmdBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            stencilReference)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETSTENCILREFERENCESTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.stencilReference = stencilReference;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.stencilReference = stencilReference;
            }
            pCB->status |= CBSTATUS_STENCIL_REFERENCE_SET;
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetStencilReference()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilReference(cmdBuffer, faceMask, stencilReference);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    // TODO : Validate dynamic offsets
    //  If any of the sets being bound include dynamic uniform or storage buffers,
    //  then pDynamicOffsets must include one element for each array element
    //  in each dynamic descriptor type binding in each set.
    //  dynamicOffsetCount is the total number of dynamic offsets provided, and
    //  must equal the total number of dynamic descriptors in the sets being bound
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                        "Incorrectly binding compute DescriptorSets during active RenderPass (%#" PRIxLEAST64 ")", pCB->activeRenderPass.handle);
            } else if ((VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) && (!pCB->activeRenderPass)) {
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
                        "Incorrectly binding graphics DescriptorSets without an active RenderPass");
            } else {
                for (uint32_t i=0; i<setCount; i++) {
                    SET_NODE* pSet = getSetNode(dev_data, pDescriptorSets[i]);
                    if (pSet) {
                        loader_platform_thread_lock_mutex(&globalLock);
                        pCB->lastBoundDescriptorSet = pDescriptorSets[i];
                        pCB->lastBoundPipelineLayout = layout;
                        pCB->boundDescriptorSets.push_back(pDescriptorSets[i]);
                        g_lastBoundDescriptorSet = pDescriptorSets[i];
                        loader_platform_thread_unlock_mutex(&globalLock);
                        skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, pDescriptorSets[i].handle, 0, DRAWSTATE_NONE, "DS",
                                "DS %#" PRIxLEAST64 " bound on pipeline %s", pDescriptorSets[i].handle, string_VkPipelineBindPoint(pipelineBindPoint));
                        if (!pSet->pUpdateStructs)
                            skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, pDescriptorSets[i].handle, 0, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                    "DS %#" PRIxLEAST64 " bound but it was never updated. You may want to either update it or not bind it.", pDescriptorSets[i].handle);
                    } else {
                        skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, pDescriptorSets[i].handle, 0, DRAWSTATE_INVALID_SET, "DS",
                                "Attempt to bind DS %#" PRIxLEAST64 " that doesn't exist!", pDescriptorSets[i].handle);
                    }
                }
                updateCBTracking(pCB);
                skipCall |= addCmd(pCB, CMD_BINDDESCRIPTORSETS);
            }
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdBindDescriptorSets()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            VkDeviceSize offset_align = 0;
            switch (indexType) {
                case VK_INDEX_TYPE_UINT16:
                    offset_align = 2;
                    break;
                case VK_INDEX_TYPE_UINT32:
                    offset_align = 4;
                    break;
                default:
                    // ParamChecker should catch bad enum, we'll also throw alignment error below if offset_align stays 0
                    break;
            }
            if (!offset_align || (offset % offset_align)) {
                skipCall |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR, "DS",
                    "vkCmdBindIndexBuffer() offset (%#" PRIxLEAST64 ") does not fall on alignment (%s) boundary.", offset, string_VkIndexType(indexType));
            }
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdBindIndexBuffer()");
        }
        pCB->status |= CBSTATUS_INDEX_BUFFER_BOUND;
        updateCBTracking(pCB);
        skipCall |= addCmd(pCB, CMD_BINDINDEXBUFFER);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            /* TODO: Need to track all the vertex buffers, not just last one */
            pCB->lastVtxBinding = startBinding + bindingCount -1;
            updateCBTracking(pCB);
            addCmd(pCB, CMD_BINDVERTEXBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdBindVertexBuffer()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VkCmdBuffer cmdBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
            // TODO : Need to pass cmdBuffer as srcObj here
            skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDraw() call #%lu, reporting DS state:", g_drawCount[DRAW]++);
            skipCall |= synchAndPrintDSConfig(dev_data, cmdBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(pCB, CMD_DRAW);
            }
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdDraw()");
        }
        skipCall |= outsideRenderPass(pCB, "vkCmdDraw");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDraw(cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VkCmdBuffer cmdBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDEXED]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
            // TODO : Need to pass cmdBuffer as srcObj here
            skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndexed() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED]++);
            skipCall |= synchAndPrintDSConfig(dev_data, cmdBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(pCB, CMD_DRAWINDEXED);
            }
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdDrawIndexed()");
        }
        skipCall |= outsideRenderPass(pCB, "vkCmdDrawIndexed");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexed(cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDIRECT]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
            // TODO : Need to pass cmdBuffer as srcObj here
            skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDIRECT]++);
            skipCall |= synchAndPrintDSConfig(dev_data, cmdBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(pCB, CMD_DRAWINDIRECT);
            }
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdDrawIndirect()");
        }
        skipCall |= outsideRenderPass(pCB, "vkCmdDrawIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDEXED_INDIRECT]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
            // TODO : Need to pass cmdBuffer as srcObj here
            skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndexedIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED_INDIRECT]++);
            skipCall |= synchAndPrintDSConfig(dev_data, cmdBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(pCB, CMD_DRAWINDEXEDINDIRECT);
            }
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdDrawIndexedIndirect()");
        }
        skipCall |= outsideRenderPass(pCB, "vkCmdDrawIndexedIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VkCmdBuffer cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_DISPATCH);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdDispatch()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdDispatch");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatch(cmdBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_DISPATCHINDIRECT);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdDispatchIndirect()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdDispatchIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_COPYBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdCopyBuffer()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdCopyBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VkCmdBuffer cmdBuffer,
                                             VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkImage destImage,
                                             VkImageLayout destImageLayout,
                                             uint32_t regionCount, const VkImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_COPYIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdCopyImage()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdCopyImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VkCmdBuffer cmdBuffer,
                                             VkImage srcImage, VkImageLayout srcImageLayout,
                                             VkImage destImage, VkImageLayout destImageLayout,
                                             uint32_t regionCount, const VkImageBlit* pRegions,
                                             VkTexFilter filter)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_BLITIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdBlitImage()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdBlitImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCmdBuffer cmdBuffer,
                                                     VkBuffer srcBuffer,
                                                     VkImage destImage, VkImageLayout destImageLayout,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_COPYBUFFERTOIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdCopyBufferToImage()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdCopyBufferToImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCmdBuffer cmdBuffer,
                                                     VkImage srcImage, VkImageLayout srcImageLayout,
                                                     VkBuffer destBuffer,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_COPYIMAGETOBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdCopyImageToBuffer()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdCopyImageToBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_UPDATEBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdUpdateBuffer()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdCopyUpdateBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize fillSize, uint32_t data)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_FILLBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdFillBuffer()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdCopyFillBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorAttachment(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    colorAttachment,
    VkImageLayout                               imageLayout,
    const VkClearColorValue*                    pColor,
    uint32_t                                    rectCount,
    const VkRect3D*                             pRects)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            // Warn if this is issued prior to Draw Cmd
            if (!hasDrawCmd(pCB)) {
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0, DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, "DS",
                        "vkCmdClearColorAttachment() issued on CB object 0x%" PRIxLEAST64 " prior to any Draw Cmds."
                        " It is recommended you use RenderPass LOAD_OP_CLEAR on Color Attachments prior to any Draw.", reinterpret_cast<uint64_t>(cmdBuffer));
            }
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_CLEARCOLORATTACHMENT);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdClearColorAttachment()");
        }

        // Validate that attachment is in reference list of active subpass
        if (pCB->activeRenderPass) {
            const VkRenderPassCreateInfo *pRPCI = dev_data->renderPassMap[pCB->activeRenderPass.handle];
            const VkSubpassDescription   *pSD   = &pRPCI->pSubpasses[pCB->activeSubpass];

            VkBool32 found = VK_FALSE;
            for (uint32_t i = 0; i < pSD->colorCount; i++) {
                if (colorAttachment == pSD->pColorAttachments[i].attachment) {
                    found = VK_TRUE;
                    break;
                }
            }
            if (VK_FALSE == found) {
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                        (uint64_t)cmdBuffer, 0, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                        "vkCmdClearColorAttachment() attachment index %d not found in attachment reference array of active subpass %d",
                        colorAttachment, pCB->activeSubpass);
            }
        }
        skipCall |= outsideRenderPass(pCB, "vkCmdClearColorAttachment");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearColorAttachment(cmdBuffer, colorAttachment, imageLayout, pColor, rectCount, pRects);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencilAttachment(
    VkCmdBuffer                                 cmdBuffer,
    VkImageAspectFlags                          imageAspectMask,
    VkImageLayout                               imageLayout,
    const VkClearDepthStencilValue*             pDepthStencil,
    uint32_t                                    rectCount,
    const VkRect3D*                             pRects)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            // Warn if this is issued prior to Draw Cmd
            if (!hasDrawCmd(pCB)) {
                // TODO : cmdBuffer should be srcObj
                skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, "DS",
                        "vkCmdClearDepthStencilAttachment() issued on CB object 0x%" PRIxLEAST64 " prior to any Draw Cmds."
                        " It is recommended you use RenderPass LOAD_OP_CLEAR on DS Attachment prior to any Draw.", reinterpret_cast<uint64_t>(cmdBuffer));
            }
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_CLEARDEPTHSTENCILATTACHMENT);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdClearDepthStencilAttachment()");
        }
        skipCall |= outsideRenderPass(pCB, "vkCmdClearDepthStencilAttachment");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearDepthStencilAttachment(cmdBuffer, imageAspectMask, imageLayout, pDepthStencil, rectCount, pRects);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(
        VkCmdBuffer cmdBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearColorValue *pColor,
        uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_CLEARCOLORIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdClearColorImage()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdClearColorImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencilImage(
        VkCmdBuffer cmdBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearDepthStencilValue *pDepthStencil,
        uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_CLEARDEPTHSTENCILIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdClearDepthStencilImage()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdClearDepthStencilImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearDepthStencilImage(cmdBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VkCmdBuffer cmdBuffer,
                                                VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkImage destImage, VkImageLayout destImageLayout,
                                                uint32_t regionCount, const VkImageResolve* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_RESOLVEIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdResolveImage()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdResolveImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_SETEVENT);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdSetEvent()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdSetEvent");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetEvent(cmdBuffer, event, stageMask);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_RESETEVENT);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdResetEvent()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdResetEvent");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetEvent(cmdBuffer, event, stageMask);
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(VkCmdBuffer cmdBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags sourceStageMask, VkPipelineStageFlags destStageMask, uint32_t memBarrierCount, const void* const* ppMemBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_WAITEVENTS);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdWaitEvents()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWaitEvents(cmdBuffer, eventCount, pEvents, sourceStageMask, destStageMask, memBarrierCount, ppMemBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(VkCmdBuffer cmdBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, VkBool32 byRegion, uint32_t memBarrierCount, const void* const* ppMemBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_PIPELINEBARRIER);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdPipelineBarrier()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, byRegion, memBarrierCount, ppMemBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_BEGINQUERY);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdBeginQuery()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_ENDQUERY);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdEndQuery()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_RESETQUERYPOOL);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdResetQueryPool()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdQueryPool");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyQueryPoolResults(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery,
                                                     uint32_t queryCount, VkBuffer destBuffer, VkDeviceSize destOffset,
                                                     VkDeviceSize destStride, VkQueryResultFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_COPYQUERYPOOLRESULTS);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdCopyQueryPoolResults()");
        }
        skipCall |= insideRenderPass(pCB, "vkCmdCopyQueryPoolResults");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyQueryPoolResults(cmdBuffer, queryPool,
                           startQuery, queryCount, destBuffer, destOffset, destStride, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCmdBuffer cmdBuffer, VkTimestampType timestampType, VkBuffer destBuffer, VkDeviceSize destOffset)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_WRITETIMESTAMP);
        } else {
            skipCall |= report_error_no_cb_begin(cmdBuffer, "vkCmdWriteTimestamp()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VkFramebufferCreateInfo* localFBCI = new VkFramebufferCreateInfo(*pCreateInfo);
        if (pCreateInfo->pAttachments) {
            localFBCI->pAttachments = new VkImageView[localFBCI->attachmentCount];
            memcpy((void*)localFBCI->pAttachments, pCreateInfo->pAttachments, localFBCI->attachmentCount*sizeof(VkImageView));
        }
        dev_data->frameBufferMap[pFramebuffer->handle] = localFBCI;
    }
    return result;
}

// Store the DAG.
struct DAGNode {
    uint32_t pass;
    std::vector<uint32_t> prev;
    std::vector<uint32_t> next;
};

bool FindDependency(const int index, const int dependent, const std::vector<DAGNode>& subpass_to_node, std::unordered_set<uint32_t>& processed_nodes) {
    // If we have already checked this node we have not found a dependency path so return false.
    if (processed_nodes.count(index))
        return false;
    processed_nodes.insert(index);
    const DAGNode& node = subpass_to_node[index];
    // Look for a dependency path. If one exists return true else recurse on the previous nodes.
    if (std::find(node.prev.begin(), node.prev.end(), dependent) == node.prev.end()) {
        for (auto elem : node.prev) {
            if (FindDependency(elem, dependent, subpass_to_node, processed_nodes))
                return true;
        }
    } else {
        return true;
    }
    return false;
}

bool CheckDependencyExists(VkDevice device, const int subpass, const std::vector<uint32_t>& dependent_subpasses, const std::vector<DAGNode>& subpass_to_node, bool& skip_call) {
    bool result = true;
    // Loop through all subpasses that share the same attachment and make sure a dependency exists
    for (uint32_t k = 0; k < dependent_subpasses.size(); ++k) {
        if (subpass == dependent_subpasses[k])
            continue;
        const DAGNode& node = subpass_to_node[subpass];
        // Check for a specified dependency between the two nodes. If one exists we are done.
        auto prev_elem = std::find(node.prev.begin(), node.prev.end(), dependent_subpasses[k]);
        auto next_elem = std::find(node.next.begin(), node.next.end(), dependent_subpasses[k]);
        if (prev_elem == node.prev.end() && next_elem == node.next.end()) {
            // If no dependency exits an implicit dependency still might. If so, warn and if not throw an error.
            std::unordered_set<uint32_t> processed_nodes;
            if (FindDependency(subpass, dependent_subpasses[k], subpass_to_node, processed_nodes) ||
                FindDependency(dependent_subpasses[k], subpass, subpass_to_node, processed_nodes)) {
                skip_call |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but only an implicit one is specified.",
                                     subpass, dependent_subpasses[k]);
            } else {
                skip_call |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but one is not specified.",
                                     subpass, dependent_subpasses[k]);
                result = false;
            }
        }
    }
    return result;
}

bool CheckPreserved(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const int index, const int attachment, const std::vector<DAGNode>& subpass_to_node, int depth, bool& skip_call) {
    const DAGNode& node = subpass_to_node[index];
    // If this node writes to the attachment return true as next nodes need to preserve the attachment.
    const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
    for (uint32_t j = 0; j < subpass.colorCount; ++j) {
        if (attachment == subpass.pColorAttachments[j].attachment)
            return true;
    }
    if (subpass.depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED) {
        if (attachment == subpass.depthStencilAttachment.attachment)
            return true;
    }
    bool result = false;
    // Loop through previous nodes and see if any of them write to the attachment.
    for (auto elem : node.prev) {
        result |= CheckPreserved(device, pCreateInfo, elem, attachment, subpass_to_node, depth + 1, skip_call);
    }
    // If the attachment was written to by a previous node than this node needs to preserve it.
    if (result && depth > 0) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
        bool has_preserved = false;
        for (uint32_t j = 0; j < subpass.preserveCount; ++j) {
            if (subpass.pPreserveAttachments[j].attachment == attachment) {
                has_preserved = true;
                break;
            }
        }
        if (!has_preserved) {
            skip_call |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Attachment %d is used by a later subpass and must be preserved in subpass %d.", attachment, index);
        }
    }
    return result;
}

bool validateDependencies(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo) {
    bool skip_call = false;
    std::vector<DAGNode> subpass_to_node(pCreateInfo->subpassCount);
    std::vector<std::vector<uint32_t>> output_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> input_attachment_to_subpass(pCreateInfo->attachmentCount);
    // Create DAG
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode& subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency& dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass > dependency.destSubpass) {
            skip_call |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Dependency graph must be specified such that an earlier pass cannot depend on a later pass.");
        }
        subpass_to_node[dependency.destSubpass].prev.push_back(dependency.srcSubpass);
        subpass_to_node[dependency.srcSubpass].next.push_back(dependency.destSubpass);
    }
    // Find for each attachment the subpasses that use them.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputCount; ++j) {
            input_attachment_to_subpass[subpass.pInputAttachments[j].attachment].push_back(i);
        }
        for (uint32_t j = 0; j < subpass.colorCount; ++j) {
            output_attachment_to_subpass[subpass.pColorAttachments[j].attachment].push_back(i);
        }
        if (subpass.depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED) {
            output_attachment_to_subpass[subpass.depthStencilAttachment.attachment].push_back(i);
        }
    }
    // If there is a dependency needed make sure one exists
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        // If the attachment is an input then all subpasses that output must have a dependency relationship
        for (uint32_t j = 0; j < subpass.inputCount; ++j) {
            const uint32_t& attachment = subpass.pInputAttachments[j].attachment;
            CheckDependencyExists(device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        // If the attachment is an output then all subpasses that use the attachment must have a dependency relationship
        for (uint32_t j = 0; j < subpass.colorCount; ++j) {
            const uint32_t& attachment = subpass.pColorAttachments[j].attachment;
            CheckDependencyExists(device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        if (subpass.depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED) {
            const uint32_t& attachment = subpass.depthStencilAttachment.attachment;
            CheckDependencyExists(device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
    }
    // Loop through implicit dependencies, if this pass reads make sure the attachment is preserved for all passes after it was written.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputCount; ++j) {
            CheckPreserved(device, pCreateInfo, i, subpass.pInputAttachments[j].attachment, subpass_to_node, 0, skip_call);
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    if (validateDependencies(device, pCreateInfo)) {
        return VK_ERROR_VALIDATION_FAILED;
    }
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pRenderPass);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VkRenderPassCreateInfo* localRPCI = new VkRenderPassCreateInfo(*pCreateInfo);
        if (pCreateInfo->pAttachments) {
            localRPCI->pAttachments = new VkAttachmentDescription[localRPCI->attachmentCount];
            memcpy((void*)localRPCI->pAttachments, pCreateInfo->pAttachments, localRPCI->attachmentCount*sizeof(VkAttachmentDescription));
        }
        if (pCreateInfo->pSubpasses) {
            localRPCI->pSubpasses = new VkSubpassDescription[localRPCI->subpassCount];
            memcpy((void*)localRPCI->pSubpasses, pCreateInfo->pSubpasses, localRPCI->subpassCount*sizeof(VkSubpassDescription));

            for (uint32_t i = 0; i < localRPCI->subpassCount; i++) {
                VkSubpassDescription *subpass = (VkSubpassDescription *) &localRPCI->pSubpasses[i];
                const uint32_t attachmentCount = subpass->inputCount +
                    subpass->colorCount * (1 + (subpass->pResolveAttachments?1:0)) +
                    subpass->preserveCount;
                VkAttachmentReference *attachments = new VkAttachmentReference[attachmentCount];

                memcpy(attachments, subpass->pInputAttachments,
                        sizeof(attachments[0]) * subpass->inputCount);
                subpass->pInputAttachments = attachments;
                attachments += subpass->inputCount;

                memcpy(attachments, subpass->pColorAttachments,
                        sizeof(attachments[0]) * subpass->colorCount);
                subpass->pColorAttachments = attachments;
                attachments += subpass->colorCount;

                if (subpass->pResolveAttachments) {
                    memcpy(attachments, subpass->pResolveAttachments,
                            sizeof(attachments[0]) * subpass->colorCount);
                    subpass->pResolveAttachments = attachments;
                    attachments += subpass->colorCount;
                }

                memcpy(attachments, subpass->pPreserveAttachments,
                        sizeof(attachments[0]) * subpass->preserveCount);
                subpass->pPreserveAttachments = attachments;
            }
        }
        if (pCreateInfo->pDependencies) {
            localRPCI->pDependencies = new VkSubpassDependency[localRPCI->dependencyCount];
            memcpy((void*)localRPCI->pDependencies, pCreateInfo->pDependencies, localRPCI->dependencyCount*sizeof(VkSubpassDependency));
        }
        dev_data->renderPassMap[pRenderPass->handle] = localRPCI;
    }
    return result;
}
// Free the renderpass shadow
static void deleteRenderPasses(layer_data* my_data)
{
    if (my_data->renderPassMap.size() <= 0)
        return;
    for (auto ii=my_data->renderPassMap.begin(); ii!=my_data->renderPassMap.end(); ++ii) {
        if ((*ii).second->pAttachments) {
            delete[] (*ii).second->pAttachments;
        }
        if ((*ii).second->pSubpasses) {
            for (uint32_t i=0; i<(*ii).second->subpassCount; ++i) {
                // Attachements are all allocated in a block, so just need to
                //  find the first non-null one to delete
                if ((*ii).second->pSubpasses[i].pInputAttachments) {
                    delete[] (*ii).second->pSubpasses[i].pInputAttachments;
                } else if ((*ii).second->pSubpasses[i].pColorAttachments) {
                    delete[] (*ii).second->pSubpasses[i].pColorAttachments;
                } else if ((*ii).second->pSubpasses[i].pResolveAttachments) {
                    delete[] (*ii).second->pSubpasses[i].pResolveAttachments;
                } else if ((*ii).second->pSubpasses[i].pPreserveAttachments) {
                    delete[] (*ii).second->pSubpasses[i].pPreserveAttachments;
                }
            }
            delete[] (*ii).second->pSubpasses;
        }
        if ((*ii).second->pDependencies) {
            delete[] (*ii).second->pDependencies;
        }
    }
    my_data->renderPassMap.clear();
}
VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCmdBuffer cmdBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkRenderPassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        if (pRenderPassBegin && pRenderPassBegin->renderPass) {
            skipCall |= insideRenderPass(pCB, "vkCmdBeginRenderPass");
            updateCBTracking(pCB);
            skipCall |= addCmd(pCB, CMD_BEGINRENDERPASS);
            pCB->activeRenderPass = pRenderPassBegin->renderPass;
            pCB->activeSubpass = 0;
            pCB->framebuffer = pRenderPassBegin->framebuffer;
            if (pCB->lastBoundPipeline) {
                skipCall |= validatePipelineState(dev_data, pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
            }
        } else {
            skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                    "You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin, contents);
}

VK_LAYER_EXPORT void VKAPI vkCmdNextSubpass(VkCmdBuffer cmdBuffer, VkRenderPassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        updateCBTracking(pCB);
        skipCall |= addCmd(pCB, CMD_NEXTSUBPASS);
        pCB->activeSubpass++;
        if (pCB->lastBoundPipeline) {
            skipCall |= validatePipelineState(dev_data, pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
        }
        skipCall |= outsideRenderPass(pCB, "vkCmdNextSubpass");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdNextSubpass(cmdBuffer, contents);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VkCmdBuffer cmdBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        skipCall |= outsideRenderPass(pCB, "vkEndRenderpass");
        updateCBTracking(pCB);
        skipCall |= addCmd(pCB, CMD_ENDRENDERPASS);
        pCB->activeRenderPass = 0;
        pCB->activeSubpass = 0;
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndRenderPass(cmdBuffer);
}

VK_LAYER_EXPORT void VKAPI vkCmdExecuteCommands(VkCmdBuffer cmdBuffer, uint32_t cmdBuffersCount, const VkCmdBuffer* pCmdBuffers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB) {
        GLOBAL_CB_NODE* pSubCB = NULL;
        for (uint32_t i=0; i<cmdBuffersCount; i++) {
            pSubCB = getCBNode(dev_data, pCmdBuffers[i]);
            if (!pSubCB) {
                skipCall |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_SECONDARY_CMD_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p in element %u of pCmdBuffers array.", (void*)pCmdBuffers[i], i);
            } else if (VK_CMD_BUFFER_LEVEL_PRIMARY == pSubCB->createInfo.level) {
                skipCall |= log_msg(mdd(pCB->cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_SECONDARY_CMD_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ Primary Cmd Buffer %p in element %u of pCmdBuffers array. All cmd buffers in pCmdBuffers array must be secondary.", (void*)pCmdBuffers[i], i);
            }
        }
        updateCBTracking(pCB);
        skipCall |= addCmd(pCB, CMD_EXECUTECOMMANDS);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdExecuteCommands(cmdBuffer, cmdBuffersCount, pCmdBuffers);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
    VkInstance                          instance,
    VkFlags                             msgFlags,
    const PFN_vkDbgMsgCallback          pfnMsgCallback,
    void*                               pUserData,
    VkDbgMsgCallback*                   pMsgCallback)
{
    layer_data* my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult res = pTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (VK_SUCCESS == res) {
        //layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        res = layer_create_msg_callback(my_data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(
    VkInstance                          instance,
    VkDbgMsgCallback                    msgCallback)
{
    layer_data* my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult res = pTable->DbgDestroyMsgCallback(instance, msgCallback);
    layer_destroy_msg_callback(my_data->report_data, msgCallback);
    return res;
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VkCmdBuffer cmdBuffer, const char* pMarker)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerBegin but extension disabled!");
        return;
    } else if (pCB) {
        updateCBTracking(pCB);
        skipCall |= addCmd(pCB, CMD_DBGMARKERBEGIN);
    }
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(cmdBuffer)->CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VkCmdBuffer cmdBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerEnd but extension disabled!");
        return;
    } else if (pCB) {
        updateCBTracking(pCB);
        skipCall |= addCmd(pCB, CMD_DBGMARKEREND);
    }
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(cmdBuffer)->CmdDbgMarkerEnd(cmdBuffer);
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice dev, const char* funcName)
{
    if (dev == NULL)
        return NULL;

    layer_data *dev_data;
    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        VkBaseLayerObject* wrapped_dev = (VkBaseLayerObject*) dev;
        dev_data = get_my_data_ptr(get_dispatch_key(wrapped_dev->baseObject), layer_data_map);
        dev_data->device_dispatch_table = new VkLayerDispatchTable;
        layer_initialize_dispatch_table(dev_data->device_dispatch_table, wrapped_dev);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }
    dev_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkDestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(funcName, "vkDestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(funcName, "vkDestroyEvent"))
        return (PFN_vkVoidFunction) vkDestroyEvent;
    if (!strcmp(funcName, "vkDestroyQueryPool"))
        return (PFN_vkVoidFunction) vkDestroyQueryPool;
    if (!strcmp(funcName, "vkDestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(funcName, "vkDestroyBufferView"))
        return (PFN_vkVoidFunction) vkDestroyBufferView;
    if (!strcmp(funcName, "vkDestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(funcName, "vkDestroyImageView"))
        return (PFN_vkVoidFunction) vkDestroyImageView;
    if (!strcmp(funcName, "vkDestroyShaderModule"))
        return (PFN_vkVoidFunction) vkDestroyShaderModule;
    if (!strcmp(funcName, "vkDestroyShader"))
        return (PFN_vkVoidFunction) vkDestroyShader;
    if (!strcmp(funcName, "vkDestroyPipeline"))
        return (PFN_vkVoidFunction) vkDestroyPipeline;
    if (!strcmp(funcName, "vkDestroyPipelineLayout"))
        return (PFN_vkVoidFunction) vkDestroyPipelineLayout;
    if (!strcmp(funcName, "vkDestroySampler"))
        return (PFN_vkVoidFunction) vkDestroySampler;
    if (!strcmp(funcName, "vkDestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorSetLayout;
    if (!strcmp(funcName, "vkDestroyDescriptorPool"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorPool;
    if (!strcmp(funcName, "vkDestroyCommandBuffer"))
        return (PFN_vkVoidFunction) vkDestroyCommandBuffer;
    if (!strcmp(funcName, "vkDestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(funcName, "vkDestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "CreatePipelineCache"))
        return (PFN_vkVoidFunction) vkCreatePipelineCache;
    if (!strcmp(funcName, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction) vkDestroyPipelineCache;
    if (!strcmp(funcName, "GetPipelineCacheSize"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheSize;
    if (!strcmp(funcName, "GetPipelineCacheData"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheData;
    if (!strcmp(funcName, "MergePipelineCaches"))
        return (PFN_vkVoidFunction) vkMergePipelineCaches;
    if (!strcmp(funcName, "vkCreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocDescriptorSets;
    if (!strcmp(funcName, "vkFreeDescriptorSets"))
        return (PFN_vkVoidFunction) vkFreeDescriptorSets;
    if (!strcmp(funcName, "vkUpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(funcName, "vkCreateCommandBuffer"))
        return (PFN_vkVoidFunction) vkCreateCommandBuffer;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdSetViewport"))
        return (PFN_vkVoidFunction) vkCmdSetViewport;
    if (!strcmp(funcName, "vkCmdSetScissor"))
        return (PFN_vkVoidFunction) vkCmdSetScissor;
    if (!strcmp(funcName, "vkCmdSetLineWidth"))
        return (PFN_vkVoidFunction) vkCmdSetLineWidth;
    if (!strcmp(funcName, "vkCmdSetDepthBias"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBias;
    if (!strcmp(funcName, "vkCmdSetBlendConstants"))
        return (PFN_vkVoidFunction) vkCmdSetBlendConstants;
    if (!strcmp(funcName, "vkCmdSetDepthBounds"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBounds;
    if (!strcmp(funcName, "vkCmdSetStencilCompareMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilCompareMask;
    if (!strcmp(funcName, "vkCmdSetStencilWriteMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilWriteMask;
    if (!strcmp(funcName, "vkCmdSetStencilReference"))
        return (PFN_vkVoidFunction) vkCmdSetStencilReference;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (PFN_vkVoidFunction) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (PFN_vkVoidFunction) vkCmdBindVertexBuffers;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (PFN_vkVoidFunction) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDraw"))
        return (PFN_vkVoidFunction) vkCmdDraw;
    if (!strcmp(funcName, "vkCmdDrawIndexed"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexed;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatch"))
        return (PFN_vkVoidFunction) vkCmdDispatch;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (PFN_vkVoidFunction) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (PFN_vkVoidFunction) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (PFN_vkVoidFunction) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(funcName, "vkCmdClearColorAttachment"))
        return (PFN_vkVoidFunction) vkCmdClearColorAttachment;
    if (!strcmp(funcName, "vkCmdClearDepthStencilAttachment"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilAttachment;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdSetEvent"))
        return (PFN_vkVoidFunction) vkCmdSetEvent;
    if (!strcmp(funcName, "vkCmdResetEvent"))
        return (PFN_vkVoidFunction) vkCmdResetEvent;
    if (!strcmp(funcName, "vkCmdWaitEvents"))
        return (PFN_vkVoidFunction) vkCmdWaitEvents;
    if (!strcmp(funcName, "vkCmdPipelineBarrier"))
        return (PFN_vkVoidFunction) vkCmdPipelineBarrier;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkCmdWriteTimestamp"))
        return (PFN_vkVoidFunction) vkCmdWriteTimestamp;
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;
    if (!strcmp(funcName, "vkCmdEndRenderPass"))
        return (PFN_vkVoidFunction) vkCmdEndRenderPass;
    if (!strcmp(funcName, "vkCmdExecuteCommands"))
        return (PFN_vkVoidFunction) vkCmdExecuteCommands;

    VkLayerDispatchTable* pTable = dev_data->device_dispatch_table;
    if (dev_data->device_extensions.debug_marker_enabled)
    {
        if (!strcmp(funcName, "vkCmdDbgMarkerBegin"))
            return (PFN_vkVoidFunction) vkCmdDbgMarkerBegin;
        if (!strcmp(funcName, "vkCmdDbgMarkerEnd"))
            return (PFN_vkVoidFunction) vkCmdDbgMarkerEnd;
    }
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(dev, funcName);
    }
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction fptr;
    if (instance == NULL)
        return NULL;

    layer_data* my_data;
    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        VkBaseLayerObject* wrapped_inst = (VkBaseLayerObject*) instance;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_inst->baseObject), layer_data_map);
        my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
        layer_init_instance_dispatch_table(my_data->instance_dispatch_table, wrapped_inst);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }
    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;

    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr)
        return fptr;

    {
        VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(instance, funcName);
    }
}
