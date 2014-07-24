//
// File: mantleDbg.h
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.  All Rights Reserved.
//
// AMD is granting you permission to use this software for reference
// purposes only and not for use in any software product.
//
// You agree that you will not reverse engineer or decompile the Materials,
// in whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE
// WILL RUN UNINTERRUPTED OR ERROR-FREE OR WARRANTIES ARISING FROM CUSTOM OF
// TRADE OR COURSE OF USAGE.  THE ENTIRE RISK ASSOCIATED WITH THE USE OF THE
// SOFTWARE IS ASSUMED BY YOU.
// Some jurisdictions do not allow the exclusion of implied warranties, so
// the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL
// NOT, UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
// In no event shall AMD's total liability to You for all damages, losses,
// and causes of action (whether in contract, tort (including negligence) or
// otherwise) exceed the amount of $100 USD.  You agree to defend, indemnify
// and hold harmless AMD and its licensors, and any of their directors,
// officers, employees, affiliates or agents from and against any and all
// loss, damage, liability and other expenses (including reasonable attorneys'
// fees), resulting from Your use of the Software or violation of the terms and
// conditions of this Agreement.
//
// U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED
// RIGHTS." Use, duplication, or disclosure by the Government is subject to the
// restrictions as set forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or
// its successor.  Use of the Materials by the Government constitutes
// acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
// stated in the Software License Agreement.
//

#ifndef __MANTLEDBG_H__
#define __MANTLEDBG_H__

#include "mantle.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
***************************************************************************************************
*   Mantle debug and validation features
***************************************************************************************************
*/

typedef enum _GR_DBG_MSG_TYPE
{
    GR_DBG_MSG_UNKNOWN      = 0x00020000,
    GR_DBG_MSG_ERROR        = 0x00020001,
    GR_DBG_MSG_WARNING      = 0x00020002,
    GR_DBG_MSG_PERF_WARNING = 0x00020003,

    GR_DBG_MSG_TYPE_BEGIN_RANGE = GR_DBG_MSG_UNKNOWN,
    GR_DBG_MSG_TYPE_END_RANGE   = GR_DBG_MSG_PERF_WARNING,
    GR_NUM_DBG_MSG_TYPE         = (GR_DBG_MSG_TYPE_END_RANGE - GR_DBG_MSG_TYPE_BEGIN_RANGE + 1),
} GR_DBG_MSG_TYPE;

typedef enum _GR_DBG_GLOBAL_OPTION
{
    GR_DBG_OPTION_DEBUG_ECHO_ENABLE = 0x00020100,
    GR_DBG_OPTION_BREAK_ON_ERROR    = 0x00020101,
    GR_DBG_OPTION_BREAK_ON_WARNING  = 0x00020102,

    GR_DBG_GLOBAL_OPTION_BEGIN_RANGE = GR_DBG_OPTION_DEBUG_ECHO_ENABLE,
    GR_DBG_GLOBAL_OPTION_END_RANGE   = GR_DBG_OPTION_BREAK_ON_WARNING,
    GR_NUM_DBG_GLOBAL_OPTION         = (GR_DBG_GLOBAL_OPTION_END_RANGE - GR_DBG_GLOBAL_OPTION_BEGIN_RANGE + 1),
} GR_DBG_GLOBAL_OPTION;

typedef enum _GR_DBG_DEVICE_OPTION
{
    GR_DBG_OPTION_DISABLE_PIPELINE_LOADS      = 0x00020400,
    GR_DBG_OPTION_FORCE_OBJECT_MEMORY_REQS    = 0x00020401,
    GR_DBG_OPTION_FORCE_LARGE_IMAGE_ALIGNMENT = 0x00020402,

    GR_DBG_DEVICE_OPTION_BEGIN_RANGE = GR_DBG_OPTION_DISABLE_PIPELINE_LOADS,
    GR_DBG_DEVICE_OPTION_END_RANGE   = GR_DBG_OPTION_FORCE_LARGE_IMAGE_ALIGNMENT,
    GR_NUM_DBG_DEVICE_OPTION         = (GR_DBG_DEVICE_OPTION_END_RANGE - GR_DBG_DEVICE_OPTION_BEGIN_RANGE + 1),
} GR_DBG_DEVICE_OPTION;

