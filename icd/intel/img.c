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

#include <unistd.h>
#include "kmd/winsys.h"
#include "dev.h"
#include "gpu.h"
#include "mem.h"
#include "img.h"

/*
 * From the Ivy Bridge PRM, volume 1 part 1, page 105:
 *
 *     "In addition to restrictions on maximum height, width, and depth,
 *      surfaces are also restricted to a maximum size in bytes. This
 *      maximum is 2 GB for all products and all surface types."
 */
static const size_t intel_max_resource_size = 1u << 31;

static void img_destroy(struct intel_obj *obj)
{
    struct intel_img *img = intel_img_from_obj(obj);

    intel_img_destroy(img);
}

static XGL_RESULT img_get_info(struct intel_base *base, int type,
                               XGL_SIZE *size, XGL_VOID *data)
{
    struct intel_img *img = intel_img_from_base(base);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            XGL_MEMORY_REQUIREMENTS *mem_req = data;

            mem_req->size = img->total_size;
            mem_req->alignment = 4096;
            mem_req->heapCount = 1;
            mem_req->heaps[0] = 0;

            *size = sizeof(*mem_req);
        }
        break;
    default:
        ret = intel_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

XGL_RESULT intel_img_create(struct intel_dev *dev,
                            const XGL_IMAGE_CREATE_INFO *info,
                            bool scanout,
                            struct intel_img **img_ret)
{
    struct intel_img *img;
    struct intel_layout *layout;

    img = (struct intel_img *) intel_base_create(dev, sizeof(*img),
            dev->base.dbg, XGL_DBG_OBJECT_IMAGE, info, 0);
    if (!img)
        return XGL_ERROR_OUT_OF_MEMORY;

    layout = &img->layout;

    img->type = info->imageType;
    img->depth = info->extent.depth;
    img->mip_levels = info->mipLevels;
    img->array_size = info->arraySize;
    img->samples = info->samples;
    intel_layout_init(layout, dev, info, scanout);

    if (layout->bo_stride > intel_max_resource_size / layout->bo_height) {
        intel_dev_log(dev, XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0,
                XGL_NULL_HANDLE, 0, 0, "image too big");
        intel_img_destroy(img);
        return XGL_ERROR_INVALID_MEMORY_SIZE;
    }

    img->total_size = img->layout.bo_stride * img->layout.bo_height;

    if (layout->aux != INTEL_LAYOUT_AUX_NONE) {
        img->aux_offset = u_align(img->total_size, 4096);
        img->total_size = img->aux_offset +
            layout->aux_stride * layout->aux_height;
    }

    if (layout->separate_stencil) {
        XGL_IMAGE_CREATE_INFO s8_info;

        img->s8_layout = icd_alloc(sizeof(*img->s8_layout), 0,
                XGL_SYSTEM_ALLOC_INTERNAL);
        if (!img->s8_layout) {
            intel_img_destroy(img);
            return XGL_ERROR_OUT_OF_MEMORY;
        }

        s8_info = *info;
        s8_info.format.channelFormat = XGL_CH_FMT_R8;
        assert(info->format.numericFormat == XGL_NUM_FMT_DS);

        intel_layout_init(img->s8_layout, dev, &s8_info, scanout);

        img->s8_offset = u_align(img->total_size, 4096);
        img->total_size = img->s8_offset +
            img->s8_layout->bo_stride * img->s8_layout->bo_height;
    }

    img->obj.destroy = img_destroy;
    img->obj.base.get_info = img_get_info;

#ifdef ENABLE_WSI_X11
    img->x11_prime_fd = -1;
#endif

    *img_ret = img;

    return XGL_SUCCESS;
}

void intel_img_destroy(struct intel_img *img)
{
#ifdef ENABLE_WSI_X11
    if (img->x11_prime_fd >= 0) {
        close(img->x11_prime_fd);
        intel_mem_free(img->obj.mem);
    }
#endif

    if (img->s8_layout)
        icd_free(img->s8_layout);

    intel_base_destroy(&img->obj.base);
}

XGL_RESULT XGLAPI intelOpenPeerImage(
    XGL_DEVICE                                  device,
    const XGL_PEER_IMAGE_OPEN_INFO*             pOpenInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelCreateImage(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    XGL_IMAGE*                                  pImage)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_img_create(dev, pCreateInfo, false,
            (struct intel_img **) pImage);
}

XGL_RESULT XGLAPI intelGetImageSubresourceInfo(
    XGL_IMAGE                                   image,
    const XGL_IMAGE_SUBRESOURCE*                pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    const struct intel_img *img = intel_img(image);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (infoType) {
    case XGL_INFO_TYPE_SUBRESOURCE_LAYOUT:
        {
            XGL_SUBRESOURCE_LAYOUT *layout = (XGL_SUBRESOURCE_LAYOUT *) pData;
            unsigned x, y;

            intel_layout_get_slice_pos(&img->layout, pSubresource->mipLevel,
                    pSubresource->arraySlice, &x, &y);
            intel_layout_pos_to_mem(&img->layout, x, y, &x, &y);

            *pDataSize = sizeof(XGL_SUBRESOURCE_LAYOUT);

            layout->offset = intel_layout_mem_to_linear(&img->layout, x, y);
            layout->size = intel_layout_get_slice_size(&img->layout,
                    pSubresource->mipLevel);
            layout->rowPitch = img->layout.bo_stride;
            layout->depthPitch = intel_layout_get_slice_stride(&img->layout,
                    pSubresource->mipLevel);
        }
        break;
    default:
        ret = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}
