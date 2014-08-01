/*
 * XGL 3-D graphics library
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
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include "driver.h"

XGL_RESULT gen7_GetGpuInfo(XGL_PHYSICAL_GPU gpu,
                           XGL_PHYSICAL_GPU_INFO_TYPE infoType,
                           XGL_SIZE * pDataSize, XGL_VOID * pData)
{
    XGL_RESULT ret = XGL_SUCCESS;
    struct _xgl_device *pdev = (struct _xgl_device *) gpu;

    switch (infoType) {
    case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES:
        break;

    default:
        ret = XGL_ERROR_INVALID_VALUE;
    }

    return ret;
}
