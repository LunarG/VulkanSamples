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
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifndef CMD_H
#define CMD_H

#include "intel.h"
#include "obj.h"
#include "view.h"

struct intel_pipeline;
struct intel_pipeline_shader;
struct intel_pipeline_delta;
struct intel_viewport_state;
struct intel_raster_state;
struct intel_msaa_state;
struct intel_blend_state;
struct intel_ds_state;
struct intel_dset;

struct intel_cmd_reloc;

/*
 * We know what workarounds are needed for intel_pipeline.  These are mostly
 * for intel_pipeline_delta.
 */
enum intel_cmd_wa_flags {
    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 60:
     *
     *     "Before any depth stall flush (including those produced by
     *      non-pipelined state commands), software needs to first send a
     *      PIPE_CONTROL with no bits set except Post-Sync Operation != 0."
     */
    INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE = 1 << 0,

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 274:
     *
     *     "A PIPE_CONTROL command, with only the Stall At Pixel Scoreboard
     *      field set (DW1 Bit 1), must be issued prior to any change to the
     *      value in this field (Maximum Number of Threads in 3DSTATE_WM)"
     *
     * From the Ivy Bridge PRM, volume 2 part 1, page 286:
     *
     *     "If this field (Maximum Number of Threads in 3DSTATE_PS) is changed
     *      between 3DPRIMITIVE commands, a PIPE_CONTROL command with Stall at
     *      Pixel Scoreboard set is required to be issued."
     */
    INTEL_CMD_WA_GEN6_PRE_COMMAND_SCOREBOARD_STALL = 1 << 1,

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 106:
     *
     *     "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth
     *      stall needs to be sent just prior to any 3DSTATE_VS,
     *      3DSTATE_URB_VS, 3DSTATE_CONSTANT_VS,
     *      3DSTATE_BINDING_TABLE_POINTER_VS, 3DSTATE_SAMPLER_STATE_POINTER_VS
     *      command.  Only one PIPE_CONTROL needs to be sent before any
     *      combination of VS associated 3DSTATE."
     */
    INTEL_CMD_WA_GEN7_PRE_VS_DEPTH_STALL_WRITE = 1 << 2,

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 258:
     *
     *     "Due to an HW issue driver needs to send a pipe control with stall
     *      when ever there is state change in depth bias related state"
     *
     * From the Ivy Bridge PRM, volume 2 part 1, page 292:
     *
     *     "A PIPE_CONTOL command with the CS Stall bit set must be programmed
     *      in the ring after this instruction
     *      (3DSTATE_PUSH_CONSTANT_ALLOC_PS)."
     */
    INTEL_CMD_WA_GEN7_POST_COMMAND_CS_STALL = 1 << 3,

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 276:
     *
     *     "The driver must make sure a PIPE_CONTROL with the Depth Stall
     *      Enable bit set after all the following states are programmed:
     *
     *       - 3DSTATE_PS
     *       - 3DSTATE_VIEWPORT_STATE_POINTERS_CC
     *       - 3DSTATE_CONSTANT_PS
     *       - 3DSTATE_BINDING_TABLE_POINTERS_PS
     *       - 3DSTATE_SAMPLER_STATE_POINTERS_PS
     *       - 3DSTATE_CC_STATE_POINTERS
     *       - 3DSTATE_BLEND_STATE_POINTERS
     *       - 3DSTATE_DEPTH_STENCIL_STATE_POINTERS"
     */
    INTEL_CMD_WA_GEN7_POST_COMMAND_DEPTH_STALL = 1 << 4,
};

enum intel_cmd_writer_type {
    INTEL_CMD_WRITER_BATCH,
    INTEL_CMD_WRITER_STATE,
    INTEL_CMD_WRITER_INSTRUCTION,

    INTEL_CMD_WRITER_COUNT,
};

struct intel_cmd_shader {
    const struct intel_pipeline_shader *shader;
    XGL_UINT kernel_pos;
};

/*
 * States bounded to the command buffer.  We want to write states directly to
 * the command buffer when possible, and reduce this struct.
 */
struct intel_cmd_bind {
    struct {
        const struct intel_pipeline *graphics;
        const struct intel_pipeline *compute;
        const struct intel_pipeline_delta *graphics_delta;
        const struct intel_pipeline_delta *compute_delta;
    } pipeline;

    /*
     * Currently active shaders for this command buffer.
     * Provides data only available after shaders are bound to
     * a command buffer, such as the kernel position in the kernel BO
     */
    struct intel_cmd_shader vs;
    struct intel_cmd_shader fs;
    struct intel_cmd_shader gs;
    struct intel_cmd_shader tcs;
    struct intel_cmd_shader tes;
    struct intel_cmd_shader cs;

    struct {
        XGL_UINT count;
        XGL_UINT used;
        struct intel_cmd_shader *shaderArray;
    } shaderCache;

    struct {
        const struct intel_viewport_state *viewport;
        const struct intel_raster_state *raster;
        const struct intel_msaa_state *msaa;
        const struct intel_blend_state *blend;
        const struct intel_ds_state *ds;
    } state;

    struct {
        const struct intel_dset *graphics;
        XGL_UINT graphics_offset;
        const struct intel_dset *compute;
        XGL_UINT compute_offset;
    } dset;

    struct {
        struct intel_mem_view graphics;
        struct intel_mem_view compute;
    } dyn_view;

    struct {
        const struct intel_mem *mem;
        XGL_GPU_SIZE offset;
        XGL_INDEX_TYPE type;
    } index;

