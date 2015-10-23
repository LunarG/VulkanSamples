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
#include "vk_layer_utils.h"

struct devExts {
    VkBool32 debug_marker_enabled;
    VkBool32 wsi_enabled;
    unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE*> swapchainMap;
};

struct layer_data {
    debug_report_data *report_data;
    // TODO: put instance data here
    std::vector<VkDbgMsgCallback> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    devExts device_extensions;
    // Layer specific data
    unordered_map<VkSampler, unique_ptr<SAMPLER_NODE>> sampleMap;
    unordered_map<VkImageView, unique_ptr<VkImageViewCreateInfo>> imageViewMap;
    unordered_map<VkImage, unique_ptr<VkImageCreateInfo>> imageMap;
    unordered_map<VkBufferView, unique_ptr<VkBufferViewCreateInfo>> bufferViewMap;
    unordered_map<VkBuffer, unique_ptr<VkBufferCreateInfo>> bufferMap;
    unordered_map<VkPipeline, PIPELINE_NODE*> pipelineMap;
    unordered_map<VkDescriptorPool, POOL_NODE*> poolMap;
    unordered_map<VkDescriptorSet, SET_NODE*> setMap;
    unordered_map<VkDescriptorSetLayout, LAYOUT_NODE*> layoutMap;
    unordered_map<VkPipelineLayout, PIPELINE_LAYOUT_NODE> pipelineLayoutMap;
    unordered_map<VkDeviceMemory, VkImage> memImageMap;
    // Map for layout chains
    unordered_map<void*, GLOBAL_CB_NODE*> commandBufferMap;
    unordered_map<VkFramebuffer, VkFramebufferCreateInfo*> frameBufferMap;
    unordered_map<VkImage, IMAGE_NODE*> imageLayoutMap;
    unordered_map<VkRenderPass, RENDER_PASS_NODE*> renderPassMap;
    // Current render pass
    VkRenderPassBeginInfo renderPassBeginInfo;
    uint32_t currentSubpass;

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
        case CMD_CLEARATTACHMENTS:
            return "CMD_CLEARCOLORATTACHMENT";
        case CMD_CLEARDEPTHSTENCILIMAGE:
            return "CMD_CLEARDEPTHSTENCILIMAGE";
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
#define MAX_SLOTS 2048
#define NUM_COMMAND_BUFFERS_TO_DISPLAY 10

static uint64_t g_drawCount[NUM_DRAW_TYPES] = {0, 0, 0, 0};

// TODO : Should be tracking lastBound per commandBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
// Track the last cmd buffer touched by this thread
static VkCommandBuffer    g_lastCommandBuffer[MAX_TID] = {NULL};
// Track the last group of CBs touched for displaying to dot file
static GLOBAL_CB_NODE* g_pLastTouchedCB[NUM_COMMAND_BUFFERS_TO_DISPLAY] = {NULL};
static uint32_t        g_lastTouchedCBIndex = 0;
// Track the last global DrawState of interest touched by any thread
static GLOBAL_CB_NODE* g_lastGlobalCB = NULL;
static PIPELINE_NODE*  g_lastBoundPipeline = NULL;
#define MAX_BINDING 0xFFFFFFFF // Default vtxBinding value in CB Node to identify if no vtxBinding set
// prototype
static GLOBAL_CB_NODE* getCBNode(layer_data*, const VkCommandBuffer);
// Update global ptrs to reflect that specified commandBuffer has been used
static void updateCBTracking(GLOBAL_CB_NODE* pCB)
{
    g_lastCommandBuffer[getTIDIndex()] = pCB->commandBuffer;
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
static VkBool32 validate_status(layer_data* my_data, GLOBAL_CB_NODE* pNode, CBStatusFlags enable_mask, CBStatusFlags status_mask, CBStatusFlags status_flag, VkFlags msg_flags, DRAW_STATE_ERROR error_code, const char* fail_msg)
{
    // If non-zero enable mask is present, check it against status but if enable_mask
    //  is 0 then no enable required so we should always just check status
    if ((!enable_mask) || (enable_mask & pNode->status)) {
        if ((pNode->status & status_mask) != status_flag) {
            // TODO : How to pass dispatchable objects as srcObject? Here src obj should be cmd buffer
            return log_msg(my_data->report_data, msg_flags, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, error_code, "DS",
                    "CB object %#" PRIxLEAST64 ": %s", reinterpret_cast<uint64_t>(pNode->commandBuffer), fail_msg);
        }
    }
    return VK_FALSE;
}
// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_NODE* getPipeline(layer_data* my_data, const VkPipeline pipeline)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->pipelineMap.find(pipeline) == my_data->pipelineMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->pipelineMap[pipeline];
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
static VkBool32 validate_draw_state_flags(layer_data* my_data, GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    VkBool32 result;
    result = validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_VIEWPORT_SET, CBSTATUS_VIEWPORT_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_VIEWPORT_NOT_BOUND, "Dynamic viewport state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_SCISSOR_SET, CBSTATUS_SCISSOR_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_SCISSOR_NOT_BOUND, "Dynamic scissor state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_LINE_WIDTH_SET, CBSTATUS_LINE_WIDTH_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_LINE_WIDTH_NOT_BOUND, "Dynamic line width state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_DEPTH_BIAS_SET, CBSTATUS_DEPTH_BIAS_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_DEPTH_BIAS_NOT_BOUND, "Dynamic depth bias state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_COLOR_BLEND_WRITE_ENABLE, CBSTATUS_BLEND_SET, CBSTATUS_BLEND_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_BLEND_NOT_BOUND, "Dynamic blend object state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_DEPTH_WRITE_ENABLE, CBSTATUS_DEPTH_BOUNDS_SET, CBSTATUS_DEPTH_BOUNDS_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND, "Dynamic depth bounds state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_READ_MASK_SET, CBSTATUS_STENCIL_READ_MASK_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil read mask state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_WRITE_MASK_SET, CBSTATUS_STENCIL_WRITE_MASK_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil write mask state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_REFERENCE_SET, CBSTATUS_STENCIL_REFERENCE_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil reference state not set for this command buffer");
    if (indexedDraw)
        result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_INDEX_BUFFER_BOUND, CBSTATUS_INDEX_BUFFER_BOUND, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_INDEX_BUFFER_NOT_BOUND, "Index buffer object not bound to this command buffer when Indexed Draw attempted");
    return result;
}
// Validate overall state at the time of a draw call
static VkBool32 validate_draw_state(layer_data* my_data, GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    // First check flag states
    VkBool32 result = validate_draw_state_flags(my_data, pCB, indexedDraw);
    PIPELINE_NODE* pPipe = getPipeline(my_data, pCB->lastBoundPipeline);
    // Now complete other state checks
    // TODO : Currently only performing next check if *something* was bound (non-zero last bound)
    //  There is probably a better way to gate when this check happens, and to know if something *should* have been bound
    //  We should have that check separately and then gate this check based on that check
    if (pPipe && (pCB->lastBoundPipelineLayout) && (pCB->lastBoundPipelineLayout != pPipe->graphicsPipelineCI.layout)) {
        result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t) pCB->lastBoundPipelineLayout, 0, DRAWSTATE_PIPELINE_LAYOUT_MISMATCH, "DS",
                "Pipeline layout from last vkCmdBindDescriptorSets() (%#" PRIxLEAST64 ") does not match PSO Pipeline layout (%#" PRIxLEAST64 ") ", (uint64_t) pCB->lastBoundPipelineLayout, (uint64_t) pPipe->graphicsPipelineCI.layout);
    }
    // Verify Vtx binding
    if (MAX_BINDING != pCB->lastVtxBinding) {
        if (pCB->lastVtxBinding >= pPipe->vtxBindingCount) {
            if (0 == pPipe->vtxBindingCount) {
                result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                        "Vtx Buffer Index %u was bound, but no vtx buffers are attached to PSO.", pCB->lastVtxBinding);
            }
            else {
                result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                        "Vtx binding Index of %u exceeds PSO pVertexBindingDescriptions max array index of %u.", pCB->lastVtxBinding, (pPipe->vtxBindingCount - 1));
            }
        }
    }
    // If Viewport or scissors are dynamic, verify that dynamic count matches PSO count
    VkBool32 dynViewport = isDynamic(pPipe, VK_DYNAMIC_STATE_VIEWPORT);
    VkBool32 dynScissor = isDynamic(pPipe, VK_DYNAMIC_STATE_SCISSOR);
    if (dynViewport) {
        if (pCB->viewports.size() != pPipe->graphicsPipelineCI.pViewportState->viewportCount) {
            result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Dynamic viewportCount from vkCmdSetViewport() is %u, but PSO viewportCount is %u. These counts must match.", pCB->viewports.size(), pPipe->graphicsPipelineCI.pViewportState->viewportCount);
        }
    }
    if (dynScissor) {
        if (pCB->scissors.size() != pPipe->graphicsPipelineCI.pViewportState->scissorCount) {
            result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Dynamic scissorCount from vkCmdSetScissor() is %u, but PSO scissorCount is %u. These counts must match.", pCB->scissors.size(), pPipe->graphicsPipelineCI.pViewportState->scissorCount);
        }
    }
    return result;
}
// Verify that create state for a pipeline is valid
static VkBool32 verifyPipelineCreateState(layer_data* my_data, const VkDevice device, const PIPELINE_NODE* pPipeline)
{
    VkBool32 skipCall = VK_FALSE;
    // VS is required
    if (!(pPipeline->active_shaders & VK_SHADER_STAGE_VERTEX_BIT)) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Vtx Shader required");
    }
    // Either both or neither TC/TE shaders should be defined
    if (((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) == 0) !=
         ((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) == 0) ) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: TE and TC shaders must be included or excluded as a pair");
    }
    // Compute shaders should be specified independent of Gfx shaders
    if ((pPipeline->active_shaders & VK_SHADER_STAGE_COMPUTE_BIT) &&
        (pPipeline->active_shaders & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
                                     VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT |
                                     VK_SHADER_STAGE_FRAGMENT_BIT))) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Do not specify Compute Shader for Gfx Pipeline");
    }
    // VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines.
    // Mismatching primitive topology and tessellation fails graphics pipeline creation.
    if (pPipeline->active_shaders & (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) &&
        (pPipeline->iaStateCI.topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST must be set as IA topology for tessellation pipelines");
    }
    if (pPipeline->iaStateCI.topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) {
        if (~pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines");
        }
        if (!pPipeline->tessStateCI.patchControlPoints || (pPipeline->tessStateCI.patchControlPoints > 32)) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                    "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology used with patchControlPoints value %u."
                    " patchControlPoints should be >0 and <=32.", pPipeline->tessStateCI.patchControlPoints);
        }
    }
    // Viewport state must be included and viewport and scissor counts should always match
    // NOTE : Even if these are flagged as dynamic, counts need to be set correctly for shader compiler
    if (!pPipeline->graphicsPipelineCI.pViewportState) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
            "Gfx Pipeline pViewportState is null. Even if viewport and scissors are dynamic PSO must include viewportCount and scissorCount in pViewportState.");
    } else if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount != pPipeline->graphicsPipelineCI.pViewportState->viewportCount) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Gfx Pipeline viewport count (%u) must match scissor count (%u).", pPipeline->vpStateCI.viewportCount, pPipeline->vpStateCI.scissorCount);
    } else {
        // If viewport or scissor are not dynamic, then verify that data is appropriate for count
        VkBool32 dynViewport = isDynamic(pPipeline, VK_DYNAMIC_STATE_VIEWPORT);
        VkBool32 dynScissor = isDynamic(pPipeline, VK_DYNAMIC_STATE_SCISSOR);
        if (!dynViewport) {
            if (pPipeline->graphicsPipelineCI.pViewportState->viewportCount && !pPipeline->graphicsPipelineCI.pViewportState->pViewports) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline viewportCount is %u, but pViewports is NULL. For non-zero viewportCount, you must either include pViewports data, or include viewport in pDynamicState and set it with vkCmdSetViewport().", pPipeline->graphicsPipelineCI.pViewportState->viewportCount);
            }
        }
        if (!dynScissor) {
            if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount && !pPipeline->graphicsPipelineCI.pViewportState->pScissors) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline scissorCount is %u, but pScissors is NULL. For non-zero scissorCount, you must either include pScissors data, or include scissor in pDynamicState and set it with vkCmdSetScissor().", pPipeline->graphicsPipelineCI.pViewportState->scissorCount);
            }
        }
    }
    return skipCall;
}
// Init the pipeline mapping info based on pipeline create info LL tree
//  Threading note : Calls to this function should wrapped in mutex
static PIPELINE_NODE* initPipeline(layer_data* dev_data, const VkGraphicsPipelineCreateInfo* pCreateInfo, PIPELINE_NODE* pBasePipeline)
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
            case VK_SHADER_STAGE_VERTEX_BIT:
                memcpy(&pPipeline->vsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                memcpy(&pPipeline->tcsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                memcpy(&pPipeline->tesCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                memcpy(&pPipeline->gsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                memcpy(&pPipeline->fsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
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
        pPipeline->vtxBindingCount = pVICI->vertexBindingDescriptionCount;
        if (pPipeline->vtxBindingCount) {
            pPipeline->pVertexBindingDescriptions = new VkVertexInputBindingDescription[pPipeline->vtxBindingCount];
            bufferSize = pPipeline->vtxBindingCount * sizeof(VkVertexInputBindingDescription);
            memcpy((void*)pPipeline->pVertexBindingDescriptions, pVICI->pVertexBindingDescriptions, bufferSize);
        }
        pPipeline->vtxAttributeCount = pVICI->vertexAttributeDescriptionCount;
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
    if (pCreateInfo->pRasterizationState != NULL) {
        memcpy((void*)&pPipeline->rsStateCI, pCreateInfo->pRasterizationState, sizeof(VkPipelineRasterizationStateCreateInfo));
        pPipeline->graphicsPipelineCI.pRasterizationState = &pPipeline->rsStateCI;
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
static VkSampleCountFlagBits getNumSamples(layer_data* my_data, const VkPipeline pipeline)
{
    PIPELINE_NODE* pPipe = my_data->pipelineMap[pipeline];
    if (VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO == pPipe->msStateCI.sType) {
        return pPipe->msStateCI.rasterizationSamples;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}
// Validate state related to the PSO
static VkBool32 validatePipelineState(layer_data* my_data, const GLOBAL_CB_NODE* pCB, const VkPipelineBindPoint pipelineBindPoint, const VkPipeline pipeline)
{
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
        // Verify that any MSAA request in PSO matches sample# in bound FB
        VkSampleCountFlagBits psoNumSamples = getNumSamples(my_data, pipeline);
        if (pCB->activeRenderPass) {
            const VkRenderPassCreateInfo* pRPCI = my_data->renderPassMap[pCB->activeRenderPass]->createInfo;
            const VkSubpassDescription* pSD = &pRPCI->pSubpasses[pCB->activeSubpass];
            VkSampleCountFlagBits subpassNumSamples = (VkSampleCountFlagBits) 0;
            uint32_t i;

            for (i = 0; i < pSD->colorAttachmentCount; i++) {
                VkSampleCountFlagBits samples;

                if (pSD->pColorAttachments[i].attachment == VK_ATTACHMENT_UNUSED)
                    continue;

                samples = pRPCI->pAttachments[pSD->pColorAttachments[i].attachment].samples;
                if (subpassNumSamples == (VkSampleCountFlagBits) 0) {
                    subpassNumSamples = samples;
                } else if (subpassNumSamples != samples) {
                    subpassNumSamples = (VkSampleCountFlagBits) -1;
                    break;
                }
            }
            if (pSD->pDepthStencilAttachment && pSD->pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                const VkSampleCountFlagBits samples = pRPCI->pAttachments[pSD->pDepthStencilAttachment->attachment].samples;
                if (subpassNumSamples == (VkSampleCountFlagBits) 0)
                    subpassNumSamples = samples;
                else if (subpassNumSamples != samples)
                    subpassNumSamples = (VkSampleCountFlagBits) -1;
            }

            if (psoNumSamples != subpassNumSamples) {
                return log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline, 0, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                        "Num samples mismatch! Binding PSO (%#" PRIxLEAST64 ") with %u samples while current RenderPass (%#" PRIxLEAST64 ") w/ %u samples!",
                        (uint64_t) pipeline, psoNumSamples, (uint64_t) pCB->activeRenderPass, subpassNumSamples);
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
    if (my_data->poolMap.find(pool) == my_data->poolMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->poolMap[pool];
}
// Return Set node ptr for specified set or else NULL
static SET_NODE* getSetNode(layer_data* my_data, const VkDescriptorSet set)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->setMap.find(set) == my_data->setMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->setMap[set];
}

static LAYOUT_NODE* getLayoutNode(layer_data* my_data, const VkDescriptorSetLayout layout) {
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->layoutMap.find(layout) == my_data->layoutMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->layoutMap[layout];
}
// Return VK_FALSE if update struct is of valid type, otherwise flag error and return code from callback
static VkBool32 validUpdateStruct(layer_data* my_data, const VkDevice device, const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            return VK_FALSE;
        default:
            return log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
}
// Set count for given update struct in the last parameter
// Return value of skipCall, which is only VK_TRUE is error occurs and callback signals execution to cease
static uint32_t getUpdateCount(layer_data* my_data, const VkDevice device, const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            return ((VkWriteDescriptorSet*)pUpdateStruct)->descriptorCount;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            // TODO : Need to understand this case better and make sure code is correct
            return ((VkCopyDescriptorSet*)pUpdateStruct)->descriptorCount;
    }
}
// For given Layout Node and binding, return index where that binding begins
static uint32_t getBindingStartIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i < pLayout->createInfo.bindingCount; i++) {
        if (pLayout->createInfo.pBinding[i].binding == binding)
            break;
        offsetIndex += pLayout->createInfo.pBinding[i].arraySize;
    }
    return offsetIndex;
}
// For given layout node and binding, return last index that is updated
static uint32_t getBindingEndIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i <  pLayout->createInfo.bindingCount; i++) {
        offsetIndex += pLayout->createInfo.pBinding[i].arraySize;
        if (pLayout->createInfo.pBinding[i].binding == binding)
            break;
    }
    return offsetIndex-1;
}
// For given layout and update, return the first overall index of the layout that is updated
static uint32_t getUpdateStartIndex(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const uint32_t binding, const uint32_t arrayIndex, const GENERIC_HEADER* pUpdateStruct)
{
    return getBindingStartIndex(pLayout, binding)+arrayIndex;
}
// For given layout and update, return the last overall index of the layout that is updated
static uint32_t getUpdateEndIndex(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const uint32_t binding, const uint32_t arrayIndex, const GENERIC_HEADER* pUpdateStruct)
{
    uint32_t count = getUpdateCount(my_data, device, pUpdateStruct);
    return getBindingStartIndex(pLayout, binding)+arrayIndex+count-1;
}
// Verify that the descriptor type in the update struct matches what's expected by the layout
static VkBool32 validateUpdateConsistency(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct, uint32_t startIndex, uint32_t endIndex)
{
    // First get actual type of update
    VkBool32 skipCall = VK_FALSE;
    VkDescriptorType actualType;
    uint32_t i = 0;
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
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
    if (VK_FALSE == skipCall) {
        // Set first stageFlags as reference and verify that all other updates match it
        VkShaderStageFlags refStageFlags = pLayout->stageFlags[startIndex];
        for (i = startIndex; i <= endIndex; i++) {
            if (pLayout->descriptorTypes[i] != actualType) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                    "Write descriptor update has descriptor type %s that does not match overlapping binding descriptor type of %s!",
                    string_VkDescriptorType(actualType), string_VkDescriptorType(pLayout->descriptorTypes[i]));
            }
            if (pLayout->stageFlags[i] != refStageFlags) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_STAGEFLAGS_MISMATCH, "DS",
                    "Write descriptor update has stageFlags %x that do not match overlapping binding descriptor stageFlags of %x!",
                    refStageFlags, pLayout->stageFlags[i]);
            }
        }
    }
    return skipCall;
}
// Determine the update type, allocate a new struct of that type, shadow the given pUpdate
//   struct into the pNewNode param. Return VK_TRUE if error condition encountered and callback signals early exit.
// NOTE : Calls to this function should be wrapped in mutex
static VkBool32 shadowUpdateNode(layer_data* my_data, const VkDevice device, GENERIC_HEADER* pUpdate, GENERIC_HEADER** pNewNode)
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

            switch (pWDS->descriptorType) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    VkDescriptorImageInfo *info = new VkDescriptorImageInfo[pWDS->descriptorCount];
                    memcpy(info, pWDS->pImageInfo, pWDS->descriptorCount * sizeof(VkDescriptorImageInfo));
                    pWDS->pImageInfo = info;
            }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                {
                    VkBufferView *info = new VkBufferView[pWDS->descriptorCount];
                    memcpy(info, pWDS->pTexelBufferView, pWDS->descriptorCount * sizeof(VkBufferView));
                    pWDS->pTexelBufferView = info;
                }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                {
                    VkDescriptorBufferInfo *info = new VkDescriptorBufferInfo[pWDS->descriptorCount];
                    memcpy(info, pWDS->pBufferInfo, pWDS->descriptorCount * sizeof(VkDescriptorBufferInfo));
                    pWDS->pBufferInfo = info;
                }
                break;
            default:
                return VK_ERROR_VALIDATION_FAILED;
                break;
            }
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            pCDS = new VkCopyDescriptorSet;
            *pNewNode = (GENERIC_HEADER*)pCDS;
            memcpy(pCDS, pUpdate, sizeof(VkCopyDescriptorSet));
            break;
        default:
            if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdate->sType), pUpdate->sType))
                return VK_TRUE;
    }
    // Make sure that pNext for the end of shadow copy is NULL
    (*pNewNode)->pNext = NULL;
    return skipCall;
}
// Verify that given sampler is valid
static VkBool32 validateSampler(const layer_data* my_data, const VkSampler* pSampler, const VkBool32 immutable)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->sampleMap.find(*pSampler);
    if (sampIt == my_data->sampleMap.end()) {
        if (!immutable) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid sampler %#" PRIxLEAST64, (uint64_t) *pSampler);
        } else { // immutable
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor whose binding has an invalid immutable sampler %#" PRIxLEAST64, (uint64_t) *pSampler);
        }
    } else {
        // TODO : Any further checks we want to do on the sampler?
    }
    return skipCall;
}
// Verify that given imageView is valid
static VkBool32 validateImageView(const layer_data* my_data, const VkImageView* pImageView, const VkImageLayout imageLayout)
{
    VkBool32 skipCall = VK_FALSE;
    auto ivIt = my_data->imageViewMap.find(*pImageView);
    if (ivIt == my_data->imageViewMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0, DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid imageView %#" PRIxLEAST64, (uint64_t) *pImageView);
    } else {
        // Validate that imageLayout is compatible with aspectMask and image format
        VkImageAspectFlags aspectMask = ivIt->second->subresourceRange.aspectMask;
        VkImage image = ivIt->second->image;
        // TODO : Check here in case we have a bad image
        auto imgIt = my_data->imageMap.find(image);
        if (imgIt == my_data->imageMap.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE, (uint64_t) image, 0, DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid image %#" PRIxLEAST64 " in imageView %#" PRIxLEAST64, (uint64_t) image, (uint64_t) *pImageView);
        } else {
            VkFormat format = (*imgIt).second->format;
            VkBool32 ds = vk_format_is_depth_or_stencil(format);
            switch (imageLayout) {
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    // Only Color bit must be set
                    if ((aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL and imageView %#" PRIxLEAST64 ""
                                    " that does not have VK_IMAGE_ASPECT_COLOR_BIT set.", (uint64_t) *pImageView);
                    }
                    // format must NOT be DS
                    if (ds) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL and imageView %#" PRIxLEAST64 ""
                                    " but the image format is %s which is not a color format.", (uint64_t) *pImageView, string_VkFormat(format));
                    }
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    // Depth or stencil bit must be set, but both must NOT be set
                    if (aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
                        if (aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
                            // both  must NOT be set
                            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with imageView %#" PRIxLEAST64 ""
                                    " that has both STENCIL and DEPTH aspects set", (uint64_t) *pImageView);
                        }
                    } else if (!(aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                        // Neither were set
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout %s and imageView %#" PRIxLEAST64 ""
                                    " that does not have STENCIL or DEPTH aspect set.", string_VkImageLayout(imageLayout), (uint64_t) *pImageView);
                    }
                    // format must be DS
                    if (!ds) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout %s and imageView %#" PRIxLEAST64 ""
                                    " but the image format is %s which is not a depth/stencil format.", string_VkImageLayout(imageLayout), (uint64_t) *pImageView, string_VkFormat(format));
                    }
                    break;
                default:
                    // anything to check for other layouts?
                    break;
            }
        }
    }
    return skipCall;
}
// Verify that given bufferView is valid
static VkBool32 validateBufferView(const layer_data* my_data, const VkBufferView* pBufferView)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->bufferViewMap.find(*pBufferView);
    if (sampIt == my_data->bufferViewMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_BUFFER_VIEW, (uint64_t) *pBufferView, 0, DRAWSTATE_BUFFERVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid bufferView %#" PRIxLEAST64, (uint64_t) *pBufferView);
    } else {
        // TODO : Any further checks we want to do on the bufferView?
    }
    return skipCall;
}
// Verify that given bufferInfo is valid
static VkBool32 validateBufferInfo(const layer_data* my_data, const VkDescriptorBufferInfo* pBufferInfo)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->bufferMap.find(pBufferInfo->buffer);
    if (sampIt == my_data->bufferMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_BUFFER, (uint64_t) pBufferInfo->buffer, 0, DRAWSTATE_BUFFERINFO_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor where bufferInfo has invalid buffer %#" PRIxLEAST64, (uint64_t) pBufferInfo->buffer);
    } else {
        // TODO : Any further checks we want to do on the bufferView?
    }
    return skipCall;
}
static VkBool32 validateUpdateContents(const layer_data* my_data, const VkWriteDescriptorSet *pWDS, const VkDescriptorSetLayoutBinding* pLayoutBinding)
{
    VkBool32 skipCall = VK_FALSE;
    // First verify that for the given Descriptor type, the correct DescriptorInfo data is supplied
    VkBufferView* pBufferView = NULL;
    const VkSampler* pSampler = NULL;
    VkImageView* pImageView = NULL;
    VkImageLayout* pImageLayout = NULL;
    VkDescriptorBufferInfo* pBufferInfo = NULL;
    VkBool32 immutable = VK_FALSE;
    uint32_t i = 0;
    // For given update type, verify that update contents are correct
    switch (pWDS->descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateSampler(my_data, &(pWDS->pImageInfo[i].sampler), immutable);
            }
            break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                if (NULL == pLayoutBinding->pImmutableSamplers) {
                    pSampler = &(pWDS->pImageInfo[i].sampler);
                    if (immutable) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, "DS",
                            "vkUpdateDescriptorSets: Update #%u is not an immutable sampler %#" PRIxLEAST64 ", but previous update(s) from this "
                            "VkWriteDescriptorSet struct used an immutable sampler. All updates from a single struct must either "
                            "use immutable or non-immutable samplers.", i, (uint64_t) *pSampler);
                    }
                } else {
                    if (i>0 && !immutable) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, "DS",
                            "vkUpdateDescriptorSets: Update #%u is an immutable sampler, but previous update(s) from this "
                            "VkWriteDescriptorSet struct used a non-immutable sampler. All updates from a single struct must either "
                            "use immutable or non-immutable samplers.", i);
                    }
                    immutable = VK_TRUE;
                    pSampler = &(pLayoutBinding->pImmutableSamplers[i]);
                }
                skipCall |= validateSampler(my_data, pSampler, immutable);
            }
            // Intentionally fall through here to also validate image stuff
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateImageView(my_data, &(pWDS->pImageInfo[i].imageView), pWDS->pImageInfo[i].imageLayout);
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateBufferView(my_data, &(pWDS->pTexelBufferView[i]));
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateBufferInfo(my_data, &(pWDS->pBufferInfo[i]));
            }
            break;
    }
    return skipCall;
}
// update DS mappings based on write and copy update arrays
static VkBool32 dsUpdate(layer_data* my_data, VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pWDS, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pCDS)
{
    VkBool32 skipCall = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);
    LAYOUT_NODE* pLayout = NULL;
    VkDescriptorSetLayoutCreateInfo* pLayoutCI = NULL;
    // Validate Write updates
    uint32_t i = 0;
    for (i=0; i < descriptorWriteCount; i++) {
        VkDescriptorSet ds = pWDS[i].dstSet;
        SET_NODE* pSet = my_data->setMap[ds];
        GENERIC_HEADER* pUpdate = (GENERIC_HEADER*) &pWDS[i];
        pLayout = pSet->pLayout;
        // First verify valid update struct
        if ((skipCall = validUpdateStruct(my_data, device, pUpdate)) == VK_TRUE) {
            break;
        }
        uint32_t binding = 0, endIndex = 0;
        binding = pWDS[i].dstBinding;
        // Make sure that layout being updated has the binding being updated
        if (pLayout->bindings.find(binding) == pLayout->bindings.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) ds, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                    "Descriptor Set %p does not have binding to match update binding %u for update type %s!", ds, binding, string_VkStructureType(pUpdate->sType));
        } else {
            // Next verify that update falls within size of given binding
            endIndex = getUpdateEndIndex(my_data, device, pLayout, binding, pWDS[i].dstArrayElement, pUpdate);
            if (getBindingEndIndex(pLayout, binding) < endIndex) {
                pLayoutCI = &pLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) ds, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                        "Descriptor update type of %s is out of bounds for matching binding %u in Layout w/ CI:\n%s!", string_VkStructureType(pUpdate->sType), binding, DSstr.c_str());
            } else { // TODO : should we skip update on a type mismatch or force it?
                uint32_t startIndex;
                startIndex = getUpdateStartIndex(my_data, device, pLayout, binding, pWDS[i].dstArrayElement, pUpdate);
                // Layout bindings match w/ update, now verify that update type & stageFlags are the same for entire update
                if ((skipCall = validateUpdateConsistency(my_data, device, pLayout, pUpdate, startIndex, endIndex)) == VK_FALSE) {
                    // The update is within bounds and consistent, but need to make sure contents make sense as well
                    if ((skipCall = validateUpdateContents(my_data, &pWDS[i], &pLayout->createInfo.pBinding[binding])) == VK_FALSE) {
                        // Update is good. Save the update info
                        // Create new update struct for this set's shadow copy
                        GENERIC_HEADER* pNewNode = NULL;
                        skipCall |= shadowUpdateNode(my_data, device, pUpdate, &pNewNode);
                        if (NULL == pNewNode) {
                            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) ds, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                                    "Out of memory while attempting to allocate UPDATE struct in vkUpdateDescriptors()");
                        } else {
                            // Insert shadow node into LL of updates for this set
                            pNewNode->pNext = pSet->pUpdateStructs;
                            pSet->pUpdateStructs = pNewNode;
                            // Now update appropriate descriptor(s) to point to new Update node
                            for (uint32_t j = startIndex; j <= endIndex; j++) {
                                assert(j<pSet->descriptorCount);
                                pSet->ppDescriptors[j] = pNewNode;
                            }
                        }
                    }
                }
            }
        }
    }
    // Now validate copy updates
    for (i=0; i < descriptorCopyCount; ++i) {
        SET_NODE *pSrcSet = NULL, *pDstSet = NULL;
        LAYOUT_NODE *pSrcLayout = NULL, *pDstLayout = NULL;
        uint32_t srcStartIndex = 0, srcEndIndex = 0, dstStartIndex = 0, dstEndIndex = 0;
        // For each copy make sure that update falls within given layout and that types match
        pSrcSet = my_data->setMap[pCDS[i].srcSet];
        pDstSet = my_data->setMap[pCDS[i].dstSet];
        pSrcLayout = pSrcSet->pLayout;
        pDstLayout = pDstSet->pLayout;
        // Validate that src binding is valid for src set layout
        if (pSrcLayout->bindings.find(pCDS[i].srcBinding) == pSrcLayout->bindings.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pSrcSet->set, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                "Copy descriptor update %u has srcBinding %u which is out of bounds for underlying SetLayout %#" PRIxLEAST64 " which only has bindings 0-%u.",
                i, pCDS[i].srcBinding, (uint64_t) pSrcLayout->layout, pSrcLayout->createInfo.bindingCount-1);
        } else if (pDstLayout->bindings.find(pCDS[i].dstBinding) == pDstLayout->bindings.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDstSet->set, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                "Copy descriptor update %u has dstBinding %u which is out of bounds for underlying SetLayout %#" PRIxLEAST64 " which only has bindings 0-%u.",
                i, pCDS[i].dstBinding, (uint64_t) pDstLayout->layout, pDstLayout->createInfo.bindingCount-1);
        } else {
            // Proceed with validation. Bindings are ok, but make sure update is within bounds of given layout
            srcEndIndex = getUpdateEndIndex(my_data, device, pSrcLayout, pCDS[i].srcBinding, pCDS[i].srcArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
            dstEndIndex = getUpdateEndIndex(my_data, device, pDstLayout, pCDS[i].dstBinding, pCDS[i].dstArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
            if (getBindingEndIndex(pSrcLayout, pCDS[i].srcBinding) < srcEndIndex) {
                pLayoutCI = &pSrcLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pSrcSet->set, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                    "Copy descriptor src update is out of bounds for matching binding %u in Layout w/ CI:\n%s!", pCDS[i].srcBinding, DSstr.c_str());
            } else if (getBindingEndIndex(pDstLayout, pCDS[i].dstBinding) < dstEndIndex) {
                pLayoutCI = &pDstLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDstSet->set, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                    "Copy descriptor dest update is out of bounds for matching binding %u in Layout w/ CI:\n%s!", pCDS[i].dstBinding, DSstr.c_str());
            } else {
                srcStartIndex = getUpdateStartIndex(my_data, device, pSrcLayout, pCDS[i].srcBinding, pCDS[i].srcArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
                dstStartIndex = getUpdateStartIndex(my_data, device, pDstLayout, pCDS[i].dstBinding, pCDS[i].dstArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
                for (uint32_t j=0; j<pCDS[i].descriptorCount; ++j) {
                    // For copy just make sure that the types match and then perform the update
                    if (pSrcLayout->descriptorTypes[srcStartIndex+j] != pDstLayout->descriptorTypes[dstStartIndex+j]) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                            "Copy descriptor update index %u, update count #%u, has src update descriptor type %s that does not match overlapping dest descriptor type of %s!",
                            i, j+1, string_VkDescriptorType(pSrcLayout->descriptorTypes[srcStartIndex+j]), string_VkDescriptorType(pDstLayout->descriptorTypes[dstStartIndex+j]));
                    } else {
                        // point dst descriptor at corresponding src descriptor
                        pDstSet->ppDescriptors[j+dstStartIndex] = pSrcSet->ppDescriptors[j+srcStartIndex];
                    }
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return skipCall;
}
// Verify that given pool has descriptors that are being requested for allocation
static VkBool32 validate_descriptor_availability_in_pool(layer_data* dev_data, POOL_NODE* pPoolNode, uint32_t count, const VkDescriptorSetLayout* pSetLayouts)
{
    VkBool32 skipCall = VK_FALSE;
    uint32_t i = 0, j = 0;
    for (i=0; i<count; ++i) {
        LAYOUT_NODE* pLayout = getLayoutNode(dev_data, pSetLayouts[i]);
        if (NULL == pLayout) {
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) pSetLayouts[i], 0, DRAWSTATE_INVALID_LAYOUT, "DS",
                    "Unable to find set layout node for layout %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pSetLayouts[i]);
        } else {
            uint32_t typeIndex = 0, poolSizeCount = 0;
            for (j=0; j<pLayout->createInfo.bindingCount; ++j) {
                typeIndex = static_cast<uint32_t>(pLayout->createInfo.pBinding[j].descriptorType);
                poolSizeCount = pLayout->createInfo.pBinding[j].arraySize;
                if (poolSizeCount > pPoolNode->availableDescriptorTypeCount[typeIndex]) {
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) pLayout->layout, 0, DRAWSTATE_DESCRIPTOR_POOL_EMPTY, "DS",
                        "Unable to allocate %u descriptors of type %s from pool %#" PRIxLEAST64 ". This pool only has %u descriptors of this type remaining.",
                            poolSizeCount, string_VkDescriptorType(pLayout->createInfo.pBinding[j].descriptorType), (uint64_t) pPoolNode->pool, pPoolNode->availableDescriptorTypeCount[typeIndex]);
                } else { // Decrement available descriptors of this type
                    pPoolNode->availableDescriptorTypeCount[typeIndex] -= poolSizeCount;
                }
            }
        }
    }
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
                switch (pWDS->descriptorType) {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    {
                        delete[] pWDS->pImageInfo;
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    {
                        delete[] pWDS->pTexelBufferView;
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    {
                        delete[] pWDS->pBufferInfo;
                    }
                    break;
                default:
                    break;
                }
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
            for (uint32_t i=0; i<pLayout->createInfo.bindingCount; i++) {
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

static void clearDescriptorPool(layer_data* my_data, const VkDevice device, const VkDescriptorPool pool, VkDescriptorPoolResetFlags flags)
{
    POOL_NODE* pPool = getPoolNode(my_data, pool);
    if (!pPool) {
        log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) pool, 0, DRAWSTATE_INVALID_POOL, "DS",
                "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkResetDescriptorPool() call", (uint64_t) pool);
    } else {
        // TODO: validate flags
        // For every set off of this pool, clear it
        SET_NODE* pSet = pPool->pSets;
        while (pSet) {
            clearDescriptorSet(my_data, pSet->set);
        }
        // Reset available count to max count for this pool
        for (uint32_t i=0; i<pPool->availableDescriptorTypeCount.size(); ++i) {
            pPool->availableDescriptorTypeCount[i] = pPool->maxDescriptorTypeCount[i];
        }
    }
}
// For given CB object, fetch associated CB Node from map
static GLOBAL_CB_NODE* getCBNode(layer_data* my_data, const VkCommandBuffer cb)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->commandBufferMap.find(cb) == my_data->commandBufferMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        // TODO : How to pass cb as srcObj here?
        log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                "Attempt to use CommandBuffer %#" PRIxLEAST64 " that doesn't exist!", reinterpret_cast<uint64_t>(cb));
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->commandBufferMap[cb];
}
// Free all CB Nodes
// NOTE : Calls to this function should be wrapped in mutex
static void deleteCommandBuffers(layer_data* my_data)
{
    if (my_data->commandBufferMap.size() <= 0)
        return;
    for (auto ii=my_data->commandBufferMap.begin(); ii!=my_data->commandBufferMap.end(); ++ii) {
        vector<CMD_NODE*> cmd_node_list = (*ii).second->pCmds;
        while (!cmd_node_list.empty()) {
            CMD_NODE* cmd_node = cmd_node_list.back();
            delete cmd_node;
            cmd_node_list.pop_back();
        }
        delete (*ii).second;
    }
    my_data->commandBufferMap.clear();
}
static VkBool32 report_error_no_cb_begin(const layer_data* dev_data, const VkCommandBuffer cb, const char* caller_name)
{
    // TODO : How to pass cb as srcObj here?
    return log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NO_BEGIN_COMMAND_BUFFER, "DS",
            "You must call vkBeginCommandBuffer() before this call to %s", (void*)caller_name);
}
static VkBool32 addCmd(const layer_data* my_data, GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd)
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
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                "Out of memory while attempting to allocate new CMD_NODE for commandBuffer %#" PRIxLEAST64, reinterpret_cast<uint64_t>(pCB->commandBuffer));
    }
    return skipCall;
}
static void resetCB(layer_data* my_data, const VkCommandBuffer cb)
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
        VkCommandBufferAllocateInfo saveCBCI = pCB->createInfo;
        pCB->commandBuffer = cb;
        pCB->createInfo = saveCBCI;
        memset(&pCB->beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        pCB->fence = 0;
        pCB->numCmds = 0;
        memset(pCB->drawCount, 0, NUM_DRAW_TYPES * sizeof(uint64_t));
        pCB->state = CB_NEW;
        pCB->submitCount = 0;
        pCB->status = 0;
        pCB->pCmds.clear();
        pCB->lastBoundPipeline = 0;
        pCB->viewports.clear();
        pCB->scissors.clear();
        pCB->lineWidth = 0;
        pCB->depthBiasConstantFactor = 0;
        pCB->depthBiasClamp = 0;
        pCB->depthBiasSlopeFactor = 0;
        memset(pCB->blendConstants, 0, 4 * sizeof(float));
        pCB->minDepthBounds = 0;
        pCB->maxDepthBounds = 0;
        memset(&pCB->front, 0, sizeof(stencil_data));
        memset(&pCB->back, 0, sizeof(stencil_data));
        pCB->lastBoundDescriptorSet = 0;
        pCB->lastBoundPipelineLayout = 0;
        pCB->activeRenderPass = 0;
        pCB->activeSubpass = 0;
        pCB->framebuffer = 0;
        pCB->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        pCB->boundDescriptorSets.clear();
        pCB->imageLayoutMap.clear();
        pCB->lastVtxBinding = MAX_BINDING;
    }
}
// Set PSO-related status bits for CB, including dynamic state set via PSO
static void set_cb_pso_status(GLOBAL_CB_NODE* pCB, const PIPELINE_NODE* pPipe)
{
    for (uint32_t i = 0; i < pPipe->cbStateCI.attachmentCount; i++) {
        if (0 != pPipe->pAttachments[i].colorWriteMask) {
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
static VkBool32 printPipeline(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(my_data, pCB->lastBoundPipeline);
        if (!pPipeTrav) {
            // nothing to print
        } else {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    vk_print_vkgraphicspipelinecreateinfo(&pPipeTrav->graphicsPipelineCI, "{DS}").c_str());
        }
    }
    return skipCall;
}
// Print details of DS config to stdout
static VkBool32 printDSConfig(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    char ds_config_str[1024*256] = {0}; // TODO : Currently making this buffer HUGE w/o overrun protection.  Need to be smarter, start smaller, and grow as needed.
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->lastBoundDescriptorSet) {
        SET_NODE* pSet = getSetNode(my_data, pCB->lastBoundDescriptorSet);
        POOL_NODE* pPool = getPoolNode(my_data, pSet->pool);
        // Print out pool details
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Details for pool %#" PRIxLEAST64 ".", (uint64_t) pPool->pool);
        string poolStr = vk_print_vkdescriptorpoolcreateinfo(&pPool->createInfo, " ");
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "%s", poolStr.c_str());
        // Print out set details
        char prefix[10];
        uint32_t index = 0;
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Details for descriptor set %#" PRIxLEAST64 ".", (uint64_t) pSet->set);
        LAYOUT_NODE* pLayout = pSet->pLayout;
        // Print layout details
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Layout #%u, (object %#" PRIxLEAST64 ") for DS %#" PRIxLEAST64 ".", index+1, (void*)pLayout->layout, (void*)pSet->set);
        sprintf(prefix, "  [L%u] ", index);
        string DSLstr = vk_print_vkdescriptorsetlayoutcreateinfo(&pLayout->createInfo, prefix).c_str();
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "%s", DSLstr.c_str());
        index++;
        GENERIC_HEADER* pUpdate = pSet->pUpdateStructs;
        if (pUpdate) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    "Update Chain [UC] for descriptor set %#" PRIxLEAST64 ":", (uint64_t) pSet->set);
            sprintf(prefix, "  [UC] ");
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    dynamic_display(pUpdate, prefix).c_str());
            // TODO : If there is a "view" associated with this update, print CI for that view
        } else {
            if (0 != pSet->descriptorCount) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                        "No Update Chain for descriptor set %#" PRIxLEAST64 " which has %u descriptors (vkUpdateDescriptors has not been called)", (uint64_t) pSet->set, pSet->descriptorCount);
            } else {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                        "FYI: No descriptors in descriptor set %#" PRIxLEAST64 ".", (uint64_t) pSet->set);
            }
        }
    }
    return skipCall;
}

