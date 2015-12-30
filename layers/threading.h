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
 * Author: Cody Northrop <cody@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 */
#ifndef THREADING_H
#define THREADING_H
#include "vk_layer_config.h"
#include "vk_layer_logging.h"

// Draw State ERROR codes
typedef enum _THREADING_CHECKER_ERROR
{
    THREADING_CHECKER_NONE,                             // Used for INFO & other non-error messages
    THREADING_CHECKER_MULTIPLE_THREADS,                 // Object used simultaneously by multiple threads
    THREADING_CHECKER_SINGLE_THREAD_REUSE,              // Object used simultaneously by recursion in single thread
} THREADING_CHECKER_ERROR;

struct layer_data {
    debug_report_data *report_data;
    VkDebugReportCallbackEXT   logging_callback;

    layer_data() :
        report_data(nullptr),
        logging_callback(VK_NULL_HANDLE)
    {};
};

static std::unordered_map<void*, layer_data *> layer_data_map;
static device_table_map                        threading_device_table_map;
static instance_table_map                      threading_instance_table_map;

static inline debug_report_data *mdd(const void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->report_data;
}
#endif // THREADING_H