typedef enum _GR_DBG_MSG_FILTER
{
    GR_DBG_MSG_FILTER_NONE     = 0x00020800,
    GR_DBG_MSG_FILTER_REPEATED = 0x00020801,
    GR_DBG_MSG_FILTER_ALL      = 0x00020802,

    GR_DBG_MSG_FILTER_BEGIN_RANGE = GR_DBG_MSG_FILTER_NONE,
    GR_DBG_MSG_FILTER_END_RANGE   = GR_DBG_MSG_FILTER_ALL,
    GR_NUM_DBG_MSG_FILTER         = (GR_DBG_MSG_FILTER_END_RANGE - GR_DBG_MSG_FILTER_BEGIN_RANGE + 1),
} GR_DBG_MSG_FILTER;

typedef enum _GR_DBG_DATA_TYPE
{
    // Common object debug data
    GR_DBG_DATA_OBJECT_TYPE          = 0x00020a00,
    GR_DBG_DATA_OBJECT_CREATE_INFO   = 0x00020a01,
    GR_DBG_DATA_OBJECT_TAG           = 0x00020a02,
    // Command buffer specific debug data
    GR_DBG_DATA_CMD_BUFFER_API_TRACE = 0x00020b00,
    // Memory object specific debug data
    GR_DBG_DATA_MEMORY_OBJECT_LAYOUT = 0x00020c00,
    GR_DBG_DATA_MEMORY_OBJECT_STATE  = 0x00020c01,
} GR_DBG_DATA_TYPE;

typedef enum _GR_DBG_OBJECT_TYPE
{
    GR_DBG_OBJECT_UNKNOWN                = 0x00020900,
    GR_DBG_OBJECT_DEVICE                 = 0x00020901,
    GR_DBG_OBJECT_QUEUE                  = 0x00020902,
    GR_DBG_OBJECT_GPU_MEMORY             = 0x00020903,
    GR_DBG_OBJECT_IMAGE                  = 0x00020904,
    GR_DBG_OBJECT_IMAGE_VIEW             = 0x00020905,
    GR_DBG_OBJECT_COLOR_TARGET_VIEW      = 0x00020906,
    GR_DBG_OBJECT_DEPTH_STENCIL_VIEW     = 0x00020907,
    GR_DBG_OBJECT_SHADER                 = 0x00020908,
    GR_DBG_OBJECT_GRAPHICS_PIPELINE      = 0x00020909,
    GR_DBG_OBJECT_COMPUTE_PIPELINE       = 0x0002090a,
    GR_DBG_OBJECT_SAMPLER                = 0x0002090b,
    GR_DBG_OBJECT_DESCRIPTOR_SET         = 0x0002090c,
    GR_DBG_OBJECT_VIEWPORT_STATE         = 0x0002090d,
    GR_DBG_OBJECT_RASTER_STATE           = 0x0002090e,
    GR_DBG_OBJECT_MSAA_STATE             = 0x0002090f,
    GR_DBG_OBJECT_COLOR_BLEND_STATE      = 0x00020910,
    GR_DBG_OBJECT_DEPTH_STENCIL_STATE    = 0x00020911,
    GR_DBG_OBJECT_CMD_BUFFER             = 0x00020912,
    GR_DBG_OBJECT_FENCE                  = 0x00020913,
    GR_DBG_OBJECT_QUEUE_SEMAPHORE        = 0x00020914,
    GR_DBG_OBJECT_EVENT                  = 0x00020915,
    GR_DBG_OBJECT_QUERY_POOL             = 0x00020916,
    GR_DBG_OBJECT_SHARED_GPU_MEMORY      = 0x00020917,
    GR_DBG_OBJECT_SHARED_QUEUE_SEMAPHORE = 0x00020918,
    GR_DBG_OBJECT_PEER_GPU_MEMORY        = 0x00020919,
    GR_DBG_OBJECT_PEER_IMAGE             = 0x0002091a,
    GR_DBG_OBJECT_PINNED_GPU_MEMORY      = 0x0002091b,
    GR_DBG_OBJECT_INTERNAL_GPU_MEMORY    = 0x0002091c,

    GR_DBG_OBJECT_TYPE_BEGIN_RANGE = GR_DBG_OBJECT_UNKNOWN,
    GR_DBG_OBJECT_TYPE_END_RANGE   = GR_DBG_OBJECT_INTERNAL_GPU_MEMORY,
    GR_NUM_DBG_OBJECT_TYPE         = (GR_DBG_OBJECT_TYPE_END_RANGE - GR_DBG_OBJECT_TYPE_BEGIN_RANGE + 1),
} GR_DBG_OBJECT_TYPE;

