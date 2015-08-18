/*
 * Vulkan
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
#include "vk_debug_report_lunarg.h"

typedef enum _DYNAMIC_STATE_BIND_POINT
{
    VK_STATE_BIND_POINT_VIEWPORT,
    VK_STATE_BIND_POINT_RASTER_LINE,
    VK_STATE_BIND_POINT_RASTER_DEPTH_BIAS,
    VK_STATE_BIND_POINT_COLOR_BLEND,
    VK_STATE_BIND_POINT_DEPTH,
    VK_STATE_BIND_POINT_STENCIL,
    VK_NUM_STATE_BIND_POINT // Used for array sizing
} DYNAMIC_STATE_BIND_POINT;

static string string_DYNAMIC_STATE_BIND_POINT(DYNAMIC_STATE_BIND_POINT sbp)
{
    switch (sbp)
    {
        case VK_STATE_BIND_POINT_VIEWPORT:
            return "VIEWPORT";
        case VK_STATE_BIND_POINT_RASTER_LINE:
            return "RASTER_LINE";
        case VK_STATE_BIND_POINT_RASTER_DEPTH_BIAS:
            return "RASTER_DEPTH_BIAS";
        case VK_STATE_BIND_POINT_COLOR_BLEND:
            return "COLOR_BLEND";
        case VK_STATE_BIND_POINT_DEPTH:
            return "DEPTH";
        case VK_STATE_BIND_POINT_STENCIL:
            return "STENCIL";
        default:
            return "UNKNOWN_DYNAMIC_STATE_BIND_POINT";
    }
}

static VkDbgObjectType dynamicStateBindPointToObjType(DYNAMIC_STATE_BIND_POINT sbp)
{
    switch (sbp)
    {
        case VK_STATE_BIND_POINT_VIEWPORT:
            return VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE;
        case VK_STATE_BIND_POINT_RASTER_LINE:
            return VK_OBJECT_TYPE_DYNAMIC_RASTER_LINE_STATE;
        case VK_STATE_BIND_POINT_RASTER_DEPTH_BIAS:
            return VK_OBJECT_TYPE_DYNAMIC_RASTER_DEPTH_BIAS_STATE;
        case VK_STATE_BIND_POINT_COLOR_BLEND:
            return VK_OBJECT_TYPE_DYNAMIC_COLOR_BLEND_STATE;
        case VK_STATE_BIND_POINT_DEPTH:
            return VK_OBJECT_TYPE_DYNAMIC_DEPTH_STATE;
        case VK_STATE_BIND_POINT_STENCIL:
            return VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE;
        default:
            return VK_OBJECT_TYPE_MAX_ENUM;
    }
}
