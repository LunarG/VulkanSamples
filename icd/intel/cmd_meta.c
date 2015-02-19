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

#include "buf.h"
#include "img.h"
#include "mem.h"
#include "state.h"
#include "cmd_priv.h"

static XGL_RESULT cmd_meta_create_buf_view(struct intel_cmd *cmd,
                                           XGL_BUFFER buf,
                                           XGL_GPU_SIZE range,
                                           XGL_FORMAT format,
                                           struct intel_buf_view **view)
{
    XGL_BUFFER_VIEW_CREATE_INFO info;

    memset(&info, 0, sizeof(info));
    info.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    info.buffer = buf;
    info.viewType = XGL_BUFFER_VIEW_TYPED;
    info.stride = icd_format_get_size(format);
    info.format = format;
    info.channels.r = XGL_CHANNEL_SWIZZLE_R;
    info.channels.g = XGL_CHANNEL_SWIZZLE_G;
    info.channels.b = XGL_CHANNEL_SWIZZLE_B;
    info.channels.a = XGL_CHANNEL_SWIZZLE_A;
    info.range = range;

    /*
     * We do not rely on the hardware to avoid out-of-bound access.  But we do
     * not want the hardware to ignore the last element either.
     */
    if (info.range % info.stride)
        info.range += info.stride - (info.range % info.stride);

    return intel_buf_view_create(cmd->dev, &info, view);
}

static void cmd_meta_set_src_for_buf(struct intel_cmd *cmd,
                                     const struct intel_buf *buf,
                                     XGL_FORMAT format,
                                     struct intel_cmd_meta *meta)
{
    struct intel_buf_view *view;
    XGL_RESULT res;

    res = cmd_meta_create_buf_view(cmd, (XGL_BUFFER) buf,
            buf->size, format, &view);
    if (res != XGL_SUCCESS) {
        cmd_fail(cmd, res);
        return;
    }

    meta->src.valid = true;

    memcpy(meta->src.surface, view->cmd,
            sizeof(view->cmd[0]) * view->cmd_len);
    meta->src.surface_len = view->cmd_len;

    intel_buf_view_destroy(view);

    meta->src.reloc_target = (intptr_t) buf->obj.mem->bo;
    meta->src.reloc_offset = 0;
    meta->src.reloc_flags = 0;
}

static void cmd_meta_set_dst_for_buf(struct intel_cmd *cmd,
                                     const struct intel_buf *buf,
                                     XGL_FORMAT format,
                                     struct intel_cmd_meta *meta)
{
    struct intel_buf_view *view;
    XGL_RESULT res;

    res = cmd_meta_create_buf_view(cmd, (XGL_BUFFER) buf,
            buf->size, format, &view);
    if (res != XGL_SUCCESS) {
        cmd_fail(cmd, res);
        return;
    }

    meta->dst.valid = true;

    memcpy(meta->dst.surface, view->cmd,
            sizeof(view->cmd[0]) * view->cmd_len);
    meta->dst.surface_len = view->cmd_len;

    intel_buf_view_destroy(view);

    meta->dst.reloc_target = (intptr_t) buf->obj.mem->bo;
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
        cmd_fail(cmd, ret);
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

static void cmd_meta_adjust_compressed_dst(struct intel_cmd *cmd,
                                           const struct intel_img *img,
                                           struct intel_cmd_meta *meta)
{
    int32_t w, h, layer;
    unsigned x_offset, y_offset;

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        w = GEN_EXTRACT(meta->dst.surface[2], GEN7_SURFACE_DW2_WIDTH);
        h = GEN_EXTRACT(meta->dst.surface[2], GEN7_SURFACE_DW2_HEIGHT);
        layer = GEN_EXTRACT(meta->dst.surface[4],
                GEN7_SURFACE_DW4_MIN_ARRAY_ELEMENT);
    } else {
        w = GEN_EXTRACT(meta->dst.surface[2], GEN6_SURFACE_DW2_WIDTH);
        h = GEN_EXTRACT(meta->dst.surface[2], GEN6_SURFACE_DW2_HEIGHT);
        layer = GEN_EXTRACT(meta->dst.surface[4],
                GEN6_SURFACE_DW4_MIN_ARRAY_ELEMENT);
    }

    /* note that the width/height fields have the real values minus 1 */
    w = (w + img->layout.block_width) / img->layout.block_width - 1;
    h = (h + img->layout.block_height) / img->layout.block_height - 1;