// ------------------------------------------------------------------------------------------------
// Memory object layout reflection

typedef struct _GR_DBG_MEMORY_OBJECT_LAYOUT_REGION
{
    GR_GPU_SIZE offset;
    GR_GPU_SIZE regionSize;
    GR_OBJECT   boundObject;
} GR_DBG_MEMORY_OBJECT_LAYOUT_REGION;

typedef struct _GR_DBG_MEMORY_OBJECT_LAYOUT
{
    GR_UINT                            regionCount;
    GR_DBG_MEMORY_OBJECT_LAYOUT_REGION regions[1];
    // (regionCount-1) more GR_DBG_MEMORY_OBJECT_LAYOUT_REGION structures to follow...
} GR_DBG_MEMORY_OBJECT_LAYOUT;

typedef struct _GR_DBG_MEMORY_OBJECT_STATE_REGION
{
    GR_GPU_SIZE offset;
    GR_GPU_SIZE regionSize;
    GR_ENUM     state;                  // GR_MEMORY_STATE
} GR_DBG_MEMORY_OBJECT_STATE_REGION;

typedef struct _GR_DBG_MEMORY_OBJECT_STATE
{
    GR_UINT                           regionCount;
    GR_DBG_MEMORY_OBJECT_STATE_REGION regions[1];
    // (regionCount-1) more GR_DBG_MEMORY_OBJECT_STATE_REGION structures to follow...
} GR_DBG_MEMORY_OBJECT_STATE;

// ------------------------------------------------------------------------------------------------
// Command buffer packet reflection