static void printCB(layer_data* my_data, const VkCommandBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->pCmds.size() > 0) {
        log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Cmds in CB %p", (void*)cb);
        vector<CMD_NODE*> pCmds = pCB->pCmds;
        for (auto ii=pCmds.begin(); ii!=pCmds.end(); ++ii) {
            // TODO : Need to pass cb as srcObj here
            log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                "  CMD#%lu: %s", (*ii)->cmdNumber, cmdTypeToString((*ii)->type).c_str());
        }
    } else {
        // Nothing to print
    }
}

static VkBool32 synchAndPrintDSConfig(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    if (!(my_data->report_data->active_flags & VK_DBG_REPORT_INFO_BIT)) {
        return skipCall;
    }
    skipCall |= printDSConfig(my_data, cb);
    skipCall |= printPipeline(my_data, cb);
    return skipCall;
}

// Flags validation error if the associated call is made inside a render pass. The apiName
// routine should ONLY be called outside a render pass.
static VkBool32 insideRenderPass(const layer_data* my_data, GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 inside = VK_FALSE;
    if (pCB->activeRenderPass) {
        inside = log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                         (uint64_t)pCB->commandBuffer, 0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                         "%s: It is invalid to issue this call inside an active render pass (%#" PRIxLEAST64 ")",
                         apiName, (uint64_t) pCB->activeRenderPass);
    }
    return inside;
}

