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
 */

#include "img.h"
#include "mem.h"
#include "state.h"
#include "cmd_priv.h"

static void cmd_meta_init_mem_view(struct intel_cmd *cmd,
                                   XGL_GPU_MEMORY mem,
                                   XGL_GPU_SIZE range,
                                   XGL_FORMAT format,
                                   XGL_MEMORY_STATE state,
                                   struct intel_mem_view *view)
{
    XGL_MEMORY_VIEW_ATTACH_INFO info;

    memset(&info, 0, sizeof(info));
    info.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;
    info.mem = mem;
    info.range = range;
    info.stride = icd_format_get_size(format);
    info.format = format;
    info.state = state;

    intel_mem_view_init(view, cmd->dev, &info);
}

static void cmd_meta_set_src_for_mem(struct intel_cmd *cmd,
                                     const struct intel_mem *mem,
                                     XGL_FORMAT format,
                                     struct intel_cmd_meta *meta)
{
    struct intel_mem_view view;

    cmd_meta_init_mem_view(cmd, (XGL_GPU_MEMORY) mem, mem->size, format,
            XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY, &view);

    meta->src.valid = true;

    memcpy(meta->src.surface, view.cmd, sizeof(view.cmd[0]) * view.cmd_len);
    meta->src.surface_len = view.cmd_len;

    meta->src.reloc_target = (intptr_t) mem->bo;
    meta->src.reloc_offset = 0;
    meta->src.reloc_flags = 0;
}

static void cmd_meta_set_dst_for_mem(struct intel_cmd *cmd,
                                     const struct intel_mem *mem,
                                     XGL_FORMAT format,
                                     struct intel_cmd_meta *meta)
{
    struct intel_mem_view view;

    cmd_meta_init_mem_view(cmd, (XGL_GPU_MEMORY) mem, mem->size, format,
            XGL_MEMORY_STATE_GRAPHICS_SHADER_WRITE_ONLY, &view);

    meta->dst.valid = true;

    memcpy(meta->dst.surface, view.cmd, sizeof(view.cmd[0]) * view.cmd_len);
    meta->dst.surface_len = view.cmd_len;

    meta->dst.reloc_target = (intptr_t) mem->bo;
    meta->dst.reloc_offset = 0;
    meta->dst.reloc_flags = INTEL_RELOC_WRITE;
}

static void cmd_meta_set_src_for_img(struct intel_cmd *cmd,
                                     const struct intel_img *img,
                                     XGL_FORMAT format,
                                     XGL_IMAGE_ASPECT aspect,
                                     struct intel_cmd_meta *meta)
{
    XGL_IMAGE_VIEW_CREATE_INFO info;
    struct intel_img_view *view;
    XGL_RESULT ret;

    memset(&info, 0, sizeof(info));
    info.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = (XGL_IMAGE) img;

    switch (img->type) {
    case XGL_IMAGE_1D:
        info.viewType = XGL_IMAGE_VIEW_1D;
        break;
    case XGL_IMAGE_2D:
        info.viewType = XGL_IMAGE_VIEW_2D;
        break;
    case XGL_IMAGE_3D:
        info.viewType = XGL_IMAGE_VIEW_3D;
        break;
    default:
        break;
    }

    info.format = format;
    info.channels.r = XGL_CHANNEL_SWIZZLE_R;
    info.channels.g = XGL_CHANNEL_SWIZZLE_G;
    info.channels.b = XGL_CHANNEL_SWIZZLE_B;
    info.channels.a = XGL_CHANNEL_SWIZZLE_A;
    info.subresourceRange.aspect = aspect;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
    info.subresourceRange.baseArraySlice = 0;
    info.subresourceRange.arraySize = XGL_LAST_MIP_OR_SLICE;

    ret = intel_img_view_create(cmd->dev, &info, &view);
    if (ret != XGL_SUCCESS) {
        cmd->result = ret;
        return;
    }

    meta->src.valid = true;

    memcpy(meta->src.surface, view->cmd,
            sizeof(view->cmd[0]) * view->cmd_len);
    meta->src.surface_len = view->cmd_len;

    meta->src.reloc_target = (intptr_t) img->obj.mem->bo;
    meta->src.reloc_offset = 0;
    meta->src.reloc_flags = 0;

    intel_img_view_destroy(view);
}