    struct {
        const struct intel_rt_view *rt[XGL_MAX_COLOR_ATTACHMENTS];
        XGL_UINT rt_count;

        const struct intel_ds_view *ds;
    } att;

    XGL_UINT draw_count;
    uint32_t wa_flags;
};

struct intel_cmd_writer {
    struct intel_bo *bo;
    void *ptr;

    /* in DWords */
    XGL_UINT size;
    XGL_UINT used;
};

struct intel_cmd {
    struct intel_obj obj;

    struct intel_dev *dev;
    struct intel_bo *scratch_bo;
    int pipeline_select;

    struct intel_cmd_reloc *relocs;
    XGL_UINT reloc_count;

    XGL_FLAGS flags;

    struct intel_cmd_writer writers[INTEL_CMD_WRITER_COUNT];

    XGL_UINT reloc_used;
    XGL_RESULT result;

    struct intel_cmd_bind bind;
};

static inline struct intel_cmd *intel_cmd(XGL_CMD_BUFFER cmd)
{
    return (struct intel_cmd *) cmd;
}

static inline struct intel_cmd *intel_cmd_from_obj(struct intel_obj *obj)
{
    return (struct intel_cmd *) obj;
}

XGL_RESULT intel_cmd_create(struct intel_dev *dev,
                            const XGL_CMD_BUFFER_CREATE_INFO *info,
                            struct intel_cmd **cmd_ret);
void intel_cmd_destroy(struct intel_cmd *cmd);

XGL_RESULT intel_cmd_begin(struct intel_cmd *cmd, XGL_FLAGS flags);
XGL_RESULT intel_cmd_end(struct intel_cmd *cmd);

static inline struct intel_bo *intel_cmd_get_batch(const struct intel_cmd *cmd,
                                                   XGL_GPU_SIZE *used)
{
    const struct intel_cmd_writer *writer =
        &cmd->writers[INTEL_CMD_WRITER_BATCH];

    if (used)
        *used = sizeof(uint32_t) * writer->used;

    return writer->bo;
}

XGL_RESULT XGLAPI intelCreateCommandBuffer(
    XGL_DEVICE                                  device,
    const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    XGL_CMD_BUFFER*                             pCmdBuffer);

XGL_RESULT XGLAPI intelBeginCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_FLAGS                                   flags);

XGL_RESULT XGLAPI intelEndCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer);

XGL_RESULT XGLAPI intelResetCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer);

XGL_VOID XGLAPI intelCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline);

XGL_VOID XGLAPI intelCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta);

XGL_VOID XGLAPI intelCmdBindStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_STATE_OBJECT                            state);

XGL_VOID XGLAPI intelCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    index,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    slotOffset);

XGL_VOID XGLAPI intelCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView);

XGL_VOID XGLAPI intelCmdBindIndexData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType);

XGL_VOID XGLAPI intelCmdBindAttachments(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment);

XGL_VOID XGLAPI intelCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_MEMORY_STATE_TRANSITION*          pStateTransitions);

XGL_VOID XGLAPI intelCmdPrepareImages(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_IMAGE_STATE_TRANSITION*           pStateTransitions);

XGL_VOID XGLAPI intelCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstVertex,
    XGL_UINT                                    vertexCount,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount);

XGL_VOID XGLAPI intelCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstIndex,
    XGL_UINT                                    indexCount,
    XGL_INT                                     vertexOffset,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount);

XGL_VOID XGLAPI intelCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride);

XGL_VOID XGLAPI intelCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride);

XGL_VOID XGLAPI intelCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    x,
    XGL_UINT                                    y,
    XGL_UINT                                    z);

XGL_VOID XGLAPI intelCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset);

XGL_VOID XGLAPI intelCmdCopyMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_COPY*                      pRegions);

XGL_VOID XGLAPI intelCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions);

XGL_VOID XGLAPI intelCmdCopyMemoryToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions);

XGL_VOID XGLAPI intelCmdCopyImageToMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions);

XGL_VOID XGLAPI intelCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_STATE                             srcImageState,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_STATE                             destImageState);

XGL_VOID XGLAPI intelCmdUpdateMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const XGL_UINT32*                           pData);

XGL_VOID XGLAPI intelCmdFillMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    XGL_UINT32                                  data);

XGL_VOID XGLAPI intelCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_FLOAT                             color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

XGL_VOID XGLAPI intelCmdClearColorImageRaw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_UINT32                            color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

XGL_VOID XGLAPI intelCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_FLOAT                                   depth,
    XGL_UINT32                                  stencil,
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

XGL_VOID XGLAPI intelCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects);

XGL_VOID XGLAPI intelCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event);

XGL_VOID XGLAPI intelCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event);

XGL_VOID XGLAPI intelCmdMemoryAtomic(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_UINT64                                  srcData,
    XGL_ATOMIC_OP                               atomicOp);

XGL_VOID XGLAPI intelCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot,
    XGL_FLAGS                                   flags);

XGL_VOID XGLAPI intelCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot);

XGL_VOID XGLAPI intelCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount);

XGL_VOID XGLAPI intelCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset);

XGL_VOID XGLAPI intelCmdInitAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    const XGL_UINT32*                           pData);

XGL_VOID XGLAPI intelCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_SIZE                                srcOffset);

XGL_VOID XGLAPI intelCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset);

XGL_VOID XGLAPI intelCmdDbgMarkerBegin(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_CHAR*                             pMarker);

XGL_VOID XGLAPI intelCmdDbgMarkerEnd(
    XGL_CMD_BUFFER                              cmdBuffer);

#endif /* CMD_H */
