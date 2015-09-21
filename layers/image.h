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
 */
#ifndef IMAGE_H
#define IMAGE_H
#include "vulkan.h"
#include "vk_layer_config.h"
#include "vk_layer_logging.h"

// Draw State ERROR codes
typedef enum _IMAGE_ERROR
{
    IMAGE_NONE,                             // Used for INFO & other non-error messages
    IMAGE_FORMAT_UNSUPPORTED,               // Request to create Image or RenderPass with a format that is not supported
    IMAGE_RENDERPASS_INVALID_ATTACHMENT,    // Invalid image layouts and/or load/storeOps for an attachment when creating RenderPass
    IMAGE_RENDERPASS_INVALID_DS_ATTACHMENT, // If no depth attachment for a RenderPass, verify that subpass DS attachment is set to UNUSED
    IMAGE_VIEW_CREATE_ERROR,                // Error occurred trying to create Image View
} IMAGE_ERROR;

#endif // IMAGE_H