static void cmd_meta_set_dst_for_img(struct intel_cmd *cmd,
                                     const struct intel_img *img,
                                     XGL_FORMAT format,
                                     XGL_UINT lod, XGL_UINT layer,
                                     struct intel_cmd_meta *meta)
{
    XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO info;
    struct intel_rt_view *rt;
    XGL_RESULT ret;

    memset(&info, 0, sizeof(info));
    info.sType = XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO;
    info.image = (XGL_IMAGE) img;
    info.format = format;
    info.mipLevel = lod;
    info.baseArraySlice = layer;
    info.arraySize = 1;

    ret = intel_rt_view_create(cmd->dev, &info, &rt);
    if (ret != XGL_SUCCESS) {
        cmd->result = ret;
        return;
    }

    meta->dst.valid = true;

    memcpy(meta->dst.surface, rt->cmd, sizeof(rt->cmd[0]) * rt->cmd_len);
    meta->dst.surface_len = rt->cmd_len;

    meta->dst.reloc_target = (intptr_t) img->obj.mem->bo;
    meta->dst.reloc_offset = 0;
    meta->dst.reloc_flags = INTEL_RELOC_WRITE;

    intel_rt_view_destroy(rt);
}

static void cmd_meta_set_src_for_writer(struct intel_cmd *cmd,
                                        enum intel_cmd_writer_type writer,
                                        XGL_GPU_SIZE size,
                                        XGL_FORMAT format,
                                        struct intel_cmd_meta *meta)
{
    struct intel_mem_view view;

    cmd_meta_init_mem_view(cmd, XGL_NULL_HANDLE, size, format,
            XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY, &view);

    meta->src.valid = true;

    memcpy(meta->src.surface, view.cmd, sizeof(view.cmd[0]) * view.cmd_len);
    meta->src.surface_len = view.cmd_len;

    meta->src.reloc_target = (intptr_t) writer;
    meta->src.reloc_offset = 0;
    meta->src.reloc_flags = INTEL_CMD_RELOC_TARGET_IS_WRITER;
}

static void cmd_meta_set_ds_view(struct intel_cmd *cmd,
                                 const struct intel_img *img,
                                 XGL_UINT lod, XGL_UINT layer,
                                 struct intel_cmd_meta *meta)
{
    XGL_DEPTH_STENCIL_VIEW_CREATE_INFO info;
    struct intel_ds_view *ds;
    XGL_RESULT ret;

    memset(&info, 0, sizeof(info));
    info.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO;
    info.image = (XGL_IMAGE) img;
    info.mipLevel = lod;
    info.baseArraySlice = layer;
    info.arraySize = 1;

    ret = intel_ds_view_create(cmd->dev, &info, &ds);
    if (ret != XGL_SUCCESS) {
        cmd->result = ret;
        return;
    }

    meta->ds.view = ds;
}

static void cmd_meta_set_ds_state(struct intel_cmd *cmd,
                                  XGL_IMAGE_ASPECT aspect,
                                  XGL_UINT32 stencil_ref,
                                  struct intel_cmd_meta *meta)
{
    XGL_DEPTH_STENCIL_STATE_CREATE_INFO info;
    struct intel_ds_state *state;
    XGL_RESULT ret;

    memset(&info, 0, sizeof(info));
    info.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO;

    if (aspect == XGL_IMAGE_ASPECT_DEPTH) {
        info.depthWriteEnable = XGL_TRUE;
    }
    else if (aspect == XGL_IMAGE_ASPECT_STENCIL) {
        info.stencilTestEnable = XGL_TRUE;
        info.stencilReadMask = 0xff;
        info.stencilWriteMask = 0xff;
        info.front.stencilFailOp = XGL_STENCIL_OP_KEEP;
        info.front.stencilPassOp = XGL_STENCIL_OP_REPLACE;
        info.front.stencilDepthFailOp = XGL_STENCIL_OP_KEEP;
        info.front.stencilFunc = XGL_COMPARE_ALWAYS;
        info.front.stencilRef = stencil_ref;
        info.back = info.front;
    }

    ret = intel_ds_state_create(cmd->dev, &info, &state);
    if (ret != XGL_SUCCESS) {
        cmd->result = ret;
        return;
    }

    meta->ds.state = state;
}

