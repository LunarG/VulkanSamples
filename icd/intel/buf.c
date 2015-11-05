/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 *
 */

#include "dev.h"
#include "obj.h"
#include "buf.h"

static void buf_destroy(struct intel_obj *obj)
{
    struct intel_buf *buf = intel_buf_from_obj(obj);

    intel_buf_destroy(buf);
}

static VkResult buf_get_memory_requirements(struct intel_base *base,
                               VkMemoryRequirements *pRequirements)
{
    struct intel_buf *buf = intel_buf_from_base(base);

    /*
     * From the Sandy Bridge PRM, volume 1 part 1, page 118:
     *
     *     "For buffers, which have no inherent "height," padding
     *      requirements are different. A buffer must be padded to the
     *      next multiple of 256 array elements, with an additional 16
     *      bytes added beyond that to account for the L1 cache line."
    */
    pRequirements->size = buf->size;
    if (buf->usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
        pRequirements->size = u_align(pRequirements->size, 256) + 16;
    }

    pRequirements->alignment      = 4096;
    pRequirements->memoryTypeBits = (1 << INTEL_MEMORY_TYPE_COUNT) - 1;

    return VK_SUCCESS;
}

VkResult intel_buf_create(struct intel_dev *dev,
                            const VkBufferCreateInfo *info,
                            struct intel_buf **buf_ret)
{
    struct intel_buf *buf;

    buf = (struct intel_buf *) intel_base_create(&dev->base.handle,
            sizeof(*buf), dev->base.dbg, VK_OBJECT_TYPE_BUFFER, info, 0);
    if (!buf)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    buf->size = info->size;
    buf->usage = info->usage;

    buf->obj.destroy = buf_destroy;
    buf->obj.base.get_memory_requirements = buf_get_memory_requirements;

    *buf_ret = buf;

    return VK_SUCCESS;
}

void intel_buf_destroy(struct intel_buf *buf)
{
    intel_base_destroy(&buf->obj.base);
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
    VkDevice                                  device,
    const VkBufferCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkBuffer*                                 pBuffer)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_buf_create(dev, pCreateInfo, (struct intel_buf **) pBuffer);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
    VkDevice                                device,
    VkBuffer                                buffer,
    const VkAllocationCallbacks*                     pAllocator)
{
    struct intel_obj *obj = intel_obj(buffer);

    obj->destroy(obj);
}