// Command buffer packet opcodes
typedef enum _GR_DBG_CMD_BUFFER_OPCODE
{
    GR_DBG_OP_CMD_BUFFER_END                = 0x00028000,
    GR_DBG_OP_CMD_BIND_PIPELINE             = 0x00028001,
    GR_DBG_OP_CMD_BIND_STATE_OBJECT         = 0x00028002,
    GR_DBG_OP_CMD_BIND_DESCRIPTOR_SET       = 0x00028003,
    GR_DBG_OP_CMD_BIND_DYNAMIC_MEMORY_VIEW  = 0x00028004,
    GR_DBG_OP_CMD_BIND_INDEX_DATA           = 0x00028005,
    GR_DBG_OP_CMD_BIND_TARGETS              = 0x00028006,
    GR_DBG_OP_CMD_PREPARE_MEMORY_REGIONS    = 0x00028007,
    GR_DBG_OP_CMD_PREPARE_IMAGES            = 0x00028008,
    GR_DBG_OP_CMD_DRAW                      = 0x00028009,
    GR_DBG_OP_CMD_DRAW_INDEXED              = 0x0002800a,
    GR_DBG_OP_CMD_DRAW_INDIRECT             = 0x0002800b,
    GR_DBG_OP_CMD_DRAW_INDEXED_INDIRECT     = 0x0002800c,
    GR_DBG_OP_CMD_DISPATCH                  = 0x0002800d,
    GR_DBG_OP_CMD_DISPATCH_INDIRECT         = 0x0002800e,
    GR_DBG_OP_CMD_COPY_MEMORY               = 0x0002800f,
    GR_DBG_OP_CMD_COPY_IMAGE                = 0x00028010,
    GR_DBG_OP_CMD_COPY_MEMORY_TO_IMAGE      = 0x00028011,
    GR_DBG_OP_CMD_COPY_IMAGE_TO_MEMORY      = 0x00028012,
    GR_DBG_OP_CMD_CLONE_IMAGE_DATA          = 0x00028013,
    GR_DBG_OP_CMD_FILL_MEMORY               = 0x00028014,
    GR_DBG_OP_CMD_UPDATE_MEMORY             = 0x00028015,
    GR_DBG_OP_CMD_CLEAR_COLOR_IMAGE         = 0x00028016,
    GR_DBG_OP_CMD_CLEAR_COLOR_IMAGE_RAW     = 0x00028017,
    GR_DBG_OP_CMD_CLEAR_DEPTH_STENCIL       = 0x00028018,
    GR_DBG_OP_CMD_RESOLVE_IMAGE             = 0x00028019,
    GR_DBG_OP_CMD_SET_EVENT                 = 0x0002801a,
    GR_DBG_OP_CMD_RESET_EVENT               = 0x0002801b,
    GR_DBG_OP_CMD_MEMORY_ATOMIC             = 0x0002801c,
    GR_DBG_OP_CMD_BEGIN_QUERY               = 0x0002801d,
    GR_DBG_OP_CMD_END_QUERY                 = 0x0002801e,
    GR_DBG_OP_CMD_RESET_QUERY_POOL          = 0x0002801f,
    GR_DBG_OP_CMD_WRITE_TIMESTAMP           = 0x00028020,
    GR_DBG_OP_CMD_INIT_ATOMIC_COUNTERS      = 0x00028021,
    GR_DBG_OP_CMD_LOAD_ATOMIC_COUNTERS      = 0x00028022,
    GR_DBG_OP_CMD_SAVE_ATOMIC_COUNTERS      = 0x00028023,
    GR_DBG_OP_CMD_DBG_MARKER_BEGIN          = 0x00028024,
    GR_DBG_OP_CMD_DBG_MARKER_END            = 0x00028025,

    GR_DBG_CMD_BUFFER_OPCODE_BEGIN_RANGE = GR_DBG_OP_CMD_BUFFER_END,
    GR_DBG_CMD_BUFFER_OPCODE_END_RANGE   = GR_DBG_OP_CMD_DBG_MARKER_END,
    GR_NUM_DBG_CMD_BUFFER_OPCODE         = (GR_DBG_CMD_BUFFER_OPCODE_END_RANGE - GR_DBG_CMD_BUFFER_OPCODE_BEGIN_RANGE + 1),
} GR_DBG_CMD_BUFFER_OPCODE;

// Command buffer packet header
typedef struct _GR_DBG_OP_HEADER
{
    GR_UINT recordSize;
    GR_ENUM opCode;                     // GR_DBG_CMD_BUFFER_OPCODE
    // Op-specific variable size data follows...
} GR_DBG_OP_HEADER;

// Memory binding reference for image objects at the time of command buffer recording
typedef struct _GR_DBG_BOUND_MEMORY_REF
{
    GR_GPU_MEMORY mem;
    GR_GPU_SIZE   offset;
} GR_DBG_BOUND_MEMORY_REF;

typedef struct _GR_DBG_OBJECT_REF
{
    GR_OBJECT               object;
    GR_DBG_BOUND_MEMORY_REF memoryRef;
} GR_DBG_OBJECT_REF;

typedef struct _GR_DBG_VIEW_REF
{
    GR_DBG_OBJECT_REF       view;
    // Memory reference for image of the color target view
    GR_DBG_BOUND_MEMORY_REF imageMemoryRef;
} GR_DBG_VIEW_REF;

// Command buffer payload packets
typedef struct _GR_DBG_OP_DATA_CMD_BIND_PIPELINE
{
    GR_ENUM           pipelineBindPoint; // GR_PIPELINE_BIND_POINT
    GR_DBG_OBJECT_REF pipeline;
} GR_DBG_OP_DATA_CMD_BIND_PIPELINE;

typedef struct _GR_DBG_OP_DATA_CMD_BIND_STATE_OBJECT
{
    GR_ENUM           stateBindPoint;   // GR_STATE_BIND_POINT
    GR_DBG_OBJECT_REF state;
} GR_DBG_OP_DATA_CMD_BIND_STATE_OBJECT;

