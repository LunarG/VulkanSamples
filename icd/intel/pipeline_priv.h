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

#ifndef PIPELINE_PRIV_H
#define PIPELINE_PRIV_H

#include "intel.h"
#include "pipeline.h"

struct intel_pipeline_create_info {
    const struct intel_gpu *gpu;

    XGL_GRAPHICS_PIPELINE_CREATE_INFO   graphics;
    XGL_PIPELINE_IA_STATE_CREATE_INFO   ia;
    XGL_PIPELINE_DB_STATE_CREATE_INFO   db;
    XGL_PIPELINE_CB_STATE               cb;
    XGL_PIPELINE_RS_STATE_CREATE_INFO   rs;
    XGL_PIPELINE_TESS_STATE_CREATE_INFO tess;
    XGL_PIPELINE_SHADER                 vs;
    XGL_PIPELINE_SHADER                 tcs;
    XGL_PIPELINE_SHADER                 tes;
    XGL_PIPELINE_SHADER                 gs;
    XGL_PIPELINE_SHADER                 fs;

    XGL_COMPUTE_PIPELINE_CREATE_INFO    compute;
    XGL_PIPELINE_SHADER                 cs;
};

struct intel_rmap *intel_rmap_create(struct intel_dev *dev,
                                     const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                                     const XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO *dyn,
                                     XGL_UINT rt_count);
void intel_rmap_destroy(struct intel_rmap *rmap);

#endif /* PIPELINE_PRIV_H */