// Flags validation error if the associated call is made outside a render pass. The apiName
// routine should ONLY be called inside a render pass.
static VkBool32 outsideRenderPass(const layer_data* my_data, GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 outside = VK_FALSE;
    if (!pCB->activeRenderPass) {
        outside = log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                          (uint64_t)pCB->commandBuffer, 0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult result = pTable->CreateInstance(pCreateInfo, pAllocator, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->enabledExtensionNameCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        init_draw_state(my_data);
    }
    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT void VKAPI vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance, pAllocator);

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
    dev_data->device_extensions.wsi_enabled = false;


    VkLayerDispatchTable    *pDisp = dev_data->device_dispatch_table;
    PFN_vkGetDeviceProcAddr  gpa   = pDisp->GetDeviceProcAddr;

    pDisp->GetSurfacePropertiesKHR   = (PFN_vkGetSurfacePropertiesKHR) gpa(device, "vkGetSurfacePropertiesKHR");
    pDisp->GetSurfaceFormatsKHR      = (PFN_vkGetSurfaceFormatsKHR) gpa(device, "vkGetSurfaceFormatsKHR");
    pDisp->GetSurfacePresentModesKHR = (PFN_vkGetSurfacePresentModesKHR) gpa(device, "vkGetSurfacePresentModesKHR");
    pDisp->CreateSwapchainKHR        = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR       = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR     = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR       = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR           = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");    

    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0) {
            dev_data->device_extensions.wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], DEBUG_MARKER_EXTENSION_NAME) == 0) {
            /* Found a matching extension name, mark it enabled and init dispatch table*/
            initDebugMarkerTable(device);
            dev_data->device_extensions.debug_marker_enabled = true;
        }
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        dev_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    }
    return result;
}
// prototype
static void deleteRenderPasses(layer_data*);
VK_LAYER_EXPORT void VKAPI vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data* dev_data = get_my_data_ptr(key, layer_data_map);
    // Free all the memory
    loader_platform_thread_lock_mutex(&globalLock);
    deletePipelines(dev_data);
    deleteRenderPasses(dev_data);
    deleteCommandBuffers(dev_data);
    deletePools(dev_data);
    deleteLayouts(dev_data);
    dev_data->imageViewMap.clear();
    dev_data->imageMap.clear();
    dev_data->bufferViewMap.clear();
    dev_data->bufferMap.clear();
    loader_platform_thread_unlock_mutex(&globalLock);

    dev_data->device_dispatch_table->DestroyDevice(device, pAllocator);
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
    if (pLayerName == NULL) {
        dispatch_key key = get_dispatch_key(physicalDevice);
        layer_data *my_data = get_my_data_ptr(key, layer_data_map);
        VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
        return pTable->EnumerateDeviceExtensionProperties(
                                                    physicalDevice,
                                                    NULL,
                                                    pCount,
                                                    pProperties);
    } else {
        return util_GetExtensionProperties(ARRAY_SIZE(ds_device_extensions),
                                           ds_device_extensions,
                                           pCount, pProperties);
    }
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