typedef struct _GR_DBG_OP_DATA_CMD_BIND_DESCRIPTOR_SET
{
    GR_ENUM           pipelineBindPoint; // GR_PIPELINE_BIND_POINT
    GR_UINT           index;
    GR_DBG_OBJECT_REF descriptorSet;
    GR_UINT           slotOffset;
} GR_DBG_OP_DATA_CMD_BIND_DESCRIPTOR_SET;

typedef struct _GR_DBG_OP_DATA_CMD_BIND_DYNAMIC_MEMORY_VIEW
{
    GR_ENUM                    pipelineBindPoint; // GR_PIPELINE_BIND_POINT
    GR_MEMORY_VIEW_ATTACH_INFO view;
} GR_DBG_OP_DATA_CMD_BIND_DYNAMIC_MEMORY_VIEW;

typedef struct _GR_DBG_OP_DATA_CMD_BIND_INDEX_DATA
{
    GR_GPU_MEMORY mem;
    GR_GPU_SIZE   offset;
    GR_ENUM       indexType;            // GR_INDEX_TYPE
} GR_DBG_OP_DATA_CMD_BIND_INDEX_DATA;

typedef struct _GR_DBG_COLOR_TARGET_BIND_INFO
{
    GR_DBG_VIEW_REF view;
    GR_ENUM         colorTargetState;   // GR_IMAGE_STATE
} GR_DBG_COLOR_TARGET_BIND_INFO;

typedef struct _GR_DBG_DEPTH_STENCIL_BIND_INFO
{
    GR_DBG_VIEW_REF view;
    GR_ENUM         depthState;         // GR_IMAGE_STATE
    GR_ENUM         stencilState;       // GR_IMAGE_STATE
} GR_DBG_DEPTH_STENCIL_BIND_INFO;

typedef struct _GR_DBG_OP_DATA_CMD_BIND_TARGETS
{
    GR_UINT                        colorTargetCount;
    GR_DBG_COLOR_TARGET_BIND_INFO  colorTargetData[GR_MAX_COLOR_TARGETS];
    GR_DBG_DEPTH_STENCIL_BIND_INFO depthTargetData;
} GR_DBG_OP_DATA_CMD_BIND_TARGETS;

typedef struct _GR_DBG_OP_DATA_CMD_PREPARE_MEMORY_REGIONS
{
    GR_UINT                    transitionCount;
    GR_MEMORY_STATE_TRANSITION stateTransitions[1];
    // (transitionCount-1) more GR_MEMORY_STATE_TRANSITION structures to follow...
} GR_DBG_OP_DATA_CMD_PREPARE_MEMORY_REGIONS;

typedef struct _GR_DBG_IMAGE_STATE_TRANSITION
{
    GR_DBG_OBJECT_REF          image;
    GR_ENUM                    oldState; // GR_IMAGE_STATE
    GR_ENUM                    newState; // GR_IMAGE_STATE
    GR_IMAGE_SUBRESOURCE_RANGE subresourceRange;
} GR_DBG_IMAGE_STATE_TRANSITION;

typedef struct _GR_DBG_OP_DATA_CMD_PREPARE_IMAGES
{
    GR_UINT                       transitionCount;
    GR_DBG_IMAGE_STATE_TRANSITION stateTransitions[1];
    // (transitionCount-1) more GR_DBG_IMAGE_STATE_TRANSITION_DATA structures to follow...
} GR_DBG_OP_DATA_CMD_PREPARE_IMAGES;

typedef struct _GR_DBG_OP_DATA_CMD_DRAW
{
    GR_UINT firstVertex;
    GR_UINT vertexCount;
    GR_UINT firstInstance;
    GR_UINT instanceCount;
} GR_DBG_OP_DATA_CMD_DRAW;

typedef struct _GR_DBG_OP_DATA_CMD_DRAW_INDEXED
{
    GR_UINT firstIndex;
    GR_UINT indexCount;
    GR_INT  vertexOffset;
    GR_UINT firstInstance;
    GR_UINT instanceCount;
} GR_DBG_OP_DATA_CMD_DRAW_INDEXED;

