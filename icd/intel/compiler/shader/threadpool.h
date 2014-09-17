/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014  LunarG, Inc.   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mesa_threadpool;
struct _mesa_threadpool_task;

struct _mesa_threadpool *
_mesa_threadpool_create(int max_threads);

struct _mesa_threadpool *
_mesa_threadpool_ref(struct _mesa_threadpool *pool);

void
_mesa_threadpool_unref(struct _mesa_threadpool *pool);

void
_mesa_threadpool_join(struct _mesa_threadpool *pool, bool graceful);

struct _mesa_threadpool_task *
_mesa_threadpool_queue_task(struct _mesa_threadpool *pool,
                            void (*func)(void *), void *data);

bool
_mesa_threadpool_complete_tasks(struct _mesa_threadpool *pool,
                                struct _mesa_threadpool_task **tasks,
                                int num_tasks);

bool
_mesa_threadpool_complete_task(struct _mesa_threadpool *pool,
                               struct _mesa_threadpool_task *task);

struct _mesa_threadpool *
_mesa_glsl_get_threadpool(int max_threads);

void
_mesa_glsl_wait_threadpool(void);

void
_mesa_glsl_destroy_threadpool(void);

#ifdef __cplusplus
}
#endif

#endif /* THREADPOOL_H */