bool ValidateCmdBufImageLayouts(VkCommandBuffer cmdBuffer) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    for (auto cb_image_data : pCB->imageLayoutMap) {
        auto image_data = dev_data->imageLayoutMap.find(cb_image_data.first);
        if (image_data == dev_data->imageLayoutMap.end()) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Cannot submit cmd buffer using deleted image %d.", cb_image_data.first);
        } else {
            if (dev_data->imageLayoutMap[cb_image_data.first]->layout != cb_image_data.second.initialLayout) {
                skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                     "Cannot submit cmd buffer using image with layout %d when first use is %d.", dev_data->imageLayoutMap[cb_image_data.first]->layout, cb_image_data.second.initialLayout);
            }
            dev_data->imageLayoutMap[cb_image_data.first]->layout = cb_image_data.second.layout;
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = NULL;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        for (uint32_t i=0; i < submit->commandBufferCount; i++) {
            skipCall |= ValidateCmdBufImageLayouts(submit->pCommandBuffers[i]);
            // Validate that cmd buffers have been updated
            pCB = getCBNode(dev_data, submit->pCommandBuffers[i]);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->submitCount++; // increment submit count
            if ((pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) && (pCB->submitCount > 1)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                        "CB %#" PRIxLEAST64 " was begun w/ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT set, but has been submitted %#" PRIxLEAST64 " times.",
                        reinterpret_cast<uint64_t>(pCB->commandBuffer), pCB->submitCount);
            }
            if (CB_UPDATE_COMPLETE != pCB->state) {
                // Flag error for using CB w/o vkEndCommandBuffer() called
                // TODO : How to pass cb as srcObj?
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NO_END_COMMAND_BUFFER, "DS",
                        "You must call vkEndCommandBuffer() on CB %#" PRIxLEAST64 " before this call to vkQueueSubmit()!", reinterpret_cast<uint64_t>(pCB->commandBuffer));
                loader_platform_thread_unlock_mutex(&globalLock);
                return VK_ERROR_VALIDATION_FAILED;
            }
            loader_platform_thread_unlock_mutex(&globalLock);
        }
    }
    if (VK_FALSE == skipCall)
        return dev_data->device_dispatch_table->QueueSubmit(queue, submitCount, pSubmits, fence);
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT void VKAPI vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyFence(device, fence, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroySemaphore(device, semaphore, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyEvent(device, event, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyQueryPool(device, queryPool, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyBuffer(device, buffer, pAllocator);
    dev_data->bufferMap.erase(buffer);
}

VK_LAYER_EXPORT void VKAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyBufferView(device, bufferView, pAllocator);
    dev_data->bufferViewMap.erase(bufferView);
}

VK_LAYER_EXPORT void VKAPI vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyImage(device, image, pAllocator);
    dev_data->imageMap.erase(image);
}

VK_LAYER_EXPORT void VKAPI vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyImageView(device, imageView, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyShaderModule(device, shaderModule, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipeline(device, pipeline, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroySampler(device, sampler, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorPool(device, descriptorPool, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, const VkCommandBuffer *pCommandBuffers)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->FreeCommandBuffers(device, commandPool, count, pCommandBuffers);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyFramebuffer(device, framebuffer, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT void VKAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyRenderPass(device, renderPass, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // TODO : This doesn't create deep copy of pQueueFamilyIndices so need to fix that if/when we want that data to be valid
        dev_data->bufferMap[*pBuffer] = unique_ptr<VkBufferCreateInfo>(new VkBufferCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->bufferViewMap[*pView] = unique_ptr<VkBufferViewCreateInfo>(new VkBufferViewCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateImage(device, pCreateInfo, pAllocator, pImage);
    if (VK_SUCCESS == result) {
        IMAGE_NODE* image_node = new IMAGE_NODE;
        image_node->layout = pCreateInfo->initialLayout;
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->imageMap[*pImage] = unique_ptr<VkImageCreateInfo>(new VkImageCreateInfo(*pCreateInfo));
        dev_data->imageLayoutMap[*pImage] = image_node;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateImageView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->imageViewMap[*pView] = unique_ptr<VkImageViewCreateInfo>(new VkImageViewCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

//TODO handle pipeline caches
VkResult VKAPI vkCreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipelineCache*                            pPipelineCache)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
    return result;
}

void VKAPI vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyPipelineCache(device, pipelineCache, pAllocator);
}

VkResult VKAPI vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}

VkResult VKAPI vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
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
        pPipeNode[i] = initPipeline(dev_data, &pCreateInfos[i], NULL);
        skipCall |= verifyPipelineCreateState(dev_data, device, pPipeNode[i]);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = dev_data->device_dispatch_table->CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
        loader_platform_thread_lock_mutex(&globalLock);
        for (i=0; i<count; i++) {
            pPipeNode[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeNode[i]->pipeline] = pPipeNode[i];
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->sampleMap[*pSampler] = unique_ptr<SAMPLER_NODE>(new SAMPLER_NODE(pSampler, pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    if (VK_SUCCESS == result) {
        LAYOUT_NODE* pNewNode = new LAYOUT_NODE;
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) *pSetLayout, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate LAYOUT_NODE in vkCreateDescriptorSetLayout()"))
                return VK_ERROR_VALIDATION_FAILED;
        }
        memset(pNewNode, 0, sizeof(LAYOUT_NODE));
        memcpy((void*)&pNewNode->createInfo, pCreateInfo, sizeof(VkDescriptorSetLayoutCreateInfo));
        pNewNode->createInfo.pBinding = new VkDescriptorSetLayoutBinding[pCreateInfo->bindingCount];
        memcpy((void*)pNewNode->createInfo.pBinding, pCreateInfo->pBinding, sizeof(VkDescriptorSetLayoutBinding)*pCreateInfo->bindingCount);
        // g++ does not like reserve with size 0
        if (pCreateInfo->bindingCount)
            pNewNode->bindings.reserve(pCreateInfo->bindingCount);
        uint32_t totalCount = 0;
        for (uint32_t i=0; i<pCreateInfo->bindingCount; i++) {
            if (!pNewNode->bindings.insert(pCreateInfo->pBinding[i].binding).second) {
                if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) *pSetLayout, 0, DRAWSTATE_INVALID_LAYOUT, "DS",
                            "duplicated binding number in VkDescriptorSetLayoutBinding"))
                    return VK_ERROR_VALIDATION_FAILED;
            }

            totalCount += pCreateInfo->pBinding[i].arraySize;
            if (pCreateInfo->pBinding[i].pImmutableSamplers) {
                VkSampler** ppIS = (VkSampler**)&pNewNode->createInfo.pBinding[i].pImmutableSamplers;
                *ppIS = new VkSampler[pCreateInfo->pBinding[i].arraySize];
                memcpy(*ppIS, pCreateInfo->pBinding[i].pImmutableSamplers, pCreateInfo->pBinding[i].arraySize*sizeof(VkSampler));
            }
        }
        if (totalCount > 0) {
            pNewNode->descriptorTypes.resize(totalCount);
            pNewNode->stageFlags.resize(totalCount);
            uint32_t offset = 0;
            uint32_t j = 0;
            for (uint32_t i=0; i<pCreateInfo->bindingCount; i++) {
                for (j = 0; j < pCreateInfo->pBinding[i].arraySize; j++) {
                    pNewNode->descriptorTypes[offset + j] = pCreateInfo->pBinding[i].descriptorType;
                    pNewNode->stageFlags[offset + j] = pCreateInfo->pBinding[i].stageFlags;
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
        dev_data->layoutMap[*pSetLayout] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VkResult VKAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    if (VK_SUCCESS == result) {
        PIPELINE_LAYOUT_NODE plNode = dev_data->pipelineLayoutMap[*pPipelineLayout];
        plNode.descriptorSetLayouts.resize(pCreateInfo->setLayoutCount);
        uint32_t i = 0;
        for (i=0; i<pCreateInfo->setLayoutCount; ++i) {
            plNode.descriptorSetLayouts[i] = pCreateInfo->pSetLayouts[i];
        }
        plNode.pushConstantRanges.resize(pCreateInfo->pushConstantRangeCount);
        for (i=0; i<pCreateInfo->pushConstantRangeCount; ++i) {
            plNode.pushConstantRanges[i] = pCreateInfo->pPushConstantRanges[i];
        }
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    if (VK_SUCCESS == result) {
        // Insert this pool into Global Pool LL at head
        if (log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) *pDescriptorPool, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                "Created Descriptor Pool %#" PRIxLEAST64, (uint64_t) *pDescriptorPool))
            return VK_ERROR_VALIDATION_FAILED;
        loader_platform_thread_lock_mutex(&globalLock);
        POOL_NODE* pNewNode = new POOL_NODE(*pDescriptorPool, pCreateInfo);
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) *pDescriptorPool, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate POOL_NODE in vkCreateDescriptorPool()"))
                return VK_ERROR_VALIDATION_FAILED;
        } else {
            dev_data->poolMap[*pDescriptorPool] = pNewNode;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        // Need to do anything if pool create fails?
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->ResetDescriptorPool(device, descriptorPool, flags);
    if (VK_SUCCESS == result) {
        clearDescriptorPool(dev_data, device, descriptorPool, flags);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Verify that requested descriptorSets are available in pool
    POOL_NODE *pPoolNode = getPoolNode(dev_data, pAllocateInfo->descriptorPool);
    if (!pPoolNode) {
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) pAllocateInfo->descriptorPool, 0, DRAWSTATE_INVALID_POOL, "DS",
                "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pAllocateInfo->descriptorPool);
    } else { // Make sure pool has all the available descriptors before calling down chain
        skipCall |= validate_descriptor_availability_in_pool(dev_data, pPoolNode, pAllocateInfo->setLayoutCount, pAllocateInfo->pSetLayouts);
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
    VkResult result = dev_data->device_dispatch_table->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
    if (VK_SUCCESS == result) {
        POOL_NODE *pPoolNode = getPoolNode(dev_data, pAllocateInfo->descriptorPool);
        if (pPoolNode) {
            if (pAllocateInfo->setLayoutCount == 0) {
                log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, pAllocateInfo->setLayoutCount, 0, DRAWSTATE_NONE, "DS",
                        "AllocateDescriptorSets called with 0 count");
            }
            for (uint32_t i = 0; i < pAllocateInfo->setLayoutCount; i++) {
                log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS",
                        "Created Descriptor Set %#" PRIxLEAST64, (uint64_t) pDescriptorSets[i]);
                // Create new set node and add to head of pool nodes
                SET_NODE* pNewNode = new SET_NODE;
                if (NULL == pNewNode) {
                    if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                            "Out of memory while attempting to allocate SET_NODE in vkAllocateDescriptorSets()"))
                        return VK_ERROR_VALIDATION_FAILED;
                } else {
                    memset(pNewNode, 0, sizeof(SET_NODE));
                    // TODO : Pool should store a total count of each type of Descriptor available
                    //  When descriptors are allocated, decrement the count and validate here
                    //  that the count doesn't go below 0. One reset/free need to bump count back up.
                    // Insert set at head of Set LL for this pool
                    pNewNode->pNext = pPoolNode->pSets;
                    pPoolNode->pSets = pNewNode;
                    LAYOUT_NODE* pLayout = getLayoutNode(dev_data, pAllocateInfo->pSetLayouts[i]);
                    if (NULL == pLayout) {
                        if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) pAllocateInfo->pSetLayouts[i], 0, DRAWSTATE_INVALID_LAYOUT, "DS",
                                "Unable to find set layout node for layout %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pAllocateInfo->pSetLayouts[i]))
                            return VK_ERROR_VALIDATION_FAILED;
                    }
                    pNewNode->pLayout = pLayout;
                    pNewNode->pool = pAllocateInfo->descriptorPool;
                    pNewNode->set = pDescriptorSets[i];
                    pNewNode->descriptorCount = pLayout->endIndex + 1;
                    if (pNewNode->descriptorCount) {
                        size_t descriptorArraySize = sizeof(GENERIC_HEADER*)*pNewNode->descriptorCount;
                        pNewNode->ppDescriptors = new GENERIC_HEADER*[descriptorArraySize];
                        memset(pNewNode->ppDescriptors, 0, descriptorArraySize);
                    }
                    dev_data->setMap[pDescriptorSets[i]] = pNewNode;
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
    if (pPoolNode && !(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT & pPoolNode->createInfo.flags)) {
        // Can't Free from a NON_FREE pool
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, (uint64_t)device, 0, DRAWSTATE_CANT_FREE_FROM_NON_FREE_POOL, "DS",
                    "It is invalid to call vkFreeDescriptorSets() with a pool created without setting VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.");
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
    VkResult result = dev_data->device_dispatch_table->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    if (VK_SUCCESS == result) {
        // For each freed descriptor add it back into the pool as available
        for (uint32_t i=0; i<count; ++i) {
            SET_NODE* pSet = dev_data->setMap[pDescriptorSets[i]]; // getSetNode() without locking
            LAYOUT_NODE* pLayout = pSet->pLayout;
            uint32_t typeIndex = 0, poolSizeCount = 0;
            for (uint32_t j=0; j<pLayout->createInfo.bindingCount; ++j) {
                typeIndex = static_cast<uint32_t>(pLayout->createInfo.pBinding[j].descriptorType);
                poolSizeCount = pLayout->createInfo.pBinding[j].arraySize;
                pPoolNode->availableDescriptorTypeCount[typeIndex] += poolSizeCount;
            }
        }
    }
    // TODO : Any other clean-up or book-keeping to do here?
    return result;
}

VK_LAYER_EXPORT void VKAPI vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    // dsUpdate will return VK_TRUE only if a bailout error occurs, so we want to call down tree when update returns VK_FALSE
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    if (!dsUpdate(dev_data, device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies)) {
        dev_data->device_dispatch_table->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pCreateInfo, VkCommandBuffer* pCommandBuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        GLOBAL_CB_NODE* pCB = new GLOBAL_CB_NODE;
        dev_data->commandBufferMap[*pCommandBuffer] = pCB;
        loader_platform_thread_unlock_mutex(&globalLock);
        resetCB(dev_data, *pCommandBuffer);
        pCB->commandBuffer = *pCommandBuffer;
        pCB->createInfo    = *pCreateInfo;
        pCB->level         = pCreateInfo->level;
        updateCBTracking(pCB);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
{
    VkBool32 skipCall = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // Validate command buffer level
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
            if (pBeginInfo->renderPass || pBeginInfo->framebuffer) {
                // These should be NULL for a Primary CB
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                    "vkAllocateCommandBuffers():  Primary Command Buffer (%p) may not specify framebuffer or renderpass parameters", (void*)commandBuffer);
            }
        } else {
            if (!pBeginInfo->renderPass || !pBeginInfo->framebuffer) {
                // These should NOT be null for an Secondary CB
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                    "vkAllocateCommandBuffers():  Secondary Command Buffers (%p) must specify framebuffer and renderpass parameters", (void*)commandBuffer);
            }
        }
        pCB->beginInfo = *pBeginInfo;
    } else {
        // TODO : Need to pass commandBuffer as objType here
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                "In vkBeginCommandBuffer() and unable to find CommandBuffer Node for CB %p!", (void*)commandBuffer);
    }
    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED;
    }
    VkResult result = dev_data->device_dispatch_table->BeginCommandBuffer(commandBuffer, pBeginInfo);
    if (VK_SUCCESS == result) {
        if (CB_NEW != pCB->state)
            resetCB(dev_data, commandBuffer);
        pCB->state = CB_UPDATE_ACTIVE;
        updateCBTracking(pCB);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    VkResult result = VK_SUCCESS;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    /* TODO: preference is to always call API function after reporting any validation errors */
    if (pCB) {
        if (pCB->state != CB_UPDATE_ACTIVE) {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkEndCommandBuffer()");
        }
    }
    if (VK_FALSE == skipCall) {
        result = dev_data->device_dispatch_table->EndCommandBuffer(commandBuffer);
        if (VK_SUCCESS == result) {
            updateCBTracking(pCB);
            pCB->state = CB_UPDATE_COMPLETE;
            // Reset CB status flags
            pCB->status = 0;
            printCB(dev_data, commandBuffer);
        }
    } else {
        result = VK_ERROR_VALIDATION_FAILED;
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->ResetCommandBuffer(commandBuffer, flags);
    if (VK_SUCCESS == result) {
        resetCB(dev_data, commandBuffer);
        updateCBTracking(getCBNode(dev_data, commandBuffer));
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_BINDPIPELINE);
            if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline,
                                    0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                                    "Incorrectly binding compute pipeline (%#" PRIxLEAST64 ") during active RenderPass (%#" PRIxLEAST64 ")",
                                    (uint64_t) pipeline, (uint64_t) pCB->activeRenderPass);
            } else if ((VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) && (!pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline,
                                    0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS", "Incorrectly binding graphics pipeline "
                                    " (%#" PRIxLEAST64 ") without an active RenderPass", (uint64_t) pipeline);
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
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline,
                                        0, DRAWSTATE_INVALID_PIPELINE, "DS",
                                        "Attempt to bind Pipeline %#" PRIxLEAST64 " that doesn't exist!", (void*)pipeline);
                }
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindPipeline()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetViewport(
    VkCommandBuffer                         commandBuffer,
    uint32_t                            viewportCount,
    const VkViewport*                   pViewports)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETVIEWPORTSTATE);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_VIEWPORT_SET;
            pCB->viewports.resize(viewportCount);
            memcpy(pCB->viewports.data(), pViewports, viewportCount * sizeof(VkViewport));
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetViewport()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetViewport(commandBuffer, viewportCount, pViewports);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetScissor(
    VkCommandBuffer                         commandBuffer,
    uint32_t                            scissorCount,
    const VkRect2D*                     pScissors)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETSCISSORSTATE);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_SCISSOR_SET;
            pCB->scissors.resize(scissorCount);
            memcpy(pCB->scissors.data(), pScissors, scissorCount * sizeof(VkRect2D));
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetScissor()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetScissor(commandBuffer, scissorCount, pScissors);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETLINEWIDTHSTATE);
            /* TODO: Do we still need this lock? */
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_LINE_WIDTH_SET;
            pCB->lineWidth = lineWidth;
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindDynamicLineWidthState()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetLineWidth(commandBuffer, lineWidth);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetDepthBias(
    VkCommandBuffer                         commandBuffer,
    float                               depthBiasConstantFactor,
    float                               depthBiasClamp,
    float                               depthBiasSlopeFactor)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETDEPTHBIASSTATE);
            pCB->status |= CBSTATUS_DEPTH_BIAS_SET;
            pCB->depthBiasConstantFactor = depthBiasConstantFactor;
            pCB->depthBiasClamp = depthBiasClamp;
            pCB->depthBiasSlopeFactor = depthBiasSlopeFactor;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetDepthBias()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETBLENDSTATE);
            pCB->status |= CBSTATUS_BLEND_SET;
            memcpy(pCB->blendConstants, blendConstants, 4 * sizeof(float));
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetBlendConstants()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetBlendConstants(commandBuffer, blendConstants);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetDepthBounds(
    VkCommandBuffer                         commandBuffer,
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETDEPTHBOUNDSSTATE);
            pCB->status |= CBSTATUS_DEPTH_BOUNDS_SET;
            pCB->minDepthBounds = minDepthBounds;
            pCB->maxDepthBounds = maxDepthBounds;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetDepthBounds()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetStencilCompareMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            compareMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILREADMASKSTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.compareMask = compareMask;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.compareMask = compareMask;
            }
            /* TODO: Do we need to track front and back separately? */
            /* TODO: We aren't capturing the faceMask, do we need to? */
            pCB->status |= CBSTATUS_STENCIL_READ_MASK_SET;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetStencilCompareMask()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetStencilWriteMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            writeMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILWRITEMASKSTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.writeMask = writeMask;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.writeMask = writeMask;
            }
            pCB->status |= CBSTATUS_STENCIL_WRITE_MASK_SET;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetStencilWriteMask()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetStencilReference(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            reference)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILREFERENCESTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.reference = reference;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.reference = reference;
            }
            pCB->status |= CBSTATUS_STENCIL_REFERENCE_SET;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetStencilReference()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilReference(commandBuffer, faceMask, reference);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    // TODO : Validate dynamic offsets
    //  If any of the sets being bound include dynamic uniform or storage buffers,
    //  then pDynamicOffsets must include one element for each array element
    //  in each dynamic descriptor type binding in each set.
    //  dynamicOffsetCount is the total number of dynamic offsets provided, and
    //  must equal the total number of dynamic descriptors in the sets being bound
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                        "Incorrectly binding compute DescriptorSets during active RenderPass (%#" PRIxLEAST64 ")", (uint64_t) pCB->activeRenderPass);
            } else if ((VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) && (!pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
                        "Incorrectly binding graphics DescriptorSets without an active RenderPass");
            } else {
                for (uint32_t i=0; i<setCount; i++) {
                    SET_NODE* pSet = getSetNode(dev_data, pDescriptorSets[i]);
                    if (pSet) {
                        loader_platform_thread_lock_mutex(&globalLock);
                        pCB->lastBoundDescriptorSet = pDescriptorSets[i];
                        pCB->lastBoundPipelineLayout = layout;
                        pCB->boundDescriptorSets.push_back(pDescriptorSets[i]);
                        loader_platform_thread_unlock_mutex(&globalLock);
                        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS",
                                "DS %#" PRIxLEAST64 " bound on pipeline %s", (uint64_t) pDescriptorSets[i], string_VkPipelineBindPoint(pipelineBindPoint));
                        if (!pSet->pUpdateStructs)
                            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                    "DS %#" PRIxLEAST64 " bound but it was never updated. You may want to either update it or not bind it.", (uint64_t) pDescriptorSets[i]);
                    } else {
                        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_INVALID_SET, "DS",
                                "Attempt to bind DS %#" PRIxLEAST64 " that doesn't exist!", (uint64_t) pDescriptorSets[i]);
                    }
                }
                updateCBTracking(pCB);
                skipCall |= addCmd(dev_data, pCB, CMD_BINDDESCRIPTORSETS);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindDescriptorSets()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
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
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR, "DS",
                    "vkCmdBindIndexBuffer() offset (%#" PRIxLEAST64 ") does not fall on alignment (%s) boundary.", offset, string_VkIndexType(indexType));
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindIndexBuffer()");
        }
        pCB->status |= CBSTATUS_INDEX_BUFFER_BOUND;
        updateCBTracking(pCB);
        skipCall |= addCmd(dev_data, pCB, CMD_BINDINDEXBUFFER);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            /* TODO: Need to track all the vertex buffers, not just last one */
            pCB->lastVtxBinding = startBinding + bindingCount -1;
            updateCBTracking(pCB);
            addCmd(dev_data, pCB, CMD_BINDVERTEXBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindVertexBuffer()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindVertexBuffers(commandBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDraw() call #%lu, reporting DS state:", g_drawCount[DRAW]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(dev_data, pCB, CMD_DRAW);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDraw()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDraw");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDEXED]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndexed() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDEXED);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDrawIndexed()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndexed");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDIRECT]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDIRECT]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDIRECT);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDrawIndirect()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDEXED_INDIRECT]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndexedIndirect() call #%lu, reporting DS state:", g_drawCount[DRAW_INDEXED_INDIRECT]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                updateCBTracking(pCB);
                skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDEXEDINDIRECT);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDrawIndexedIndirect()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndexedIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_DISPATCH);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDispatch()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdDispatch");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatch(commandBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_DISPATCHINDIRECT);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDispatchIndirect()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdDispatchIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatchIndirect(commandBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_COPYBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

