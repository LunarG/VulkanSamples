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
#include "vk_layer.h"
#include <vector>
#include "vk_debug_report_lunarg.h"

using namespace std;

// Draw State ERROR codes
typedef enum _DRAW_STATE_ERROR
{
    DRAWSTATE_NONE,                             // Used for INFO & other non-error messages
    DRAWSTATE_INTERNAL_ERROR,                   // Error with DrawState internal data structures
    DRAWSTATE_NO_PIPELINE_BOUND,                // Unable to identify a bound pipeline
    DRAWSTATE_INVALID_POOL,                     // Invalid DS pool
    DRAWSTATE_INVALID_SET,                      // Invalid DS
    DRAWSTATE_INVALID_LAYOUT,                   // Invalid DS layout
    DRAWSTATE_INVALID_PIPELINE,                 // Invalid Pipeline handle referenced
    DRAWSTATE_INVALID_PIPELINE_CREATE_STATE,    // Attempt to create a pipeline with invalid state
    DRAWSTATE_INVALID_CMD_BUFFER,               // Invalid CmdBuffer referenced
    DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS,          // binding in vkCmdBindVertexData() too large for PSO's pVertexBindingDescriptions array
    DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR,        // binding offset in vkCmdBindIndexBuffer() out of alignment based on indexType
    //DRAWSTATE_MISSING_DOT_PROGRAM,              // No "dot" program in order to generate png image
    DRAWSTATE_OUT_OF_MEMORY,                    // malloc failed
    DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH,         // Type in layout vs. update are not the same
    DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS,  // Descriptors set for update out of bounds for corresponding layout section
    DRAWSTATE_INVALID_UPDATE_INDEX,             // Index of requested update is invalid for specified descriptors set
    DRAWSTATE_INVALID_UPDATE_STRUCT,            // Struct in DS Update tree is of invalid type
    DRAWSTATE_NUM_SAMPLES_MISMATCH,             // Number of samples in bound PSO does not match number in FB of current RenderPass
    DRAWSTATE_NO_END_CMD_BUFFER,                // Must call vkEndCommandBuffer() before QueueSubmit on that cmdBuffer
    DRAWSTATE_NO_BEGIN_CMD_BUFFER,              // Binding cmds or calling End on CB that never had vkBeginCommandBuffer() called on it
    DRAWSTATE_CMD_BUFFER_SINGLE_SUBMIT_VIOLATION, // Cmd Buffer created with VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT flag is submitted multiple times
    DRAWSTATE_INVALID_SECONDARY_CMD_BUFFER,     // vkCmdExecuteCommands() called with a primary cmdBuffer in pCmdBuffers array
    DRAWSTATE_VIEWPORT_NOT_BOUND,               // Draw submitted with no viewport state object bound
    DRAWSTATE_LINE_WIDTH_NOT_BOUND,             // Draw submitted with no line width state object bound
    DRAWSTATE_DEPTH_BIAS_NOT_BOUND,             // Draw submitted with no depth bias state object bound
    DRAWSTATE_BLEND_NOT_BOUND,                  // Draw submitted with no blend state object bound when color write enabled
    DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND,           // Draw submitted with no depth bounds state object bound when depth enabled
    DRAWSTATE_STENCIL_NOT_BOUND,                // Draw submitted with no stencil state object bound when stencil enabled
    DRAWSTATE_INDEX_BUFFER_NOT_BOUND,           // Draw submitted with no depth-stencil state object bound when depth write enabled
    DRAWSTATE_PIPELINE_LAYOUT_MISMATCH,         // Draw submitted PSO Pipeline layout that doesn't match layout from BindDescriptorSets
    DRAWSTATE_INVALID_RENDERPASS,               // Use of a NULL or otherwise invalid RenderPass object
    DRAWSTATE_INVALID_RENDERPASS_CMD,           // Invalid cmd submitted while a RenderPass is active
    DRAWSTATE_NO_ACTIVE_RENDERPASS,             // Rendering cmd submitted without an active RenderPass
    DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED,       // DescriptorSet bound but it was never updated. This is a warning code.
    DRAWSTATE_CLEAR_CMD_BEFORE_DRAW,            // Clear cmd issued before any Draw in CmdBuffer, should use RenderPass Ops instead
    DRAWSTATE_BEGIN_CB_INVALID_STATE,           // Primary/Secondary CB created with mismatched FB/RP information
    DRAWSTATE_INVALID_EXTENSION,
} DRAW_STATE_ERROR;

