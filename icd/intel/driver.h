/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifndef __DRIVER_H_INCLUDED__
#define __DRIVER_H_INCLUDED__

#include "intel.h"

/**
 * Typedefs for all XGL API entrypoint functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Base class for device drivers.
 */
#define MAX_DRIVER_NAME 64

struct _xgl_device {
    /* MUST be first element of structure */
    const struct icd_dispatch_table *exec;

    int fd;                 /* file descriptor of render-node */
    uint32_t dev_id;
    uint32_t ven_id;
    char device_path[XGL_MAX_PHYSICAL_GPU_NAME];
    char driver_name[MAX_DRIVER_NAME];
    void *driver_handle;
};


struct _xgl_config {
    XGL_UINT num_gpus;
    struct _xgl_device gpu_info[XGL_MAX_PHYSICAL_GPUS];
    XGL_PHYSICAL_GPU render_node_list[XGL_MAX_PHYSICAL_GPUS];
};

extern struct _xgl_config icd_data;

#ifdef __cplusplus
}                               // extern "C"
#endif                          // __cplusplus
#endif                          /* __DRIVER_H_INCLUDED__ */