typedef struct _GR_DBG_OP_DATA_CMD_DRAW_INDIRECT
{
    GR_GPU_MEMORY mem;
    GR_GPU_SIZE   offset;
} GR_DBG_OP_DATA_CMD_DRAW_INDIRECT;

typedef struct _GR_DBG_OP_DATA_CMD_DRAW_INDEXED_INDIRECT
{
    GR_GPU_MEMORY mem;
    GR_GPU_SIZE   offset;
} GR_DBG_OP_DATA_CMD_DRAW_INDEXED_INDIRECT;

typedef struct _GR_DBG_OP_DATA_CMD_DISPATCH
{
    GR_UINT x;
    GR_UINT y;
    GR_UINT z;
} GR_DBG_OP_DATA_CMD_DISPATCH;

typedef struct _GR_DBG_OP_DATA_CMD_DISPATCH_INDIRECT
{
    GR_GPU_MEMORY mem;
    GR_GPU_SIZE   offset;
} GR_DBG_OP_DATA_CMD_DISPATCH_INDIRECT;

typedef struct _GR_DBG_OP_DATA_CMD_COPY_MEMORY
{
    GR_GPU_MEMORY  srcMem;
    GR_GPU_MEMORY  destMem;
    GR_UINT        regionCount;
    GR_MEMORY_COPY regions[1];
    // (regionCount-1) more GR_MEMORY_COPY structures to follow...
} GR_DBG_OP_DATA_CMD_COPY_MEMORY;

typedef struct _GR_DBG_OP_DATA_CMD_COPY_IMAGE
{
    GR_DBG_OBJECT_REF srcImage;
    GR_DBG_OBJECT_REF destImage;
    GR_UINT           regionCount;
    GR_IMAGE_COPY     regions[1];
    // (regionCount-1) more GR_IMAGE_COPY structures to follow...
} GR_DBG_OP_DATA_CMD_COPY_IMAGE;

typedef struct _GR_DBG_OP_DATA_CMD_COPY_MEMORY_TO_IMAGE
{
    GR_GPU_MEMORY        srcMem;
    GR_DBG_OBJECT_REF    destImage;
    GR_UINT              regionCount;
    GR_MEMORY_IMAGE_COPY regions[1];
    // (regionCount-1) more GR_MEMORY_IMAGE_COPY structures to follow...
} GR_DBG_OP_DATA_CMD_COPY_MEMORY_TO_IMAGE;

typedef struct _GR_DBG_OP_DATA_CMD_COPY_IMAGE_TO_MEMORY
{
    GR_DBG_OBJECT_REF    srcImage;
    GR_GPU_MEMORY        destMem;
    GR_UINT              regionCount;
    GR_MEMORY_IMAGE_COPY regions[1];
    // (regionCount-1) more GR_MEMORY_IMAGE_COPY structures to follow...
} GR_DBG_OP_DATA_CMD_COPY_IMAGE_TO_MEMORY;

typedef struct _GR_DBG_OP_DATA_CMD_CLONE_IMAGE_DATA
{
    GR_DBG_OBJECT_REF srcImage;
    GR_ENUM           srcImageState;    // GR_IMAGE_STATE
    GR_DBG_OBJECT_REF destImage;
    GR_ENUM           destImageState;   // GR_IMAGE_STATE
} GR_DBG_OP_DATA_CMD_CLONE_IMAGE_DATA;

typedef struct _GR_DBG_OP_DATA_CMD_UPDATE_MEMORY
{
    GR_GPU_MEMORY destMem;
    GR_GPU_SIZE   destOffset;
    GR_GPU_SIZE   dataSize;
    GR_UINT32     data[1];
    // (dataSize-4) more bytes of data to follow...
} GR_DBG_OP_DATA_CMD_UPDATE_MEMORY;

typedef struct _GR_DBG_OP_DATA_CMD_FILL_MEMORY
{
    GR_GPU_MEMORY destMem;
    GR_GPU_SIZE   destOffset;
    GR_GPU_SIZE   fillSize;
    GR_UINT32     data;
} GR_DBG_OP_DATA_CMD_FILL_MEMORY;