typedef enum _DRAW_TYPE
{
    DRAW                  = 0,
    DRAW_INDEXED          = 1,
    DRAW_INDIRECT         = 2,
    DRAW_INDEXED_INDIRECT = 3,
    DRAW_BEGIN_RANGE      = DRAW,
    DRAW_END_RANGE        = DRAW_INDEXED_INDIRECT,
    NUM_DRAW_TYPES        = (DRAW_END_RANGE - DRAW_BEGIN_RANGE + 1),
} DRAW_TYPE;

typedef struct _SHADER_DS_MAPPING {
    uint32_t slotCount;
    VkDescriptorSetLayoutCreateInfo* pShaderMappingSlot;
} SHADER_DS_MAPPING;

typedef struct _GENERIC_HEADER {
    VkStructureType sType;
    const void*    pNext;
} GENERIC_HEADER;

typedef struct _PIPELINE_NODE {
    VkPipeline                              pipeline;
    VkGraphicsPipelineCreateInfo            graphicsPipelineCI;
    VkPipelineVertexInputStateCreateInfo    vertexInputCI;
    VkPipelineInputAssemblyStateCreateInfo  iaStateCI;
    VkPipelineTessellationStateCreateInfo   tessStateCI;
    VkPipelineViewportStateCreateInfo       vpStateCI;
    VkPipelineRasterStateCreateInfo         rsStateCI;
    VkPipelineMultisampleStateCreateInfo    msStateCI;
    VkPipelineColorBlendStateCreateInfo     cbStateCI;
    VkPipelineDepthStencilStateCreateInfo   dsStateCI;
    VkPipelineShaderStageCreateInfo         vsCI;
    VkPipelineShaderStageCreateInfo         tcsCI;
    VkPipelineShaderStageCreateInfo         tesCI;
    VkPipelineShaderStageCreateInfo         gsCI;
    VkPipelineShaderStageCreateInfo         fsCI;
    // Compute shader is include in VkComputePipelineCreateInfo
    VkComputePipelineCreateInfo          computePipelineCI;
    // Flag of which shader stages are active for this pipeline
    uint32_t                             active_shaders;
    VkGraphicsPipelineCreateInfo*        pCreateTree;       // Ptr to shadow of data in create tree
    // Vtx input info (if any)
    uint32_t                             vtxBindingCount;   // number of bindings
    VkVertexInputBindingDescription*     pVertexBindingDescriptions;
    uint32_t                             vtxAttributeCount; // number of attributes
    VkVertexInputAttributeDescription*   pVertexAttributeDescriptions;
    uint32_t                             attachmentCount;   // number of CB attachments
    VkPipelineColorBlendAttachmentState* pAttachments;
} PIPELINE_NODE;

typedef struct _SAMPLER_NODE {
    VkSampler           sampler;
    VkSamplerCreateInfo createInfo;
} SAMPLER_NODE;

typedef struct _BUFFER_NODE {
    VkBufferView           buffer;
    VkBufferViewCreateInfo createInfo;
    VkDescriptorInfo       descriptorInfo;
} BUFFER_NODE;