bool VerifySourceImageLayout(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto src_image_element = pCB->imageLayoutMap.find(srcImage);
    if (src_image_element == pCB->imageLayoutMap.end()) {
        pCB->imageLayoutMap[srcImage].initialLayout = srcImageLayout;
        pCB->imageLayoutMap[srcImage].layout = srcImageLayout;
        return false;
    }
    if (src_image_element->second.layout != srcImageLayout) {
        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                             "Cannot copy from an image whose source layout is %d and doesn't match the current layout %d.", srcImageLayout, src_image_element->second.layout);
    }
    if (srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        if (srcImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for input image should be TRANSFER_SOURCE_OPTIMAL instead of GENERAL.");
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for input image is %d but can only be TRANSFER_SOURCE_OPTIMAL or GENERAL.", srcImageLayout);
        }
    }
    return skip_call;
}

bool VerifyDestImageLayout(VkCommandBuffer cmdBuffer, VkImage destImage, VkImageLayout destImageLayout) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto dest_image_element = pCB->imageLayoutMap.find(destImage);
    if (dest_image_element == pCB->imageLayoutMap.end()) {
        pCB->imageLayoutMap[destImage].initialLayout = destImageLayout;
        pCB->imageLayoutMap[destImage].layout = destImageLayout;
        return false;
    }
    if (dest_image_element->second.layout != destImageLayout) {
        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                             "Cannot copy from an image whose dest layout is %d and doesn't match the current layout %d.", destImageLayout, dest_image_element->second.layout);
    }
    if (destImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        if (destImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for output image should be TRANSFER_DST_OPTIMAL instead of GENERAL.");
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for output image is %d but can only be TRANSFER_DST_OPTIMAL or GENERAL.", destImageLayout);
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VkCommandBuffer commandBuffer,
                                             VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkImage dstImage,
                                             VkImageLayout dstImageLayout,
                                             uint32_t regionCount, const VkImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_COPYIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyImage");
        skipCall |= VerifySourceImageLayout(commandBuffer, srcImage, srcImageLayout);
        skipCall |= VerifyDestImageLayout(commandBuffer, dstImage, dstImageLayout);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VkCommandBuffer commandBuffer,
                                             VkImage srcImage, VkImageLayout srcImageLayout,
                                             VkImage dstImage, VkImageLayout dstImageLayout,
                                             uint32_t regionCount, const VkImageBlit* pRegions,
                                             VkFilter filter)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_BLITIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBlitImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdBlitImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer,
                                                     VkBuffer srcBuffer,
                                                     VkImage dstImage, VkImageLayout dstImageLayout,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_COPYBUFFERTOIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyBufferToImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyBufferToImage");
        skipCall |= VerifyDestImageLayout(commandBuffer, dstImage, dstImageLayout);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer,
                                                     VkImage srcImage, VkImageLayout srcImageLayout,
                                                     VkBuffer dstBuffer,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_COPYIMAGETOBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyImageToBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyImageToBuffer");
        skipCall |= VerifySourceImageLayout(commandBuffer, srcImage, srcImageLayout);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_UPDATEBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdUpdateBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyUpdateBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_FILLBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdFillBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyFillBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearAttachments(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            // Warn if this is issued prior to Draw Cmd
            if (!hasDrawCmd(pCB)) {
                // TODO : commandBuffer should be srcObj
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, "DS",
                        "vkCmdClearAttachments() issued on CB object 0x%" PRIxLEAST64 " prior to any Draw Cmds."
                        " It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw.", reinterpret_cast<uint64_t>(commandBuffer));
            }
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_CLEARATTACHMENTS);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdClearAttachments()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdClearAttachments");
    }

    // Validate that attachment is in reference list of active subpass
    if (pCB->activeRenderPass) {
        const VkRenderPassCreateInfo *pRPCI = dev_data->renderPassMap[pCB->activeRenderPass]->createInfo;
        const VkSubpassDescription   *pSD   = &pRPCI->pSubpasses[pCB->activeSubpass];

        for (uint32_t attachment_idx = 0; attachment_idx < attachmentCount; attachment_idx++) {
            const VkClearAttachment *attachment = &pAttachments[attachment_idx];
            if (attachment->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
                VkBool32 found = VK_FALSE;
                for (uint32_t i = 0; i < pSD->colorAttachmentCount; i++) {
                    if (attachment->colorAttachment == pSD->pColorAttachments[i].attachment) {
                        found = VK_TRUE;
                        break;
                    }
                }
                if (VK_FALSE == found) {
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                            (uint64_t)commandBuffer, 0, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                            "vkCmdClearAttachments() attachment index %d not found in attachment reference array of active subpass %d",
                            attachment->colorAttachment, pCB->activeSubpass);
                }
            } else if (attachment->aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
                /* TODO: Is this a good test for depth/stencil? */
                if (!pSD->pDepthStencilAttachment || pSD->pDepthStencilAttachment->attachment != attachment->colorAttachment) {
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                            (uint64_t)commandBuffer, 0, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                            "vkCmdClearAttachments() attachment index %d does not match depthStencilAttachment.attachment (%d) found in active subpass %d",
                            attachment->colorAttachment,
                            (pSD->pDepthStencilAttachment) ? pSD->pDepthStencilAttachment->attachment : VK_ATTACHMENT_UNUSED,
                            pCB->activeSubpass);
                }
            }
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(
        VkCommandBuffer commandBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearColorValue *pColor,
        uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_CLEARCOLORIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdClearColorImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdClearColorImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencilImage(
        VkCommandBuffer commandBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearDepthStencilValue *pDepthStencil,
        uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_CLEARDEPTHSTENCILIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdClearDepthStencilImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdClearDepthStencilImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VkCommandBuffer commandBuffer,
                                                VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkImage dstImage, VkImageLayout dstImageLayout,
                                                uint32_t regionCount, const VkImageResolve* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_RESOLVEIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResolveImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdResolveImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_SETEVENT);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetEvent()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdSetEvent");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetEvent(commandBuffer, event, stageMask);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_RESETEVENT);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResetEvent()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdResetEvent");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetEvent(commandBuffer, event, stageMask);
}