typedef struct _GR_DBG_OP_DATA_CMD_CLEAR_COLOR_IMAGE
{
    GR_DBG_OBJECT_REF          image;
    GR_FLOAT                   color[4];
    GR_UINT                    rangeCount;
    GR_IMAGE_SUBRESOURCE_RANGE ranges[1];
    // (rangeCount-1) more GR_IMAGE_SUBRESOURCE_RANGE structures to follow...
} GR_DBG_OP_DATA_CMD_CLEAR_COLOR_IMAGE;

typedef struct _GR_DBG_OP_DATA_CMD_CLEAR_COLOR_IMAGE_RAW
{
    GR_DBG_OBJECT_REF          image;
    GR_UINT32                  color[4];
    GR_UINT                    rangeCount;
    GR_IMAGE_SUBRESOURCE_RANGE ranges[1];
    // (rangeCount-1) more GR_IMAGE_SUBRESOURCE_RANGE structures to follow...
} GR_DBG_OP_DATA_CMD_CLEAR_COLOR_IMAGE_RAW;

typedef struct _GR_DBG_OP_DATA_CMD_CLEAR_DEPTH_STENCIL
{
    GR_DBG_OBJECT_REF          image;
    GR_FLOAT                   depth;
    GR_UINT8                   stencil;
    GR_UINT                    rangeCount;
    GR_IMAGE_SUBRESOURCE_RANGE ranges[1];
    // (rangeCount-1) more GR_IMAGE_SUBRESOURCE_RANGE structures to follow...
} GR_DBG_OP_DATA_CMD_CLEAR_DEPTH_STENCIL;

typedef struct _GR_DBG_OP_DATA_CMD_RESOLVE_IMAGE
{
    GR_DBG_OBJECT_REF srcImage;
    GR_DBG_OBJECT_REF destImage;
    GR_UINT           rectCount;
    GR_IMAGE_RESOLVE  rects[1];
    // (rectCount-1) more GR_IMAGE_RESOLVE structures to follow...
} GR_DBG_OP_DATA_CMD_RESOLVE_IMAGE;

typedef struct _GR_DBG_OP_DATA_CMD_SET_EVENT
{
    GR_DBG_OBJECT_REF event;
} GR_DBG_OP_DATA_CMD_SET_EVENT;

typedef struct _GR_DBG_OP_DATA_CMD_RESET_EVENT
{
    GR_DBG_OBJECT_REF event;
} GR_DBG_OP_DATA_CMD_RESET_EVENT;

typedef struct _GR_DBG_OP_DATA_CMD_MEMORY_ATOMIC
{
    GR_GPU_MEMORY destMem;
    GR_GPU_SIZE   destOffset;
    GR_UINT64     srcData;
    GR_ENUM       atomicOp;             // GR_ATOMIC_OP
} GR_DBG_OP_DATA_CMD_MEMORY_ATOMIC;

typedef struct _GR_DBG_OP_DATA_CMD_BEGIN_QUERY
{
    GR_DBG_OBJECT_REF queryPool;
    GR_UINT           slot;
    GR_FLAGS          flags;
} GR_DBG_OP_DATA_CMD_BEGIN_QUERY;

typedef struct _GR_DBG_OP_DATA_CMD_END_QUERY
{
    GR_DBG_OBJECT_REF queryPool;
    GR_UINT           slot;
} GR_DBG_OP_DATA_CMD_END_QUERY;

typedef struct _GR_DBG_OP_DATA_CMD_RESET_QUERY_POOL
{
    GR_DBG_OBJECT_REF queryPool;
    GR_UINT           startQuery;
    GR_UINT           queryCount;
} GR_DBG_OP_DATA_CMD_RESET_QUERY_POOL;

typedef struct _GR_DBG_OP_DATA_CMD_WRITE_TIMESTAMP
{
    GR_ENUM       timestampType;        // GR_TIMESTAMP_TYPE
    GR_GPU_MEMORY destMem;
    GR_GPU_SIZE   destOffset;
} GR_DBG_OP_DATA_CMD_WRITE_TIMESTAMP;

