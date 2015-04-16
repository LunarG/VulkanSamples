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
#include "mem.h"
#include "event.h"

static VkResult event_map(struct intel_event *event, uint32_t **ptr_ret)
{
    void *ptr;

    if (!event->obj.mem)
        return VK_ERROR_MEMORY_NOT_BOUND;

    /*
     * This is an unsynchronous mapping.  It doesn't look like we want a
     * synchronous mapping.  But it is also unclear what would happen when GPU
     * writes to it at the same time.  We need atomicy here.
     */
    ptr = intel_mem_map(event->obj.mem, 0);
    if (!ptr)
        return VK_ERROR_MEMORY_MAP_FAILED;

    *ptr_ret = (uint32_t *) ((uint8_t *) ptr + event->obj.offset);

    return VK_SUCCESS;
}

static void event_unmap(struct intel_event *event)
{
    intel_mem_unmap(event->obj.mem);
}

static VkResult event_write(struct intel_event *event, uint32_t val)
{
    VkResult ret;
    uint32_t *ptr;

    ret = event_map(event, &ptr);
    if (ret == VK_SUCCESS) {
        *ptr = val;
        event_unmap(event);
    }

    return ret;
}

static VkResult event_read(struct intel_event *event, uint32_t *val)
{
    VkResult ret;
    uint32_t *ptr;

    ret = event_map(event, &ptr);
    if (ret == VK_SUCCESS) {
        *val = *ptr;
        event_unmap(event);
    }

    return ret;
}

static void event_destroy(struct intel_obj *obj)
{
    struct intel_event *event = intel_event_from_obj(obj);

    intel_event_destroy(event);
}

static VkResult event_get_info(struct intel_base *base, int type,
                                 size_t *size, void *data)
{
    VkResult ret = VK_SUCCESS;

    switch (type) {
    case VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            VkMemoryRequirements *mem_req = data;

            *size = sizeof(VkMemoryRequirements);
            if (data == NULL)
                return ret;
            /* use dword aligned to 64-byte boundaries */
            mem_req->size = 4;
            mem_req->alignment = 64;
            mem_req->memPropsAllowed = INTEL_MEMORY_PROPERTY_ALL;
        }
        break;
    default:
        ret = intel_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

VkResult intel_event_create(struct intel_dev *dev,
                              const VkEventCreateInfo *info,
                              struct intel_event **event_ret)
{
    struct intel_event *event;

    event = (struct intel_event *) intel_base_create(&dev->base.handle,
            sizeof(*event), dev->base.dbg, VK_DBG_OBJECT_EVENT, info, 0);
    if (!event)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    event->obj.base.get_info = event_get_info;
    event->obj.destroy = event_destroy;

    *event_ret = event;

    return VK_SUCCESS;
}

void intel_event_destroy(struct intel_event *event)
{
    intel_base_destroy(&event->obj.base);
}

VkResult intel_event_set(struct intel_event *event)
{
    return event_write(event, 1);
}

VkResult intel_event_reset(struct intel_event *event)
{
    return event_write(event, 0);
}

VkResult intel_event_get_status(struct intel_event *event)
{
    VkResult ret;
    uint32_t val;

    ret = event_read(event, &val);
    if (ret != VK_SUCCESS)
        return ret;

    return (val) ? VK_EVENT_SET : VK_EVENT_RESET;
}

ICD_EXPORT VkResult VKAPI vkCreateEvent(
    VkDevice                                  device,
    const VkEventCreateInfo*                pCreateInfo,
    VkEvent*                                  pEvent)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_event_create(dev, pCreateInfo,
            (struct intel_event **) pEvent);
}

ICD_EXPORT VkResult VKAPI vkGetEventStatus(
    VkEvent                                   event_)
{
    struct intel_event *event = intel_event(event_);

    return intel_event_get_status(event);
}

ICD_EXPORT VkResult VKAPI vkSetEvent(
    VkEvent                                   event_)
{
    struct intel_event *event = intel_event(event_);

    return intel_event_set(event);
}

ICD_EXPORT VkResult VKAPI vkResetEvent(
    VkEvent                                   event_)
{
    struct intel_event *event = intel_event(event_);

    return intel_event_reset(event);
}