bool TransitionImageLayouts(VkCommandBuffer cmdBuffer, uint32_t memBarrierCount, const void* const* ppMemBarriers) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    bool skip = false;
    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = reinterpret_cast<const VkMemoryBarrier*>(ppMemBarriers[i]);
        if (mem_barrier && mem_barrier->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            auto image_mem_barrier = reinterpret_cast<const VkImageMemoryBarrier*>(mem_barrier);
            auto image_data = pCB->imageLayoutMap.find(image_mem_barrier->image);
            if (image_data == pCB->imageLayoutMap.end()) {
                pCB->imageLayoutMap[image_mem_barrier->image].initialLayout = image_mem_barrier->oldLayout;
                pCB->imageLayoutMap[image_mem_barrier->image].layout = image_mem_barrier->newLayout;
            } else {
                if (image_data->second.layout != image_mem_barrier->oldLayout) {
                    skip |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "You cannot transition the layout from %d when current layout is %d.", image_mem_barrier->oldLayout, image_data->second.layout);
                }
                image_data->second.layout = image_mem_barrier->newLayout;
            }
        }
    }
    return skip;
}

bool ValidateBarriers(VkCommandBuffer cmdBuffer, uint32_t memBarrierCount, const void* const* ppMemBarriers) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB->activeRenderPass && memBarrierCount) {
        for (uint32_t i = 0; i < memBarrierCount; ++i) {
            auto mem_barrier = reinterpret_cast<const VkMemoryBarrier*>(ppMemBarriers[i]);
            if (mem_barrier && mem_barrier->sType != VK_STRUCTURE_TYPE_MEMORY_BARRIER) {
                skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                     "Image or Buffers Barriers cannot be used during a render pass.");
            }
        }
        if (!dev_data->renderPassMap[pCB->activeRenderPass]->hasSelfDependency[pCB->activeSubpass]) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "Barriers cannot be set during subpass %d with no self dependency specified.", pCB->activeSubpass);
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags sourceStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const void* const* ppMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_WAITEVENTS);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWaitEvents()");
        }
        skipCall |= TransitionImageLayouts(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
        skipCall |= ValidateBarriers(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWaitEvents(commandBuffer, eventCount, pEvents, sourceStageMask, dstStageMask, memoryBarrierCount, ppMemoryBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const void* const* ppMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_PIPELINEBARRIER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdPipelineBarrier()");
        }
        skipCall |= TransitionImageLayouts(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
        skipCall |= ValidateBarriers(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, ppMemoryBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_BEGINQUERY);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBeginQuery()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_ENDQUERY);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdEndQuery()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndQuery(commandBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_RESETQUERYPOOL);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResetQueryPool()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdQueryPool");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetQueryPool(commandBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t startQuery,
                                                     uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                                     VkDeviceSize stride, VkQueryResultFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_COPYQUERYPOOLRESULTS);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyQueryPoolResults()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyQueryPoolResults");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyQueryPoolResults(commandBuffer, queryPool,
                           startQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t slot)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_WRITETIMESTAMP);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWriteTimestamp()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VkFramebufferCreateInfo* localFBCI = new VkFramebufferCreateInfo(*pCreateInfo);
        if (pCreateInfo->pAttachments) {
            localFBCI->pAttachments = new VkImageView[localFBCI->attachmentCount];
            memcpy((void*)localFBCI->pAttachments, pCreateInfo->pAttachments, localFBCI->attachmentCount*sizeof(VkImageView));
        }
        dev_data->frameBufferMap[*pFramebuffer] = localFBCI;
    }
    return result;
}

