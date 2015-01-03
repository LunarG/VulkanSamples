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

#ifndef EVENT_H
#define EVENT_H

#include "intel.h"
#include "obj.h"

struct intel_dev;

struct intel_event {
    struct intel_obj obj;
};

static inline struct intel_event *intel_event(XGL_EVENT event)
{
    return (struct intel_event *) event;
}

static inline struct intel_event *intel_event_from_obj(struct intel_obj *obj)
{
    return (struct intel_event *) obj;
}

XGL_RESULT intel_event_create(struct intel_dev *dev,
                              const XGL_EVENT_CREATE_INFO *info,
                              struct intel_event **event_ret);
void intel_event_destroy(struct intel_event *event);

XGL_RESULT intel_event_set(struct intel_event *event);
XGL_RESULT intel_event_reset(struct intel_event *event);
XGL_RESULT intel_event_get_status(struct intel_event *event);

#endif /* EVENT_H */