    /* adjust width and height */
    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        meta->dst.surface[2] &= ~(GEN7_SURFACE_DW2_WIDTH__MASK |
                                  GEN7_SURFACE_DW2_HEIGHT__MASK);
        meta->dst.surface[2] |= GEN_SHIFT32(w, GEN7_SURFACE_DW2_WIDTH) |
                                GEN_SHIFT32(h, GEN7_SURFACE_DW2_HEIGHT);
    } else {
        meta->dst.surface[2] &= ~(GEN6_SURFACE_DW2_WIDTH__MASK |
                                  GEN6_SURFACE_DW2_HEIGHT__MASK);
        meta->dst.surface[2] |= GEN_SHIFT32(w, GEN6_SURFACE_DW2_WIDTH) |
                                GEN_SHIFT32(h, GEN6_SURFACE_DW2_HEIGHT);
    }

    if (!layer)
        return;

    meta->dst.reloc_offset = intel_layout_get_slice_tile_offset(&img->layout,
            0, layer, &x_offset, &y_offset);

    /*
     * The lower 2 bits (or 1 bit for Y) are missing.  This may be a problem
     * for small images (16x16 or smaller).  We will need to adjust the
     * drawing rectangle instead.
     */
    x_offset = (x_offset / img->layout.block_width) >> 2;
    y_offset = (y_offset / img->layout.block_height) >> 1;

    /* adjust min array element and X/Y offsets */
    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        meta->dst.surface[4] &= ~GEN7_SURFACE_DW4_MIN_ARRAY_ELEMENT__MASK;
        meta->dst.surface[5] |= GEN_SHIFT32(x_offset, GEN7_SURFACE_DW5_X_OFFSET) |
                                GEN_SHIFT32(y_offset, GEN7_SURFACE_DW5_Y_OFFSET);
    } else {
        meta->dst.surface[4] &= ~GEN6_SURFACE_DW4_MIN_ARRAY_ELEMENT__MASK;
        meta->dst.surface[5] |= GEN_SHIFT32(x_offset, GEN6_SURFACE_DW5_X_OFFSET) |
                                GEN_SHIFT32(y_offset, GEN6_SURFACE_DW5_Y_OFFSET);
    }
}

static void cmd_meta_set_dst_for_img(struct intel_cmd *cmd,
                                     const struct intel_img *img,
                                     XGL_FORMAT format,
                                     uint32_t lod, uint32_t layer,
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
        cmd_fail(cmd, ret);
        return;
    }

    meta->dst.valid = true;

    memcpy(meta->dst.surface, rt->cmd, sizeof(rt->cmd[0]) * rt->cmd_len);
    meta->dst.surface_len = rt->cmd_len;

    meta->dst.reloc_target = (intptr_t) img->obj.mem->bo;
    meta->dst.reloc_offset = 0;
    meta->dst.reloc_flags = INTEL_RELOC_WRITE;

    if (icd_format_is_compressed(img->layout.format))
        cmd_meta_adjust_compressed_dst(cmd, img, meta);

    intel_rt_view_destroy(rt);
}

static void cmd_meta_set_src_for_writer(struct intel_cmd *cmd,
                                        enum intel_cmd_writer_type writer,
                                        XGL_GPU_SIZE size,
                                        XGL_FORMAT format,
                                        struct intel_cmd_meta *meta)
{
    struct intel_buf_view *view;
    XGL_RESULT res;

    res = cmd_meta_create_buf_view(cmd, (XGL_BUFFER) XGL_NULL_HANDLE,
            size, format, &view);
    if (res != XGL_SUCCESS) {
        cmd_fail(cmd, res);
        return;
    }

    meta->src.valid = true;

    memcpy(meta->src.surface, view->cmd,
            sizeof(view->cmd[0]) * view->cmd_len);
    meta->src.surface_len = view->cmd_len;

    intel_buf_view_destroy(view);

    meta->src.reloc_target = (intptr_t) writer;
    meta->src.reloc_offset = 0;
    meta->src.reloc_flags = INTEL_CMD_RELOC_TARGET_IS_WRITER;
}

static void cmd_meta_set_ds_view(struct intel_cmd *cmd,
                                 const struct intel_img *img,
                                 uint32_t lod, uint32_t layer,
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
        cmd_fail(cmd, ret);
        return;
    }

    meta->ds.view = ds;
}