static enum intel_dev_meta_shader get_shader_id(const struct intel_dev *dev,
                                                const struct intel_img *img,
                                                bool copy_array)
{
    enum intel_dev_meta_shader shader_id;

    switch (img->type) {
    case XGL_IMAGE_1D:
        shader_id = (copy_array) ?
            INTEL_DEV_META_FS_COPY_1D_ARRAY : INTEL_DEV_META_FS_COPY_1D;
        break;
    case XGL_IMAGE_2D:
        shader_id = (img->samples > 1) ? INTEL_DEV_META_FS_COPY_2D_MS :
                    (copy_array) ?  INTEL_DEV_META_FS_COPY_2D_ARRAY :
                    INTEL_DEV_META_FS_COPY_2D;
        break;
    case XGL_IMAGE_3D:
    default:
        shader_id = INTEL_DEV_META_FS_COPY_2D_ARRAY;
        break;
    }

    return shader_id;
}

static bool cmd_meta_mem_dword_aligned(const struct intel_cmd *cmd,
                                       XGL_GPU_SIZE src_offset,
                                       XGL_GPU_SIZE dst_offset,
                                       XGL_GPU_SIZE size)
{
    return !((src_offset | dst_offset | size) & 0x3);
}

static XGL_FORMAT cmd_meta_img_raw_format(const struct intel_cmd *cmd,
                                          XGL_FORMAT format)
{
    format.numericFormat = XGL_NUM_FMT_UINT;

    if (icd_format_is_compressed(format)) {
        switch (icd_format_get_size(format)) {
        case 1:
            format.channelFormat = XGL_CH_FMT_R8;
            break;
        case 2:
            format.channelFormat = XGL_CH_FMT_R16;
            break;
        case 4:
            format.channelFormat = XGL_CH_FMT_R32;
            break;
        case 8:
            format.channelFormat = XGL_CH_FMT_R32G32;
            break;
        case 16:
            format.channelFormat = XGL_CH_FMT_R32G32B32A32;
            break;
        default:
            assert(!"unsupported compressed block size");
            format.channelFormat = XGL_CH_FMT_R8;
            break;
        }
    }

    return format;
}

XGL_VOID XGLAPI intelCmdCopyMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_COPY*                      pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_mem *src = intel_mem(srcMem);
    struct intel_mem *dst = intel_mem(destMem);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    XGL_UINT i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_VS_POINTS;

    meta.height = 1;
    meta.samples = 1;

    format.channelFormat = XGL_CH_FMT_UNDEFINED;
    format.numericFormat = XGL_NUM_FMT_UINT;

    for (i = 0; i < regionCount; i++) {
        const XGL_MEMORY_COPY *region = &pRegions[i];
        XGL_CHANNEL_FORMAT ch;

        meta.src.x = region->srcOffset;
        meta.dst.x = region->destOffset;
        meta.width = region->copySize;

        if (cmd_meta_mem_dword_aligned(cmd, region->srcOffset,
                    region->destOffset, region->copySize)) {
            meta.shader_id = INTEL_DEV_META_VS_COPY_MEM;
            meta.src.x /= 4;
            meta.dst.x /= 4;
            meta.width /= 4;

            /*
             * INTEL_DEV_META_VS_COPY_MEM is untyped but expects the stride to
             * be 16
             */
            ch = XGL_CH_FMT_R32G32B32A32;
        } else {
            if (cmd_gen(cmd) == INTEL_GEN(6)) {
                intel_dev_log(cmd->dev, XGL_DBG_MSG_ERROR,
                        XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                        "unaligned xglCmdCopyMemory unsupported");
                cmd->result = XGL_ERROR_UNKNOWN;
                continue;
            }

            meta.shader_id = INTEL_DEV_META_VS_COPY_MEM_UNALIGNED;

            /*
             * INTEL_DEV_META_VS_COPY_MEM_UNALIGNED is untyped but expects the
             * stride to be 4
             */
            ch = XGL_CH_FMT_R8G8B8A8;
        }

        if (format.channelFormat != ch) {
            format.channelFormat = ch;

            cmd_meta_set_src_for_mem(cmd, src, format, &meta);
            cmd_meta_set_dst_for_mem(cmd, dst, format, &meta);
        }

        cmd_draw_meta(cmd, &meta);
    }
}

