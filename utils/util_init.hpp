/*
 * Vulkan Samples Kit
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

#ifndef UTIL_INIT
#define UTIL_INIT

#include "util.hpp"

VkResult init_global_extension_properties(
        layer_properties &layer_props);

VkResult init_global_layer_properties(sample_info &info);
VkResult init_instance(struct sample_info &info, char const*const app_short_name);
VkResult init_device(struct sample_info &info);
VkResult init_enumerate_device(struct sample_info &info, uint32_t gpu_count = 1);
void init_connection(struct sample_info &info);
void init_window(struct sample_info &info);
void init_wsi(struct sample_info &info);
void init_command_buffer(struct sample_info &info);
void init_device_queue(struct sample_info &info);
void init_swap_chain(struct sample_info &info);
void init_depth_buffer(struct sample_info &info);
void init_descriptor_and_pipeline_layouts(struct sample_info &info);
void init_renderpass(struct sample_info &info);
void init_framebuffers(struct sample_info &info);
void init_dynamic_state(struct sample_info &info);


#endif // UTIL_INIT