static void cmd_meta_set_ds_state(struct intel_cmd *cmd,
                                  XGL_IMAGE_ASPECT aspect,
                                  uint32_t stencil_ref,
                                  struct intel_cmd_meta *meta)
{
    meta->ds.stencil_ref = stencil_ref;
    meta->ds.aspect = aspect;
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
    switch (icd_format_get_size(format)) {
    case 1:
        format = XGL_FMT_R8_UINT;
        break;
    case 2:
        format = XGL_FMT_R16_UINT;
        break;
    case 4:
        format = XGL_FMT_R32_UINT;
        break;
    case 8:
        format = XGL_FMT_R32G32_UINT;
        break;
    case 16:
        format = XGL_FMT_R32G32B32A32_UINT;
        break;
    default:
        assert(!"unsupported image format for raw blit op");
        format = XGL_FMT_UNDEFINED;
        break;
    }

    return format;
}

ICD_EXPORT void XGLAPI xglCmdCopyBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  srcBuffer,
    XGL_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const XGL_BUFFER_COPY*                      pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_buf *src = intel_buf(srcBuffer);
    struct intel_buf *dst = intel_buf(destBuffer);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    uint32_t i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_VS_POINTS;

    meta.height = 1;
    meta.samples = 1;

    format = XGL_FMT_UNDEFINED;

    for (i = 0; i < regionCount; i++) {
        const XGL_BUFFER_COPY *region = &pRegions[i];
        XGL_FORMAT fmt;

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
            fmt = XGL_FMT_R32G32B32A32_UINT;
        } else {
            if (cmd_gen(cmd) == INTEL_GEN(6)) {
                intel_dev_log(cmd->dev, XGL_DBG_MSG_ERROR,
                        XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                        "unaligned xglCmdCopyBuffer unsupported");
                cmd_fail(cmd, XGL_ERROR_UNKNOWN);
                continue;
            }

            meta.shader_id = INTEL_DEV_META_VS_COPY_MEM_UNALIGNED;

            /*
             * INTEL_DEV_META_VS_COPY_MEM_UNALIGNED is untyped but expects the
             * stride to be 4
             */
            fmt = XGL_FMT_R8G8B8A8_UINT;
        }

        if (format != fmt) {
            format = fmt;

            cmd_meta_set_src_for_buf(cmd, src, format, &meta);
            cmd_meta_set_dst_for_buf(cmd, dst, format, &meta);
        }

        cmd_draw_meta(cmd, &meta);
    }
}