XGL_VOID XGLAPI intelCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *src = intel_img(srcImage);
    struct intel_img *dst = intel_img(destImage);
    struct intel_cmd_meta meta;
    XGL_FORMAT raw_format;
    bool raw_copy;
    XGL_UINT i;

    if (src->type != dst->type) {
        cmd->result = XGL_ERROR_UNKNOWN;
        return;
    }

    if (icd_format_is_equal(src->layout.format, dst->layout.format)) {
        raw_copy = true;
        raw_format = cmd_meta_img_raw_format(cmd, src->layout.format);
    } else if (icd_format_is_compressed(src->layout.format) ||
               icd_format_is_compressed(dst->layout.format)) {
        cmd->result = XGL_ERROR_UNKNOWN;
        return;
    }

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    cmd_meta_set_src_for_img(cmd, src,
            (raw_copy) ? raw_format : src->layout.format,
            XGL_IMAGE_ASPECT_COLOR, &meta);

    meta.samples = dst->samples;

    for (i = 0; i < regionCount; i++) {
        const XGL_IMAGE_COPY *region = &pRegions[i];
        XGL_UINT j;

        meta.shader_id = get_shader_id(cmd->dev, src,
                (region->extent.depth > 1));

        meta.src.lod = region->srcSubresource.mipLevel;
        meta.src.layer = region->srcSubresource.arraySlice +
            region->srcOffset.z;
        meta.src.x = region->srcOffset.x;
        meta.src.y = region->srcOffset.y;

        meta.dst.lod = region->destSubresource.mipLevel;
        meta.dst.layer = region->destSubresource.arraySlice +
                region->destOffset.z;
        meta.dst.x = region->destOffset.x;
        meta.dst.y = region->destOffset.y;

        meta.width = region->extent.width;
        meta.height = region->extent.height;

        for (j = 0; j < region->extent.depth; j++) {
            cmd_meta_set_dst_for_img(cmd, dst,
                    (raw_copy) ? raw_format : dst->layout.format,
                    meta.dst.lod, meta.dst.layer, &meta);

            cmd_draw_meta(cmd, &meta);

            meta.src.layer++;
            meta.dst.layer++;
        }
    }
}

XGL_VOID XGLAPI intelCmdCopyMemoryToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_mem *mem = intel_mem(srcMem);
    struct intel_img *img = intel_img(destImage);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    XGL_UINT i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    meta.shader_id = INTEL_DEV_META_FS_COPY_MEM_TO_IMG;
    meta.samples = img->samples;

    format = cmd_meta_img_raw_format(cmd, img->layout.format);
    cmd_meta_set_src_for_mem(cmd, mem, format, &meta);

    for (i = 0; i < regionCount; i++) {
        const XGL_MEMORY_IMAGE_COPY *region = &pRegions[i];
        XGL_UINT j;

        meta.src.x = region->memOffset / icd_format_get_size(format);

        meta.dst.lod = region->imageSubresource.mipLevel;
        meta.dst.layer = region->imageSubresource.arraySlice +
            region->imageOffset.z;
        meta.dst.x = region->imageOffset.x;
        meta.dst.y = region->imageOffset.y;

        meta.width = region->imageExtent.width;
        meta.height = region->imageExtent.height;

        for (j = 0; j < region->imageExtent.depth; j++) {
            cmd_meta_set_dst_for_img(cmd, img, format,
                    meta.dst.lod, meta.dst.layer, &meta);

            cmd_draw_meta(cmd, &meta);

            meta.src.x += meta.width * meta.height;
            meta.dst.layer++;
        }
    }
}

XGL_VOID XGLAPI intelCmdCopyImageToMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(srcImage);
    struct intel_mem *mem = intel_mem(destMem);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    XGL_UINT i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    format = cmd_meta_img_raw_format(cmd, img->layout.format);
    cmd_meta_set_src_for_img(cmd, img, format, XGL_IMAGE_ASPECT_COLOR, &meta);
    cmd_meta_set_dst_for_mem(cmd, mem, format, &meta);

    meta.height = 1;
    meta.samples = 1;

    for (i = 0; i < regionCount; i++) {
        const XGL_MEMORY_IMAGE_COPY *region = &pRegions[i];
        XGL_UINT j;

        meta.shader_id = get_shader_id(cmd->dev, img,
                (region->imageExtent.depth > 1));

        meta.src.lod = region->imageSubresource.mipLevel;
        meta.src.layer = region->imageSubresource.arraySlice +
            region->imageOffset.z;
        meta.src.x = region->imageOffset.x;
        meta.src.y = region->imageOffset.y;

        meta.dst.x = region->memOffset / icd_format_get_size(format);

        meta.width = region->imageExtent.width * region->imageExtent.height;

        for (j = 0; j < region->imageExtent.depth; j++) {
            cmd_draw_meta(cmd, &meta);

            meta.src.layer++;
            meta.dst.x += meta.width;
        }
    }
}