// Descriptor Data structures
// Layout Node has the core layout data
typedef struct _LAYOUT_NODE {
    VkDescriptorSetLayout           layout;
    VkDescriptorType*               pTypes; // Dynamic array that will be created to verify descriptor types
    VkDescriptorSetLayoutCreateInfo createInfo;
    uint32_t                        startIndex; // 1st index of this layout
    uint32_t                        endIndex; // last index of this layout
} LAYOUT_NODE;
typedef struct _SET_NODE {
    VkDescriptorSet      set;
    VkDescriptorPool     pool;
    VkDescriptorSetUsage setUsage;
    // Head of LL of all Update structs for this set
    GENERIC_HEADER*      pUpdateStructs;
    // Total num of descriptors in this set (count of its layout plus all prior layouts)
    uint32_t             descriptorCount;
    GENERIC_HEADER**     ppDescriptors; // Array where each index points to update node for its slot
    LAYOUT_NODE*         pLayout; // Layout for this set
    struct _SET_NODE*    pNext;
} SET_NODE;

typedef struct _POOL_NODE {
    VkDescriptorPool           pool;
    VkDescriptorPoolUsage      poolUsage;
    uint32_t                   maxSets;
    VkDescriptorPoolCreateInfo createInfo;
    SET_NODE*                  pSets; // Head of LL of sets for this Pool
} POOL_NODE;

// Cmd Buffer Tracking
typedef enum _CMD_TYPE
{
    CMD_BINDPIPELINE,
    CMD_BINDPIPELINEDELTA,
    CMD_SETVIEWPORTSTATE,
    CMD_SETSCISSORSTATE,
    CMD_SETLINEWIDTHSTATE,
    CMD_SETDEPTHBIASSTATE,
    CMD_SETBLENDSTATE,
    CMD_SETDEPTHBOUNDSSTATE,
    CMD_SETSTENCILREADMASKSTATE,
    CMD_SETSTENCILWRITEMASKSTATE,
    CMD_SETSTENCILREFERENCESTATE,
    CMD_BINDDESCRIPTORSETS,
    CMD_BINDINDEXBUFFER,
    CMD_BINDVERTEXBUFFER,
    CMD_DRAW,
    CMD_DRAWINDEXED,
    CMD_DRAWINDIRECT,
    CMD_DRAWINDEXEDINDIRECT,
    CMD_DISPATCH,
    CMD_DISPATCHINDIRECT,
    CMD_COPYBUFFER,
    CMD_COPYIMAGE,
    CMD_BLITIMAGE,
    CMD_COPYBUFFERTOIMAGE,
    CMD_COPYIMAGETOBUFFER,
    CMD_CLONEIMAGEDATA,
    CMD_UPDATEBUFFER,
    CMD_FILLBUFFER,
    CMD_CLEARCOLORIMAGE,
    CMD_CLEARCOLORATTACHMENT,
    CMD_CLEARDEPTHSTENCILIMAGE,
    CMD_CLEARDEPTHSTENCILATTACHMENT,
    CMD_RESOLVEIMAGE,
    CMD_SETEVENT,
    CMD_RESETEVENT,
    CMD_WAITEVENTS,
    CMD_PIPELINEBARRIER,
    CMD_BEGINQUERY,
    CMD_ENDQUERY,
    CMD_RESETQUERYPOOL,
    CMD_WRITETIMESTAMP,
    CMD_INITATOMICCOUNTERS,
    CMD_LOADATOMICCOUNTERS,
    CMD_SAVEATOMICCOUNTERS,
    CMD_BEGINRENDERPASS,
    CMD_NEXTSUBPASS,
    CMD_ENDRENDERPASS,
    CMD_EXECUTECOMMANDS,
    CMD_DBGMARKERBEGIN,
    CMD_DBGMARKEREND,
} CMD_TYPE;
// Data structure for holding sequence of cmds in cmd buffer
typedef struct _CMD_NODE {
    CMD_TYPE          type;
    uint64_t          cmdNumber;
} CMD_NODE;