ICD_EXPORT void XGLAPI xglCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    uint32_t                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *src = intel_img(srcImage);
    struct intel_img *dst = intel_img(destImage);
    struct intel_cmd_meta meta;
    XGL_FORMAT raw_format;
    bool raw_copy = false;
    uint32_t i;

    if (src->type != dst->type) {
        cmd_fail(cmd, XGL_ERROR_UNKNOWN);
        return;
    }

    if (src->layout.format == dst->layout.format) {
        raw_copy = true;
        raw_format = cmd_meta_img_raw_format(cmd, src->layout.format);
    } else if (icd_format_is_compressed(src->layout.format) ||
               icd_format_is_compressed(dst->layout.format)) {
        cmd_fail(cmd, XGL_ERROR_UNKNOWN);
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
        uint32_t j;

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

ICD_EXPORT void XGLAPI xglCmdCopyBufferToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  srcBuffer,
    XGL_IMAGE                                   destImage,
    uint32_t                                    regionCount,
    const XGL_BUFFER_IMAGE_COPY*                pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_buf *buf = intel_buf(srcBuffer);
    struct intel_img *img = intel_img(destImage);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    uint32_t i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    meta.shader_id = INTEL_DEV_META_FS_COPY_MEM_TO_IMG;
    meta.samples = img->samples;

    format = cmd_meta_img_raw_format(cmd, img->layout.format);
    cmd_meta_set_src_for_buf(cmd, buf, format, &meta);

    for (i = 0; i < regionCount; i++) {
        const XGL_BUFFER_IMAGE_COPY *region = &pRegions[i];
        uint32_t j;

        meta.src.x = region->bufferOffset / icd_format_get_size(format);

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

ICD_EXPORT void XGLAPI xglCmdCopyImageToBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const XGL_BUFFER_IMAGE_COPY*                pRegions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(srcImage);
    struct intel_buf *buf = intel_buf(destBuffer);
    struct intel_cmd_meta meta;
    XGL_FORMAT img_format, buf_format;
    uint32_t i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_VS_POINTS;

    img_format = cmd_meta_img_raw_format(cmd, img->layout.format);

    /* buf_format is ignored by hw, but we derive stride from it */
    switch (img_format) {
    case XGL_FMT_R8_UINT:
        meta.shader_id = INTEL_DEV_META_VS_COPY_R8_TO_MEM;
        buf_format = XGL_FMT_R8G8B8A8_UINT;
        break;
    case XGL_FMT_R16_UINT:
        meta.shader_id = INTEL_DEV_META_VS_COPY_R16_TO_MEM;
        buf_format = XGL_FMT_R8G8B8A8_UINT;
        break;
    case XGL_FMT_R32_UINT:
        meta.shader_id = INTEL_DEV_META_VS_COPY_R32_TO_MEM;
        buf_format = XGL_FMT_R32G32B32A32_UINT;
        break;
    case XGL_FMT_R32G32_UINT:
        meta.shader_id = INTEL_DEV_META_VS_COPY_R32G32_TO_MEM;
        buf_format = XGL_FMT_R32G32B32A32_UINT;
        break;
    case XGL_FMT_R32G32B32A32_UINT:
        meta.shader_id = INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM;
        buf_format = XGL_FMT_R32G32B32A32_UINT;
        break;
    default:
        img_format = XGL_FMT_UNDEFINED;
        buf_format = XGL_FMT_UNDEFINED;
        break;
    }

    if (img_format == XGL_FMT_UNDEFINED ||
        (cmd_gen(cmd) == INTEL_GEN(6) &&
         icd_format_get_size(img_format) < 4)) {
        intel_dev_log(cmd->dev, XGL_DBG_MSG_ERROR,
                XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                "xglCmdCopyImageToBuffer with bpp %d unsupported",
                icd_format_get_size(img->layout.format));
        cmd_fail(cmd, XGL_ERROR_UNKNOWN);
        return;
    }

    cmd_meta_set_src_for_img(cmd, img, img_format,
            XGL_IMAGE_ASPECT_COLOR, &meta);
    cmd_meta_set_dst_for_buf(cmd, buf, buf_format, &meta);

    meta.samples = 1;

    for (i = 0; i < regionCount; i++) {
        const XGL_BUFFER_IMAGE_COPY *region = &pRegions[i];
        uint32_t j;

        meta.src.lod = region->imageSubresource.mipLevel;
        meta.src.layer = region->imageSubresource.arraySlice +
            region->imageOffset.z;
        meta.src.x = region->imageOffset.x;
        meta.src.y = region->imageOffset.y;

        meta.dst.x = region->bufferOffset / icd_format_get_size(img_format);
        meta.width = region->imageExtent.width;
        meta.height = region->imageExtent.height;

        for (j = 0; j < region->imageExtent.depth; j++) {
            cmd_draw_meta(cmd, &meta);

            meta.src.layer++;
            meta.dst.x += meta.width * meta.height;
        }
    }
}

ICD_EXPORT void XGLAPI xglCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_LAYOUT                            srcImageLayout,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_LAYOUT                            destImageLayout)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *src = intel_img(srcImage);
    struct intel_img *dst = intel_img(destImage);
    struct intel_buf *src_buf, *dst_buf;
    XGL_BUFFER_CREATE_INFO buf_info;
    XGL_BUFFER_COPY buf_region;
    XGL_RESULT res;

    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = src->obj.mem->size;

    memset(&buf_region, 0, sizeof(buf_region));
    buf_region.copySize = src->obj.mem->size;

    res = intel_buf_create(cmd->dev, &buf_info, &src_buf);
    if (res != XGL_SUCCESS) {
        cmd_fail(cmd, res);
        return;
    }

    res = intel_buf_create(cmd->dev, &buf_info, &dst_buf);
    if (res != XGL_SUCCESS) {
        intel_buf_destroy(src_buf);
        cmd_fail(cmd, res);
        return;
    }

    intel_obj_bind_mem(&src_buf->obj, src->obj.mem, 0);
    intel_obj_bind_mem(&dst_buf->obj, dst->obj.mem, 0);

    cmd_batch_flush(cmd, GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH);
    xglCmdCopyBuffer(cmdBuffer, (XGL_BUFFER) src_buf,
            (XGL_BUFFER) dst_buf, 1, &buf_region);

    intel_buf_destroy(src_buf);
    intel_buf_destroy(dst_buf);
}

ICD_EXPORT void XGLAPI xglCmdUpdateBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const uint32_t*                             pData)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_buf *dst = intel_buf(destBuffer);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    uint32_t *ptr;
    uint32_t offset;

    /* must be 4-byte aligned */
    if ((destOffset | dataSize) & 3) {
        cmd_fail(cmd, XGL_ERROR_UNKNOWN);
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
    format = XGL_FMT_R32G32B32A32_UINT;

    cmd_meta_set_src_for_writer(cmd, INTEL_CMD_WRITER_STATE,
            offset + dataSize, format, &meta);
    cmd_meta_set_dst_for_buf(cmd, dst, format, &meta);

    cmd_draw_meta(cmd, &meta);
}