XGL_VOID XGLAPI intelCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_STATE                             srcImageState,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_STATE                             destImageState)
{
    const struct intel_img *src = intel_img(srcImage);
    XGL_IMAGE_COPY region;
    XGL_UINT lv;

    memset(&region, 0, sizeof(region));
    region.srcSubresource.aspect = XGL_IMAGE_ASPECT_COLOR;
    region.destSubresource.aspect = XGL_IMAGE_ASPECT_COLOR;

    for (lv = 0; lv < src->mip_levels; lv++) {
        region.srcSubresource.mipLevel = lv;
        region.destSubresource.mipLevel = lv;

        region.extent.width = u_minify(src->layout.width0, lv);
        region.extent.height = u_minify(src->layout.height0, lv);
        region.extent.depth = src->array_size;

        intelCmdCopyImage(cmdBuffer, srcImage, destImage, 1, &region);
    }
}

XGL_VOID XGLAPI intelCmdUpdateMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const XGL_UINT32*                           pData)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_mem *dst = intel_mem(destMem);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    uint32_t *ptr;
    uint32_t offset;

    /* must be 4-byte aligned */
    if ((destOffset | dataSize) & 3) {
        cmd->result = XGL_ERROR_UNKNOWN;
        return;
    }

    /* write to dynamic state writer first */
    offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_BLOB, 32,
            (dataSize + 3) / 4, &ptr);
    memcpy(ptr, pData, dataSize);

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_VS_POINTS;

    meta.shader_id = INTEL_DEV_META_VS_COPY_MEM;

    meta.src.x = offset / 4;
    meta.dst.x = destOffset / 4;
    meta.width = dataSize / 4;
    meta.height = 1;
    meta.samples = 1;

    /*
     * INTEL_DEV_META_VS_COPY_MEM is untyped but expects the stride to be 16
     */
    format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    format.numericFormat = XGL_NUM_FMT_UINT;

    cmd_meta_set_src_for_writer(cmd, INTEL_CMD_WRITER_STATE,
            offset + dataSize, format, &meta);
    cmd_meta_set_dst_for_mem(cmd, dst, format, &meta);

    cmd_draw_meta(cmd, &meta);
}

XGL_VOID XGLAPI intelCmdFillMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    XGL_UINT32                                  data)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_mem *dst = intel_mem(destMem);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;

    /* must be 4-byte aligned */
    if ((destOffset | fillSize) & 3) {
        cmd->result = XGL_ERROR_UNKNOWN;
        return;
    }

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_VS_POINTS;

    meta.shader_id = INTEL_DEV_META_VS_FILL_MEM;

    meta.clear_val[0] = data;

    meta.dst.x = destOffset / 4;
    meta.width = fillSize / 4;
    meta.height = 1;
    meta.samples = 1;

    /*
     * INTEL_DEV_META_VS_FILL_MEM is untyped but expects the stride to be 16
     */
    format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    format.numericFormat = XGL_NUM_FMT_UINT;

    cmd_meta_set_dst_for_mem(cmd, dst, format, &meta);

    cmd_draw_meta(cmd, &meta);
}

static void cmd_meta_clear_image(struct intel_cmd *cmd,
                                 struct intel_img *img,
                                 XGL_FORMAT format,
                                 struct intel_cmd_meta *meta,
                                 const XGL_IMAGE_SUBRESOURCE_RANGE *range)
{
    XGL_UINT mip_levels, array_size;
    XGL_UINT i, j;

    if (range->baseMipLevel >= img->mip_levels ||
        range->baseArraySlice >= img->array_size)
        return;

    mip_levels = img->mip_levels - range->baseMipLevel;
    if (mip_levels > range->mipLevels)
        mip_levels = range->mipLevels;

    array_size = img->array_size - range->baseArraySlice;
    if (array_size > range->arraySize)
        array_size = range->arraySize;

    for (i = 0; i < mip_levels; i++) {
        meta->dst.lod = range->baseMipLevel + i;
        meta->dst.layer = range->baseArraySlice;

        meta->width = u_minify(img->layout.width0, meta->dst.lod);
        meta->height = u_minify(img->layout.height0, meta->dst.lod);

        for (j = 0; j < array_size; j++) {
            if (range->aspect == XGL_IMAGE_ASPECT_COLOR) {
                cmd_meta_set_dst_for_img(cmd, img, format,
                        meta->dst.lod, meta->dst.layer, meta);

                cmd_draw_meta(cmd, meta);
            } else {
                cmd_meta_set_ds_view(cmd, img, meta->dst.lod,
                        meta->dst.layer, meta);
                cmd_meta_set_ds_state(cmd, range->aspect,
                        meta->clear_val[1], meta);

                cmd_draw_meta(cmd, meta);

                intel_ds_view_destroy(meta->ds.view);
                intel_ds_state_destroy(meta->ds.state);
            }

            meta->dst.layer++;
        }
    }
}