// Store the DAG.
struct DAGNode {
    uint32_t pass;
    std::vector<uint32_t> prev;
    std::vector<uint32_t> next;
};

VkBool32 FindDependency(const int index, const int dependent, const std::vector<DAGNode>& subpass_to_node, std::unordered_set<uint32_t>& processed_nodes) {
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

VkBool32 CheckDependencyExists(const layer_data* my_data, VkDevice device, const int subpass, const std::vector<uint32_t>& dependent_subpasses, const std::vector<DAGNode>& subpass_to_node, VkBool32& skip_call) {
    VkBool32 result = true;
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
                skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but only an implicit one is specified.",
                                     subpass, dependent_subpasses[k]);
            } else {
                skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but one is not specified.",
                                     subpass, dependent_subpasses[k]);
                result = false;
            }
        }
    }
    return result;
}

VkBool32 CheckPreserved(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const int index, const int attachment, const std::vector<DAGNode>& subpass_to_node, int depth, VkBool32& skip_call) {
    const DAGNode& node = subpass_to_node[index];
    // If this node writes to the attachment return true as next nodes need to preserve the attachment.
    const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        if (attachment == subpass.pColorAttachments[j].attachment)
            return VK_TRUE;
    }
    if (subpass.pDepthStencilAttachment &&
        subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
        if (attachment == subpass.pDepthStencilAttachment->attachment)
            return VK_TRUE;
    }
    VkBool32 result = VK_FALSE;
    // Loop through previous nodes and see if any of them write to the attachment.
    for (auto elem : node.prev) {
        result |= CheckPreserved(my_data, device, pCreateInfo, elem, attachment, subpass_to_node, depth + 1, skip_call);
    }
    // If the attachment was written to by a previous node than this node needs to preserve it.
    if (result && depth > 0) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
        VkBool32 has_preserved = false;
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            if (subpass.pPreserveAttachments[j].attachment == attachment) {
                has_preserved = true;
                break;
            }
        }
        if (!has_preserved) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Attachment %d is used by a later subpass and must be preserved in subpass %d.", attachment, index);
        }
    }
    return result;
}

VkBool32 ValidateDependencies(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, std::vector<DAGNode>& subpass_to_node) {
   VkBool32 skip_call = false;
    std::vector<std::vector<uint32_t>> output_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> input_attachment_to_subpass(pCreateInfo->attachmentCount);

    // Create DAG
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode& subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency& dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass > dependency.dstSubpass) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Dependency graph must be specified such that an earlier pass cannot depend on a later pass.");
        }
        subpass_to_node[dependency.dstSubpass].prev.push_back(dependency.srcSubpass);
        subpass_to_node[dependency.srcSubpass].next.push_back(dependency.dstSubpass);
    }

    // Find for each attachment the subpasses that use them.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            input_attachment_to_subpass[subpass.pInputAttachments[j].attachment].push_back(i);
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            output_attachment_to_subpass[subpass.pColorAttachments[j].attachment].push_back(i);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            output_attachment_to_subpass[subpass.pDepthStencilAttachment->attachment].push_back(i);
        }
    }
    // If there is a dependency needed make sure one exists
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        // If the attachment is an input then all subpasses that output must have a dependency relationship
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            const uint32_t& attachment = subpass.pInputAttachments[j].attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        // If the attachment is an output then all subpasses that use the attachment must have a dependency relationship
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            const uint32_t& attachment = subpass.pColorAttachments[j].attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(my_data, device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            const uint32_t& attachment = subpass.pDepthStencilAttachment->attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(my_data, device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
    }
    // Loop through implicit dependencies, if this pass reads make sure the attachment is preserved for all passes after it was written.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            CheckPreserved(my_data, device, pCreateInfo, i, subpass.pInputAttachments[j].attachment, subpass_to_node, 0, skip_call);
        }
    }
    return skip_call;
}

bool ValidateLayouts(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo) {
    bool skip = false;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            if (subpass.pInputAttachments[j].layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL &&
                subpass.pInputAttachments[j].layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                if (subpass.pInputAttachments[j].layout == VK_IMAGE_LAYOUT_GENERAL) {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is GENERAL but should be READ_ONLY_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is %d but can only be READ_ONLY_OPTIMAL or GENERAL.", subpass.pInputAttachments[j].attachment);
                }
            }
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            if (subpass.pColorAttachments[j].layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                if (subpass.pColorAttachments[j].layout == VK_IMAGE_LAYOUT_GENERAL) {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is GENERAL but should be COLOR_ATTACHMENT_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is %d but can only be COLOR_ATTACHMENT_OPTIMAL or GENERAL.", subpass.pColorAttachments[j].attachment);
                }
            }
        }
        if (subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            if (subpass.pDepthStencilAttachment->layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                if (subpass.pDepthStencilAttachment->layout == VK_IMAGE_LAYOUT_GENERAL) {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for depth attachment is GENERAL but should be DEPTH_STENCIL_ATTACHMENT_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for depth attachment is %d but can only be DEPTH_STENCIL_ATTACHMENT_OPTIMAL or GENERAL.", subpass.pDepthStencilAttachment->attachment);
                }
            }
        }
    }
    return skip;
}

bool CreatePassDAG(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, std::vector<DAGNode>& subpass_to_node, std::vector<bool>& has_self_dependency) {
    bool skip_call = false;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode& subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency& dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass > dependency.dstSubpass) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Depedency graph must be specified such that an earlier pass cannot depend on a later pass.");
        } else if (dependency.srcSubpass == dependency.dstSubpass) {
            has_self_dependency[dependency.srcSubpass] = true;
        }
        subpass_to_node[dependency.dstSubpass].prev.push_back(dependency.srcSubpass);
        subpass_to_node[dependency.srcSubpass].next.push_back(dependency.dstSubpass);
    }
    return skip_call;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Create DAG
    std::vector<bool> has_self_dependency(pCreateInfo->subpassCount);
    std::vector<DAGNode> subpass_to_node(pCreateInfo->subpassCount);
    skip_call |= CreatePassDAG(dev_data, device, pCreateInfo, subpass_to_node, has_self_dependency);
    // Validate using DAG
    skip_call |= ValidateDependencies(dev_data, device, pCreateInfo, subpass_to_node);
    skip_call |= ValidateLayouts(dev_data, device, pCreateInfo);
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED;
    }
    VkResult result = dev_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
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
                const uint32_t attachmentCount = subpass->inputAttachmentCount +
                    subpass->colorAttachmentCount * (1 + (subpass->pResolveAttachments?1:0)) +
                    ((subpass->pDepthStencilAttachment) ? 1 : 0) + subpass->preserveAttachmentCount;
                VkAttachmentReference *attachments = new VkAttachmentReference[attachmentCount];

                memcpy(attachments, subpass->pInputAttachments,
                        sizeof(attachments[0]) * subpass->inputAttachmentCount);
                subpass->pInputAttachments = attachments;
                attachments += subpass->inputAttachmentCount;

                memcpy(attachments, subpass->pColorAttachments,
                        sizeof(attachments[0]) * subpass->colorAttachmentCount);
                subpass->pColorAttachments = attachments;
                attachments += subpass->colorAttachmentCount;

                if (subpass->pResolveAttachments) {
                    memcpy(attachments, subpass->pResolveAttachments,
                            sizeof(attachments[0]) * subpass->colorAttachmentCount);
                    subpass->pResolveAttachments = attachments;
                    attachments += subpass->colorAttachmentCount;
                }

                if (subpass->pDepthStencilAttachment) {
                    memcpy(attachments, subpass->pDepthStencilAttachment,
                            sizeof(attachments[0]) * 1);
                    subpass->pDepthStencilAttachment = attachments;
                    attachments += 1;
                }

                memcpy(attachments, subpass->pPreserveAttachments,
                        sizeof(attachments[0]) * subpass->preserveAttachmentCount);
                subpass->pPreserveAttachments = attachments;
            }
        }
        if (pCreateInfo->pDependencies) {
            localRPCI->pDependencies = new VkSubpassDependency[localRPCI->dependencyCount];
            memcpy((void*)localRPCI->pDependencies, pCreateInfo->pDependencies, localRPCI->dependencyCount*sizeof(VkSubpassDependency));
        }
        dev_data->renderPassMap[*pRenderPass] = new RENDER_PASS_NODE();
        dev_data->renderPassMap[*pRenderPass]->hasSelfDependency = has_self_dependency;
        dev_data->renderPassMap[*pRenderPass]->createInfo = localRPCI;
    }
    return result;
}
// Free the renderpass shadow
static void deleteRenderPasses(layer_data* my_data)
{
    if (my_data->renderPassMap.size() <= 0)
        return;
    for (auto ii=my_data->renderPassMap.begin(); ii!=my_data->renderPassMap.end(); ++ii) {
        const VkRenderPassCreateInfo* pRenderPassInfo = (*ii).second->createInfo;
        if (pRenderPassInfo->pAttachments) {
            delete[] pRenderPassInfo->pAttachments;
        }
        if (pRenderPassInfo->pSubpasses) {
            for (uint32_t i=0; i<pRenderPassInfo->subpassCount; ++i) {
                // Attachements are all allocated in a block, so just need to
                //  find the first non-null one to delete
                if (pRenderPassInfo->pSubpasses[i].pInputAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pInputAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pColorAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pColorAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pResolveAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pResolveAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pPreserveAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pPreserveAttachments;
                }
            }
            delete[] pRenderPassInfo->pSubpasses;
        }
        if (pRenderPassInfo->pDependencies) {
            delete[] pRenderPassInfo->pDependencies;
        }
        delete (*ii).second;
    }
    my_data->renderPassMap.clear();
}

bool VerifyFramebufferAndRenderPassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    const VkRenderPassCreateInfo* pRenderPassInfo = dev_data->renderPassMap[pRenderPassBegin->renderPass]->createInfo;
    const VkFramebufferCreateInfo* pFramebufferInfo = dev_data->frameBufferMap[pRenderPassBegin->framebuffer];
    if (pRenderPassInfo->attachmentCount != pFramebufferInfo->attachmentCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                             "You cannot start a render pass using a framebuffer with a different number of attachments.");
    }
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[i];
        const VkImage& image = dev_data->imageViewMap[image_view]->image;
        auto image_data = pCB->imageLayoutMap.find(image);
        if (image_data == pCB->imageLayoutMap.end()) {
            pCB->imageLayoutMap[image].initialLayout = pRenderPassInfo->pAttachments[i].initialLayout;
            pCB->imageLayoutMap[image].layout = pRenderPassInfo->pAttachments[i].initialLayout;
        } else if (pRenderPassInfo->pAttachments[i].initialLayout != image_data->second.layout) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "You cannot start a render pass using attachment %i where the intial layout differs from the starting layout.", i);
        }
    }
    return skip_call;
}

void TransitionSubpassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, const int subpass_index) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto render_pass_data = dev_data->renderPassMap.find(pRenderPassBegin->renderPass);
    if (render_pass_data == dev_data->renderPassMap.end()) {
        return;
    }
    const VkRenderPassCreateInfo* pRenderPassInfo = render_pass_data->second->createInfo;
    auto framebuffer_data = dev_data->frameBufferMap.find(pRenderPassBegin->framebuffer);
    if (framebuffer_data == dev_data->frameBufferMap.end()) {
        return;
    }
    const VkFramebufferCreateInfo* pFramebufferInfo = framebuffer_data->second;
    const VkSubpassDescription& subpass = pRenderPassInfo->pSubpasses[subpass_index];
    for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pInputAttachments[j].attachment];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = subpass.pInputAttachments[j].layout;
            }
        }
    }
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pColorAttachments[j].attachment];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = subpass.pColorAttachments[j].layout;
            }
        }
    }
    if (subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pDepthStencilAttachment->attachment];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = subpass.pDepthStencilAttachment->layout;
            }
        }
    }
}

void TransitionFinalSubpassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto render_pass_data = dev_data->renderPassMap.find(pRenderPassBegin->renderPass);
    if (render_pass_data == dev_data->renderPassMap.end()) {
        return;
    }
    const VkRenderPassCreateInfo* pRenderPassInfo = render_pass_data->second->createInfo;
    auto framebuffer_data = dev_data->frameBufferMap.find(pRenderPassBegin->framebuffer);
    if (framebuffer_data == dev_data->frameBufferMap.end()) {
        return;
    }
    const VkFramebufferCreateInfo* pFramebufferInfo = framebuffer_data->second;
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[i];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = pRenderPassInfo->pAttachments[i].finalLayout;
            }
        }
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    skipCall |= VerifyFramebufferAndRenderPassLayouts(commandBuffer, pRenderPassBegin);
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pRenderPassBegin && pRenderPassBegin->renderPass) {
            skipCall |= insideRenderPass(dev_data, pCB, "vkCmdBeginRenderPass");
            updateCBTracking(pCB);
            skipCall |= addCmd(dev_data, pCB, CMD_BEGINRENDERPASS);
            pCB->activeRenderPass = pRenderPassBegin->renderPass;
            // This is a shallow copy as that is all that is needed for now
            pCB->activeRenderPassBeginInfo = *pRenderPassBegin;
            pCB->activeSubpass = 0;
            pCB->framebuffer = pRenderPassBegin->framebuffer;
            if (pCB->lastBoundPipeline) {
                skipCall |= validatePipelineState(dev_data, pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
            }
        } else {
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                    "You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);

    // This is a shallow copy as that is all that is needed for now
    dev_data->renderPassBeginInfo = *pRenderPassBegin;
    dev_data->currentSubpass = 0;
}

VK_LAYER_EXPORT void VKAPI vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    TransitionSubpassLayouts(commandBuffer, &dev_data->renderPassBeginInfo, ++dev_data->currentSubpass);
    if (pCB) {
        updateCBTracking(pCB);
        skipCall |= addCmd(dev_data, pCB, CMD_NEXTSUBPASS);
        pCB->activeSubpass++;
        TransitionSubpassLayouts(commandBuffer, &pCB->activeRenderPassBeginInfo, ++pCB->activeSubpass);
        if (pCB->lastBoundPipeline) {
            skipCall |= validatePipelineState(dev_data, pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdNextSubpass");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdNextSubpass(commandBuffer, contents);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    TransitionFinalSubpassLayouts(commandBuffer, &dev_data->renderPassBeginInfo);
    if (pCB) {
        skipCall |= outsideRenderPass(dev_data, pCB, "vkEndRenderpass");
        updateCBTracking(pCB);
        skipCall |= addCmd(dev_data, pCB, CMD_ENDRENDERPASS);
        TransitionFinalSubpassLayouts(commandBuffer, &pCB->activeRenderPassBeginInfo);
        pCB->activeRenderPass = 0;
        pCB->activeSubpass = 0;
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndRenderPass(commandBuffer);
}

VK_LAYER_EXPORT void VKAPI vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBuffersCount, const VkCommandBuffer* pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        GLOBAL_CB_NODE* pSubCB = NULL;
        for (uint32_t i=0; i<commandBuffersCount; i++) {
            pSubCB = getCBNode(dev_data, pCommandBuffers[i]);
            if (!pSubCB) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p in element %u of pCommandBuffers array.", (void*)pCommandBuffers[i], i);
            } else if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == pSubCB->createInfo.level) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ Primary Cmd Buffer %p in element %u of pCommandBuffers array. All cmd buffers in pCommandBuffers array must be secondary.", (void*)pCommandBuffers[i], i);
            }
        }
        updateCBTracking(pCB);
        skipCall |= addCmd(dev_data, pCB, CMD_EXECUTECOMMANDS);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdExecuteCommands(commandBuffer, commandBuffersCount, pCommandBuffers);
}

bool ValidateMapImageLayouts(VkDevice device, VkDeviceMemory mem) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto mem_data = dev_data->memImageMap.find(mem);
    if (mem_data != dev_data->memImageMap.end()) {
        auto image_data = dev_data->imageLayoutMap.find(mem_data->second);
        if (image_data != dev_data->imageLayoutMap.end()) {
            if (image_data->second->layout != VK_IMAGE_LAYOUT_PREINITIALIZED && image_data->second->layout != VK_IMAGE_LAYOUT_GENERAL) {
                skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                    "Cannot map an image with layout %d. Only GENERAL or PREINITIALIZED are supported.", image_data->second->layout);
            }
        }
    }
    return skip_call;
}

// clang-format off
VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = ValidateMapImageLayouts(device, mem);
    if (VK_FALSE == skip_call) {
        return dev_data->device_dispatch_table->MapMemory(device, mem, offset, size, flags, ppData);
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VkResult VKAPI vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->BindImageMemory(device, image, mem, memOffset);
    loader_platform_thread_lock_mutex(&globalLock);
    dev_data->memImageMap[mem] = image;
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSwapchainKHR(
    VkDevice                        device,
    const VkSwapchainCreateInfoKHR *pCreateInfo,
    VkSwapchainKHR                 *pSwapchain)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSwapchainKHR(device, pCreateInfo, pSwapchain);

    if (VK_SUCCESS == result) {
        SWAPCHAIN_NODE *swapchain_data = new SWAPCHAIN_NODE;
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->device_extensions.swapchainMap[*pSwapchain] = swapchain_data;
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySwapchainKHR(
    VkDevice                        device,
    VkSwapchainKHR swapchain)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain);

    loader_platform_thread_lock_mutex(&globalLock);
    auto swapchain_data = dev_data->device_extensions.swapchainMap.find(swapchain);
    if (swapchain_data != dev_data->device_extensions.swapchainMap.end()) {
        if (swapchain_data->second->images.size() > 0) {
            for (auto swapchain_image : swapchain_data->second->images) {
                auto image_item = dev_data->imageLayoutMap.find(swapchain_image);
                if (image_item != dev_data->imageLayoutMap.end())
                    dev_data->imageLayoutMap.erase(image_item);
            }
        }
        delete swapchain_data->second;
        dev_data->device_extensions.swapchainMap.erase(swapchain);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return dev_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSwapchainImagesKHR(
    VkDevice                device,
    VkSwapchainKHR          swapchain,
    uint32_t*               pCount,
    VkImage*                pSwapchainImages)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages != NULL) {
        // This should never happen and is checked by param checker.
        if (!pCount) return result;
        for (uint32_t i = 0; i < *pCount; ++i) {
            IMAGE_NODE* image_node = new IMAGE_NODE;
            image_node->layout = VK_IMAGE_LAYOUT_UNDEFINED;
            loader_platform_thread_lock_mutex(&globalLock);
            dev_data->device_extensions.swapchainMap[swapchain]->images.push_back(pSwapchainImages[i]);
            dev_data->imageLayoutMap[pSwapchainImages[i]] = image_node;
            loader_platform_thread_unlock_mutex(&globalLock);
        }
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR* pPresentInfo)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    bool skip_call = false;

    if (pPresentInfo) {
        for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
            auto swapchain_data = dev_data->device_extensions.swapchainMap.find(pPresentInfo->swapchains[i]);
            if (swapchain_data != dev_data->device_extensions.swapchainMap.end() && pPresentInfo->imageIndices[i] < swapchain_data->second->images.size()) {
                VkImage image = swapchain_data->second->images[pPresentInfo->imageIndices[i]];
                auto image_data = dev_data->imageLayoutMap.find(image);
                if (image_data != dev_data->imageLayoutMap.end()) {
                    if (image_data->second->layout != VK_IMAGE_LAYOUT_PRESENT_SOURCE_KHR) {
                        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, (uint64_t)queue, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                             "Images passed to present must be in layout PRESENT_SOURCE_KHR but is in %d", image_data->second->layout);
                    }
                }
            }
        }
    }

    if (VK_FALSE == skip_call)
        return dev_data->device_dispatch_table->QueuePresentKHR(queue, pPresentInfo);
    return VK_ERROR_VALIDATION_FAILED;
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

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VkCommandBuffer commandBuffer, const char* pMarker)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)commandBuffer, 0, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerBegin but extension disabled!");
        return;
    } else if (pCB) {
        updateCBTracking(pCB);
        skipCall |= addCmd(dev_data, pCB, CMD_DBGMARKERBEGIN);
    }
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(commandBuffer)->CmdDbgMarkerBegin(commandBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)commandBuffer, 0, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerEnd but extension disabled!");
        return;
    } else if (pCB) {
        updateCBTracking(pCB);
        skipCall |= addCmd(dev_data, pCB, CMD_DBGMARKEREND);
    }
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(commandBuffer)->CmdDbgMarkerEnd(commandBuffer);
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
    if (!strcmp(funcName, "vkFreeCommandBuffers"))
        return (PFN_vkVoidFunction) vkFreeCommandBuffers;
    if (!strcmp(funcName, "vkDestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(funcName, "vkDestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "CreatePipelineCache"))
        return (PFN_vkVoidFunction) vkCreatePipelineCache;
    if (!strcmp(funcName, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction) vkDestroyPipelineCache;
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
    if (!strcmp(funcName, "vkAllocateDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocateDescriptorSets;
    if (!strcmp(funcName, "vkFreeDescriptorSets"))
        return (PFN_vkVoidFunction) vkFreeDescriptorSets;
    if (!strcmp(funcName, "vkUpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(funcName, "vkAllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
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
    if (!strcmp(funcName, "vkCmdClearAttachments"))
        return (PFN_vkVoidFunction) vkCmdClearAttachments;
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
    if (!strcmp(funcName, "vkMapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;

    if (dev_data->device_extensions.wsi_enabled)
    {
        if (!strcmp(funcName, "vkCreateSwapchainKHR"))
            return (PFN_vkVoidFunction) vkCreateSwapchainKHR;
        if (!strcmp(funcName, "vkDestroySwapchainKHR"))
            return (PFN_vkVoidFunction) vkDestroySwapchainKHR;
        if (!strcmp(funcName, "vkGetSwapchainImagesKHR"))
            return (PFN_vkVoidFunction) vkGetSwapchainImagesKHR;
        if (!strcmp(funcName, "vkQueuePresentKHR"))
            return (PFN_vkVoidFunction) vkQueuePresentKHR;
    }

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