ICD_EXPORT void XGLAPI xglCmdFillBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    uint32_t                                    data)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_buf *dst = intel_buf(destBuffer);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;

    /* must be 4-byte aligned */
    if ((destOffset | fillSize) & 3) {
        cmd_fail(cmd, XGL_ERROR_UNKNOWN);
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
    format = XGL_FMT_R32G32B32A32_UINT;

    cmd_meta_set_dst_for_buf(cmd, dst, format, &meta);

    cmd_draw_meta(cmd, &meta);
}

static void cmd_meta_clear_image(struct intel_cmd *cmd,
                                 struct intel_img *img,
                                 XGL_FORMAT format,
                                 struct intel_cmd_meta *meta,
                                 const XGL_IMAGE_SUBRESOURCE_RANGE *range)
{
    uint32_t mip_levels, array_size;
    uint32_t i, j;

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

        /* TODO INTEL_CMD_META_DS_HIZ_CLEAR requires 8x4 aligned rectangle */
        meta->width = u_minify(img->layout.width0, meta->dst.lod);
        meta->height = u_minify(img->layout.height0, meta->dst.lod);

        if (meta->ds.op != INTEL_CMD_META_DS_NOP &&
            !intel_img_can_enable_hiz(img, meta->dst.lod))
            continue;

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
            }

            meta->dst.layer++;
        }
    }
}

void cmd_meta_ds_op(struct intel_cmd *cmd,
                    enum intel_cmd_meta_ds_op op,
                    struct intel_img *img,
                    const XGL_IMAGE_SUBRESOURCE_RANGE *range)
{
    struct intel_cmd_meta meta;

    if (img->layout.aux != INTEL_LAYOUT_AUX_HIZ)
        return;
    if (range->aspect != XGL_IMAGE_ASPECT_DEPTH)
        return;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_DEPTH_STENCIL_RECT;
    meta.samples = img->samples;

    meta.ds.aspect = XGL_IMAGE_ASPECT_DEPTH;
    meta.ds.op = op;
    meta.ds.optimal = true;

    cmd_meta_clear_image(cmd, img, img->layout.format, &meta, range);
}

ICD_EXPORT void XGLAPI xglCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const float                                 color[4],
    uint32_t                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(image);
    struct intel_cmd_meta meta;
    uint32_t i;

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

ICD_EXPORT void XGLAPI xglCmdClearColorImageRaw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const uint32_t                              color[4],
    uint32_t                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(image);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    uint32_t i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_FS_RECT;

    meta.shader_id = INTEL_DEV_META_FS_CLEAR_COLOR;
    meta.samples = img->samples;

    icd_format_get_raw_value(img->layout.format, color, meta.clear_val);
    format = cmd_meta_img_raw_format(cmd, img->layout.format);

    for (i = 0; i < rangeCount; i++)
        cmd_meta_clear_image(cmd, img, format, &meta, &pRanges[i]);
}

ICD_EXPORT void XGLAPI xglCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *img = intel_img(image);
    struct intel_cmd_meta meta;
    uint32_t i;

    memset(&meta, 0, sizeof(meta));
    meta.mode = INTEL_CMD_META_DEPTH_STENCIL_RECT;

    meta.shader_id = INTEL_DEV_META_FS_CLEAR_DEPTH;
    meta.samples = img->samples;

    meta.clear_val[0] = u_fui(depth);
    meta.clear_val[1] = stencil;

    /* assume optimal DS until revision 59 */
    meta.ds.optimal = true;

    for (i = 0; i < rangeCount; i++) {
        const XGL_IMAGE_SUBRESOURCE_RANGE *range = &pRanges[i];

        cmd_meta_clear_image(cmd, img, img->layout.format,
                &meta, range);
    }
}

ICD_EXPORT void XGLAPI xglCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    uint32_t                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_img *src = intel_img(srcImage);
    struct intel_img *dst = intel_img(destImage);
    struct intel_cmd_meta meta;
    XGL_FORMAT format;
    uint32_t i;

    if (src->samples <= 1 || dst->samples > 1 ||
        src->layout.format != dst->layout.format) {
        cmd_fail(cmd, XGL_ERROR_UNKNOWN);
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
