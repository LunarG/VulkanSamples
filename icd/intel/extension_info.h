/*
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

#ifndef EXTENSION_INFO_H
#define EXTENSION_INFO_H

#include "intel.h"

enum intel_phy_dev_ext_type {
    INTEL_PHY_DEV_EXT_WSI_DEVICE_SWAPCHAIN,

    INTEL_PHY_DEV_EXT_COUNT,
    INTEL_PHY_DEV_EXT_INVALID = INTEL_PHY_DEV_EXT_COUNT,
};

enum intel_global_ext_type {
    INTEL_PHY_DEV_EXT_DEBUG_REPORT,
    INTEL_GLOBAL_EXT_WSI_SWAPCHAIN,

    INTEL_GLOBAL_EXT_COUNT,
    INTEL_GLOBAL_EXT_INVALID = INTEL_GLOBAL_EXT_COUNT,
};

extern const VkExtensionProperties intel_phy_dev_gpu_exts[INTEL_PHY_DEV_EXT_COUNT];
extern const VkExtensionProperties intel_global_exts[INTEL_GLOBAL_EXT_COUNT];

bool compare_vk_extension_properties(
        const VkExtensionProperties *op1,
        const char                  *extensionName);
#endif /* EXTENSION_INFO_H */
