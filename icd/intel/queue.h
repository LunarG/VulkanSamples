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

#ifndef QUEUE_H
#define QUEUE_H

#include "intel.h"
#include "obj.h"

struct intel_bo;
struct intel_dev;

struct intel_queue {
    struct intel_base base;

    struct intel_dev *dev;
    struct intel_bo *last_submitted_bo;
};

static inline struct intel_queue *intel_queue(XGL_QUEUE queue)
{
    return (struct intel_queue *) queue;
}

XGL_RESULT intel_queue_create(struct intel_dev *dev,
                              XGL_QUEUE_TYPE type,
                              struct intel_queue **queue_ret);
void intel_queue_destroy(struct intel_queue *queue);

XGL_RESULT intel_queue_wait(struct intel_queue *queue, int64_t timeout);

XGL_RESULT XGLAPI intelQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs);

XGL_RESULT XGLAPI intelQueueWaitIdle(
    XGL_QUEUE                                   queue);

#endif /* QUEUE_H */