typedef struct _GR_DBG_OP_DATA_CMD_INIT_ATOMIC_COUNTERS
{
    GR_ENUM   pipelineBindPoint;        // GR_PIPELINE_BIND_POINT
    GR_UINT   startCounter;
    GR_UINT   counterCount;
    GR_UINT32 data[1];
    // (counterCount-1) more DWORDs of data to follow...
} GR_DBG_OP_DATA_CMD_INIT_ATOMIC_COUNTERS;

typedef struct _GR_DBG_OP_DATA_CMD_LOAD_ATOMIC_COUNTERS
{
    GR_ENUM       pipelineBindPoint;    // GR_PIPELINE_BIND_POINT
    GR_UINT       startCounter;
    GR_UINT       counterCount;
    GR_GPU_MEMORY srcMem;
    GR_GPU_SIZE   srcOffset;
} GR_DBG_OP_DATA_CMD_LOAD_ATOMIC_COUNTERS;

typedef struct _GR_DBG_OP_DATA_CMD_SAVE_ATOMIC_COUNTERS
{
    GR_ENUM       pipelineBindPoint;    // GR_PIPELINE_BIND_POINT
    GR_UINT       startCounter;
    GR_UINT       counterCount;
    GR_GPU_MEMORY destMem;
    GR_GPU_SIZE   destOffset;
} GR_DBG_OP_DATA_CMD_SAVE_ATOMIC_COUNTERS;

typedef struct _GR_DBG_OP_DATA_CMD_DBG_MARKER_BEGIN
{
    GR_CHAR marker[1];
    // The rest of null terminated string follows, up to the size of the packet...
} GR_DBG_OP_DATA_CMD_DBG_MARKER_BEGIN;

typedef struct _GR_DBG_OP_DATA_CMD_DBG_MARKER_END
{
    GR_UINT _reserved;
} GR_DBG_OP_DATA_CMD_DBG_MARKER_END;

// ------------------------------------------------------------------------------------------------
// Debug message callback

typedef GR_VOID (GR_STDCALL *GR_DBG_MSG_CALLBACK_FUNCTION)(
    GR_ENUM        msgType,             // GR_DBG_MSG_TYPE
    GR_ENUM        validationLevel,     // GR_VALIDATION_LEVEL
    GR_BASE_OBJECT srcObject,
    GR_SIZE        location,
    GR_ENUM        msgCode,             // GR_DBG_MSG_CODE
    const GR_CHAR* pMsg,
    GR_VOID*       pUserData);

// ------------------------------------------------------------------------------------------------
// Debug functions

GR_RESULT GR_STDCALL grDbgSetValidationLevel(
    GR_DEVICE device,
    GR_ENUM   validationLevel);         // GR_VALIDATION_LEVEL

GR_RESULT GR_STDCALL grDbgRegisterMsgCallback(
    GR_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback,
    GR_VOID*                     pUserData);

GR_RESULT GR_STDCALL grDbgUnregisterMsgCallback(
    GR_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);

GR_RESULT GR_STDCALL grDbgSetMessageFilter(
    GR_DEVICE device,
    GR_ENUM   msgCode,                  // GR_DBG_MSG_CODE
    GR_ENUM   filter);                  // GR_DBG_MSG_FILTER

GR_RESULT GR_STDCALL grDbgSetObjectTag(
    GR_BASE_OBJECT object,
    GR_SIZE        tagSize,
    const GR_VOID* pTag);

GR_RESULT GR_STDCALL grDbgSetGlobalOption(
    GR_ENUM        dbgOption,           // GR_DBG_GLOBAL_OPTION
    GR_SIZE        dataSize,
    const GR_VOID* pData);

GR_RESULT GR_STDCALL grDbgSetDeviceOption(
    GR_DEVICE      device,
    GR_ENUM        dbgOption,           // GR_DBG_DEVICE_OPTION
    GR_SIZE        dataSize,
    const GR_VOID* pData);

GR_VOID GR_STDCALL grCmdDbgMarkerBegin(
    GR_CMD_BUFFER  cmdBuffer,
    const GR_CHAR* pMarker);

GR_VOID GR_STDCALL grCmdDbgMarkerEnd(
    GR_CMD_BUFFER  cmdBuffer);

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // __MANTLEDBG_H__
