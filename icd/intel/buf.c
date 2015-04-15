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
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "dev.h"
#include "obj.h"
#include "buf.h"

static void buf_destroy(struct intel_obj *obj)
{
    struct intel_buf *buf = intel_buf_from_obj(obj);

    intel_buf_destroy(buf);
}

static VK_RESULT buf_get_info(struct intel_base *base, int type,
                               size_t *size, void *data)
{
    struct intel_buf *buf = intel_buf_from_base(base);
    VK_RESULT ret = VK_SUCCESS;

    switch (type) {
    case VK_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            VK_MEMORY_REQUIREMENTS *mem_req = data;

            *size = sizeof(VK_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;

            /*
             * From the Sandy Bridge PRM, volume 1 part 1, page 118:
             *
             *     "For buffers, which have no inherent "height," padding
             *      requirements are different. A buffer must be padded to the
             *      next multiple of 256 array elements, with an additional 16
             *      bytes added beyond that to account for the L1 cache line."
             */
            mem_req->size = buf->size;
            if (buf->usage & VK_BUFFER_USAGE_SHADER_ACCESS_READ_BIT)
                mem_req->size = u_align(mem_req->size, 256) + 16;

            mem_req->alignment = 4096;
            mem_req->memType = VK_MEMORY_TYPE_BUFFER;

        }
        break;
        case VK_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        {
            VK_BUFFER_MEMORY_REQUIREMENTS *buf_req = data;

            *size = sizeof(VK_BUFFER_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            buf_req->usage = buf->usage;
        }
        break;
    default:
        ret = intel_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

VK_RESULT intel_buf_create(struct intel_dev *dev,
                            const VkBufferCreateInfo *info,
                            struct intel_buf **buf_ret)
{
    struct intel_buf *buf;

    buf = (struct intel_buf *) intel_base_create(&dev->base.handle,
            sizeof(*buf), dev->base.dbg, VK_DBG_OBJECT_BUFFER, info, 0);
    if (!buf)
        return VK_ERROR_OUT_OF_MEMORY;

    buf->size = info->size;
    buf->usage = info->usage;

    buf->obj.destroy = buf_destroy;
    buf->obj.base.get_info = buf_get_info;

    *buf_ret = buf;

    return VK_SUCCESS;
}

void intel_buf_destroy(struct intel_buf *buf)
{
    intel_base_destroy(&buf->obj.base);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateBuffer(
    VK_DEVICE                                  device,
    const VkBufferCreateInfo*               pCreateInfo,
    VK_BUFFER*                                 pBuffer)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_buf_create(dev, pCreateInfo, (struct intel_buf **) pBuffer);
}