typedef enum _CB_STATE
{
    CB_NEW,            // Newly created CB w/o any cmds
    CB_UPDATE_ACTIVE,  // BeginCB has been called on this CB
    CB_UPDATE_COMPLETE // EndCB has been called on this CB
} CB_STATE;
// CB Status -- used to track status of various bindings on cmd buffer objects
typedef VkFlags CBStatusFlags;
typedef enum _CBStatusFlagBits
{
    CBSTATUS_NONE                              = 0x00000000, // No status is set
    CBSTATUS_VIEWPORT_SET                      = 0x00000001, // Viewport has been set
    CBSTATUS_LINE_WIDTH_SET                    = 0x00000002, // Line width has been set
    CBSTATUS_DEPTH_BIAS_SET                    = 0x00000004, // Depth bias has been set
    CBSTATUS_COLOR_BLEND_WRITE_ENABLE          = 0x00000008, // PSO w/ CB Enable set has been set
    CBSTATUS_BLEND_SET                         = 0x00000010, // Blend state object has been set
    CBSTATUS_DEPTH_WRITE_ENABLE                = 0x00000020, // PSO w/ Depth Enable set has been set
    CBSTATUS_STENCIL_TEST_ENABLE               = 0x00000040, // PSO w/ Stencil Enable set has been set
    CBSTATUS_DEPTH_BOUNDS_SET                  = 0x00000080, // Depth bounds state object has been set
    CBSTATUS_STENCIL_READ_MASK_SET             = 0x00000100, // Stencil read mask has been set
    CBSTATUS_STENCIL_WRITE_MASK_SET            = 0x00000200, // Stencil write mask has been set
    CBSTATUS_STENCIL_REFERENCE_SET             = 0x00000400, // Stencil reference has been set
    CBSTATUS_INDEX_BUFFER_BOUND                = 0x00000800, // Index buffer has been set
    CBSTATUS_SCISSOR_SET                       = 0x00001000, // Scissor has been set
} CBStatusFlagBits;

typedef struct stencil_data {
    uint32_t                     stencilCompareMask;
    uint32_t                     stencilWriteMask;
    uint32_t                     stencilReference;
} CBStencilData;

// Cmd Buffer Wrapper Struct
typedef struct _GLOBAL_CB_NODE {
    VkCmdBuffer                  cmdBuffer;
    VkCmdBufferCreateInfo        createInfo;
    VkCmdBufferBeginInfo         beginInfo;
    VkFence                      fence;    // fence tracking this cmd buffer
    uint64_t                     numCmds;  // number of cmds in this CB
    uint64_t                     drawCount[NUM_DRAW_TYPES]; // Count of each type of draw in this CB
    CB_STATE                     state;  // Track cmd buffer update state
    uint64_t                     submitCount; // Number of times CB has been submitted
    CBStatusFlags                status; // Track status of various bindings on cmd buffer
    vector<CMD_NODE*>            pCmds;
    // Currently storing "lastBound" objects on per-CB basis
    //  long-term may want to create caches of "lastBound" states and could have
    //  each individual CMD_NODE referencing its own "lastBound" state
    VkPipeline                   lastBoundPipeline;
    uint32_t                     lastVtxBinding;
    vector<VkViewport>           viewports;
    vector<VkRect2D>             scissors;
    float                        lineWidth;
    float                        depthBias;
    float                        depthBiasClamp;
    float                        slopeScaledDepthBias;
    float                        blendConst[4];
    float                        minDepthBounds;
    float                        maxDepthBounds;
    CBStencilData                front;
    CBStencilData                back;
    VkDescriptorSet              lastBoundDescriptorSet;
    VkPipelineLayout             lastBoundPipelineLayout;
    VkRenderPass                 activeRenderPass;
    uint32_t                     activeSubpass;
    VkFramebuffer                framebuffer;
    VkCmdBufferLevel             level;
    vector<VkDescriptorSet>      boundDescriptorSets;
} GLOBAL_CB_NODE;
