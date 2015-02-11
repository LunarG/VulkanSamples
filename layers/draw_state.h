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
#include "xglLayer.h"
// Draw State ERROR codes
typedef enum _DRAW_STATE_ERROR
{
    DRAWSTATE_NONE,                             // Used for INFO & other non-error messages
    DRAWSTATE_INTERNAL_ERROR,                   // Error with DrawState internal data structures
    DRAWSTATE_NO_PIPELINE_BOUND,                // Unable to identify a bound pipeline
    DRAWSTATE_INVALID_REGION,                   // Invalid DS region
    DRAWSTATE_INVALID_SET,                      // Invalid DS
    DRAWSTATE_INVALID_LAYOUT,                   // Invalid DS layout
    DRAWSTATE_DS_END_WITHOUT_BEGIN,             // EndDSUpdate called w/o corresponding BeginDSUpdate
    DRAWSTATE_UPDATE_WITHOUT_BEGIN,             // Attempt to update descriptors w/o calling BeginDescriptorRegionUpdate
    DRAWSTATE_INVALID_PIPELINE,                 // Invalid Pipeline referenced
    DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS,          // binding in xglCmdBindVertexData() too large for PSO's pVertexBindingDescriptions array
    DRAWSTATE_INVALID_DYNAMIC_STATE_OBJECT,     // Invalid dyn state object
    DRAWSTATE_MISSING_DOT_PROGRAM,              // No "dot" program in order to generate png image
    DRAWSTATE_BINDING_DS_NO_END_UPDATE,         // DS bound to CmdBuffer w/o call to xglEndDescriptorSetUpdate())
    DRAWSTATE_NO_DS_REGION,                     // No DS Region is available
    DRAWSTATE_OUT_OF_MEMORY,                    // malloc failed
    DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH,         // Type in layout vs. update are not the same
    DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS,  // Descriptors set for update out of bounds for corresponding layout section
    DRAWSTATE_INVALID_UPDATE_INDEX              // Index of requested update is invalid for specified descriptors set
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
    // TODO : Need to understand this with new binding model, changed to LAYOUT_CI for now
    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pShaderMappingSlot;
} SHADER_DS_MAPPING;

typedef struct _GENERIC_HEADER {
    XGL_STRUCTURE_TYPE sType;
    const void*    pNext;
} GENERIC_HEADER;

typedef struct _PIPELINE_NODE {
    XGL_PIPELINE           pipeline;
    struct _PIPELINE_NODE  *pNext;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO     *pCreateTree; // Ptr to shadow of data in create tree
    // Vtx input info (if any)
    uint32_t                                vtxBindingCount;   // number of bindings
    XGL_VERTEX_INPUT_BINDING_DESCRIPTION*   pVertexBindingDescriptions;
    uint32_t                                vtxAttributeCount; // number of attributes
    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pVertexAttributeDescriptions;
    uint32_t                                attachmentCount;   // number of CB attachments
    XGL_PIPELINE_CB_ATTACHMENT_STATE*       pAttachments;
} PIPELINE_NODE;

typedef struct _SAMPLER_NODE {
    XGL_SAMPLER              sampler;
    XGL_SAMPLER_CREATE_INFO  createInfo;
    struct _SAMPLER_NODE*    pNext;
} SAMPLER_NODE;

typedef struct _IMAGE_NODE {
    XGL_IMAGE_VIEW             image;
    XGL_IMAGE_VIEW_CREATE_INFO createInfo;
    XGL_IMAGE_VIEW_ATTACH_INFO attachInfo;
    struct _IMAGE_NODE*        pNext;
} IMAGE_NODE;

typedef struct _BUFFER_NODE {
    XGL_BUFFER_VIEW             buffer;
    XGL_BUFFER_VIEW_CREATE_INFO createInfo;
    XGL_BUFFER_VIEW_ATTACH_INFO attachInfo;
    struct _BUFFER_NODE*        pNext;
} BUFFER_NODE;

typedef struct _DYNAMIC_STATE_NODE {
    XGL_DYNAMIC_STATE_OBJECT     stateObj;
    GENERIC_HEADER   *pCreateInfo;
    struct _DYNAMIC_STATE_NODE *pNext;
} DYNAMIC_STATE_NODE;
// Descriptor Data structures
// Layout Node has the core layout data
typedef struct _LAYOUT_NODE {
    XGL_DESCRIPTOR_SET_LAYOUT                    layout;
    XGL_FLAGS                                    stageFlags;
    const uint32_t                               shaderStageBindPoints[XGL_NUM_SHADER_STAGE];
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfoList;
    uint32_t                                     startIndex; // 1st index of this layout
    uint32_t                                     endIndex; // last index of this layout
    struct _LAYOUT_NODE*                         pPriorSetLayout; // Points to node w/ priorSetLayout
    struct _LAYOUT_NODE*                         pNext; // Point to next layout in global LL chain of layouts
} LAYOUT_NODE;

typedef struct _SET_NODE {
    XGL_DESCRIPTOR_SET                           set;
    XGL_DESCRIPTOR_REGION                        region;
    XGL_DESCRIPTOR_SET_USAGE                     setUsage;
    // Head of LL of Update structs for this set
    GENERIC_HEADER*                              pUpdateStructs;
    // Total num of descriptors in this set (count of its layout plus all prior layouts)
    uint32_t                                     descriptorCount;
    LAYOUT_NODE*                                 pLayouts;
    struct _SET_NODE*                            pNext;
} SET_NODE;

typedef struct _REGION_NODE {
    XGL_DESCRIPTOR_REGION                        region;
    XGL_DESCRIPTOR_REGION_USAGE                  regionUsage;
    uint32_t                                     maxSets;
    const XGL_DESCRIPTOR_REGION_CREATE_INFO      createInfo;
    bool32_t                                     updateActive; // Track if Region is in an update block
    struct _REGION_NODE*                         pNext;
    SET_NODE*                                    pSets; // Head of LL of sets for this Region
} REGION_NODE;

//prototypes for extension functions
void drawStateDumpDotFile(char* outFileName);
void drawStateDumpPngFile(char* outFileName);
// Func ptr typedefs
typedef void (*DRAW_STATE_DUMP_DOT_FILE)(char*);
typedef void (*DRAW_STATE_DUMP_PNG_FILE)(char*);
