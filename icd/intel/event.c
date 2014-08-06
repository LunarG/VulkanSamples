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

#include "kmd/winsys.h"
#include "dev.h"
#include "mem.h"
#include "event.h"

static XGL_RESULT event_map(struct intel_event *event, uint32_t **ptr_ret)
{
    void *ptr;

    if (!event->obj.mem)
        return XGL_ERROR_MEMORY_NOT_BOUND;

    /*
     * This is an unsynchronous mapping.  It doesn't look like we want a
     * synchronous mapping.  But it is also unclear what would happen when GPU
     * writes to it at the same time.  We need atomicy here.
     */
    ptr = intel_mem_map(event->obj.mem, 0);
    if (!ptr)
        return XGL_ERROR_MEMORY_MAP_FAILED;

    *ptr_ret = (uint32_t *) ((uint8_t *) ptr + event->obj.offset);

    return XGL_SUCCESS;
}

static void event_unmap(struct intel_event *event)
{
    intel_mem_unmap(event->obj.mem);
}

static XGL_RESULT event_write(struct intel_event *event, uint32_t val)
{
    XGL_RESULT ret;
    uint32_t *ptr;

    ret = event_map(event, &ptr);
    if (ret == XGL_SUCCESS) {
        *ptr = val;
        event_unmap(event);
    }

    return ret;
}

static XGL_RESULT event_read(struct intel_event *event, uint32_t *val)
{
    XGL_RESULT ret;
    uint32_t *ptr;

    ret = event_map(event, &ptr);
    if (ret == XGL_SUCCESS) {
        *val = *ptr;
        event_unmap(event);
    }

    return ret;
}

static void event_destroy_callback(struct intel_obj *obj)
{
    struct intel_event *event = intel_event_from_obj(obj);

    intel_event_destroy(event);
}

XGL_RESULT intel_event_create(struct intel_dev *dev,
                              const XGL_EVENT_CREATE_INFO *info,
                              struct intel_event **event_ret)
{
    struct intel_event *event;
    XGL_MEMORY_REQUIREMENTS *mem_req;

    event = icd_alloc(sizeof(*event), 0, XGL_SYSTEM_ALLOC_API_OBJECT);
    if (!event)
        return XGL_ERROR_OUT_OF_MEMORY;

    memset(event, 0, sizeof(*event));

    event->obj.destroy = event_destroy_callback;

    /* use dword aligned to 64-byte boundaries */
    mem_req = &event->obj.base.mem_requirements;
    mem_req->size = 4;
    mem_req->alignment = 64;
    mem_req->heapCount = 1;
    mem_req->heaps[0] = 0;

    event->obj.base.dispatch = dev->base.dispatch;
    if (dev->base.dbg) {
        event->obj.base.dbg =
            intel_base_dbg_create(XGL_DBG_OBJECT_EVENT, info, sizeof(*info));
        if (!event->obj.base.dbg) {
            icd_free(event);
            return XGL_ERROR_OUT_OF_MEMORY;
        }
    }

    *event_ret = event;

    return XGL_SUCCESS;
}

void intel_event_destroy(struct intel_event *event)
{
    if (event->obj.base.dbg)
        intel_base_dbg_destroy(event->obj.base.dbg);

    icd_free(event);
}

XGL_RESULT intel_event_set(struct intel_event *event)
{
    return event_write(event, 1);
}

XGL_RESULT intel_event_reset(struct intel_event *event)
{
    return event_write(event, 0);
}

XGL_RESULT intel_event_get_status(struct intel_event *event)
{
    XGL_RESULT ret;
    uint32_t val;

    ret = event_read(event, &val);
    if (ret != XGL_SUCCESS)
        return ret;

    return (val) ? XGL_EVENT_SET : XGL_EVENT_RESET;
}

XGL_RESULT XGLAPI intelCreateEvent(
    XGL_DEVICE                                  device,
    const XGL_EVENT_CREATE_INFO*                pCreateInfo,
    XGL_EVENT*                                  pEvent)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_event_create(dev, pCreateInfo,
            (struct intel_event **) pEvent);
}

XGL_RESULT XGLAPI intelGetEventStatus(
    XGL_EVENT                                   event_)
{
    struct intel_event *event = intel_event(event_);

    return intel_event_get_status(event);
}

XGL_RESULT XGLAPI intelSetEvent(
    XGL_EVENT                                   event_)
{
    struct intel_event *event = intel_event(event_);

    return intel_event_set(event);
}

XGL_RESULT XGLAPI intelResetEvent(
    XGL_EVENT                                   event_)
{
    struct intel_event *event = intel_event(event_);

    return intel_event_reset(event);
}
