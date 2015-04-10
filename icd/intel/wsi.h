/*
 * Vulkan
 *
 * Copyright (C) 2015 LunarG, Inc.
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

#ifndef WSI_H
#define WSI_H

#include "intel.h"

struct intel_fence;
struct intel_gpu;
struct intel_img;

VkResult intel_wsi_gpu_get_info(struct intel_gpu *gpu,
                                  VkPhysicalGpuInfoType type,
                                  size_t *size, void *data);
void intel_wsi_gpu_cleanup(struct intel_gpu *gpu);

VkResult intel_wsi_img_init(struct intel_img *img);
void intel_wsi_img_cleanup(struct intel_img *img);

VkResult intel_wsi_fence_init(struct intel_fence *fence);
void intel_wsi_fence_cleanup(struct intel_fence *fence);
void intel_wsi_fence_copy(struct intel_fence *fence,
                          const struct intel_fence *src);
VkResult intel_wsi_fence_wait(struct intel_fence *fence,
                                int64_t timeout_ns);

#endif /* WSI_H */