XGL_VOID XGLAPI intelCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_FLOAT                             color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(image);
    struct intel_cmd_meta meta;
    XGL_UINT i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    meta.shader_id = INTEL_DEV_META_FS_CLEAR_COLOR;
    meta.samples = img->samples;

    meta.clear_val[0] = u_fui(color[0]);
    meta.clear_val[1] = u_fui(color[1]);
    meta.clear_val[2] = u_fui(color[2]);
    meta.clear_val[3] = u_fui(color[3]);

    for (i = 0; i < rangeCount; i++) {
        cmd_meta_clear_image(cmd, img, img->layout.format,
                &meta, &pRanges[i]);
    }
}

XGL_VOID XGLAPI intelCmdClearColorImageRaw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_UINT32                            color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(image);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    XGL_UINT i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    meta.shader_id = INTEL_DEV_META_FS_CLEAR_COLOR;
    meta.samples = img->samples;

    meta.clear_val[0] = color[0];
    meta.clear_val[1] = color[1];
    meta.clear_val[2] = color[2];
    meta.clear_val[3] = color[3];

    format = cmd_meta_img_raw_format(cmd, img->layout.format);

    for (i = 0; i < rangeCount; i++)
        cmd_meta_clear_image(cmd, img, format, &meta, &pRanges[i]);
}

XGL_VOID XGLAPI intelCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_FLOAT                                   depth,
    XGL_UINT32                                  stencil,
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(image);
    struct intel_cmd_meta meta;
    XGL_UINT i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_DEPTH_STENCIL_RECT;

    meta.shader_id = INTEL_DEV_META_FS_CLEAR_DEPTH;
    meta.samples = img->samples;

    meta.clear_val[0] = u_fui(depth);
    meta.clear_val[1] = stencil;

    for (i = 0; i < rangeCount; i++) {
        const XGL_IMAGE_SUBRESOURCE_RANGE *range = &pRanges[i];

        cmd_meta_clear_image(cmd, img, img->layout.format,
                &meta, range);
    }
}

XGL_VOID XGLAPI intelCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *src = intel_img(srcImage);
    struct intel_img *dst = intel_img(destImage);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    XGL_UINT i;

    if (src->samples <= 1 || dst->samples > 1 ||
        !icd_format_is_equal(src->layout.format, dst->layout.format)) {
        cmd->result = XGL_ERROR_UNKNOWN;
        return;
    }

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    switch (src->samples) {
    case 2:
    default:
        meta.shader_id = INTEL_DEV_META_FS_RESOLVE_2X;
        break;
    case 4:
        meta.shader_id = INTEL_DEV_META_FS_RESOLVE_4X;
        break;
    case 8:
        meta.shader_id = INTEL_DEV_META_FS_RESOLVE_8X;
        break;
    case 16:
        meta.shader_id = INTEL_DEV_META_FS_RESOLVE_16X;
        break;
    }

    meta.samples = 1;

    format = cmd_meta_img_raw_format(cmd, src->layout.format);
    cmd_meta_set_src_for_img(cmd, src, format, XGL_IMAGE_ASPECT_COLOR, &meta);

    for (i = 0; i < rectCount; i++) {
        const XGL_IMAGE_RESOLVE *rect = &pRects[i];

        meta.src.lod = rect->srcSubresource.mipLevel;
        meta.src.layer = rect->srcSubresource.arraySlice;
        meta.src.x = rect->srcOffset.x;
        meta.src.y = rect->srcOffset.y;

        meta.dst.lod = rect->destSubresource.mipLevel;
        meta.dst.layer = rect->destSubresource.arraySlice;
        meta.dst.x = rect->destOffset.x;
        meta.dst.y = rect->destOffset.y;

        meta.width = rect->extent.width;
        meta.height = rect->extent.height;

        cmd_meta_set_dst_for_img(cmd, dst, format,
                meta.dst.lod, meta.dst.layer, &meta);

        cmd_draw_meta(cmd, &meta);
    }
}
