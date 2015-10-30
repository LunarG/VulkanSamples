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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 *
 */

#ifndef SAMPLER_H
#define SAMPLER_H

#include "intel.h"
#include "obj.h"

struct intel_sampler {
    struct intel_obj obj;

    /*
     * SAMPLER_STATE
     * SAMPLER_BORDER_COLOR_STATE
     */
    uint32_t cmd[15];
};

static inline struct intel_sampler *intel_sampler(VkSampler sampler)
{
    return *(struct intel_sampler **) &sampler;
}

static inline struct intel_sampler *intel_sampler_from_obj(struct intel_obj *obj)
{
    return (struct intel_sampler *) obj;
}

VkResult intel_sampler_create(struct intel_dev *dev,
                                const VkSamplerCreateInfo *info,
                                struct intel_sampler **sampler_ret);
void intel_sampler_destroy(struct intel_sampler *sampler);

#endif /* SAMPLER_H */
