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
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <xgl.h>

#define ASSERT_XGL_SUCCESS(err) ASSERT_EQ(XGL_SUCCESS, err) << xgl_result_string(err)

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define ERR(err) printf("%s:%d: failed with %s\n", \
    __FILE__, __LINE__, xgl_result_string(err));

#define ERR_EXIT(err) do { ERR(err); exit(-1); } while (0)

#define ERR_MSG(msg) printf("%s:%d: failed with %s\n", \
    __FILE__, __LINE__, msg);

#define ERR_MSG_EXIT(err) do { ERR_MSG(err); exit(-1); } while (0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define MAX_GPUS 8

#define MAX_QUEUE_TYPES 5

struct app_dev {
    struct app_gpu *gpu; /* point back to the GPU */

    XGL_DEVICE obj;

    XGL_UINT heap_count;
    XGL_MEMORY_HEAP_PROPERTIES *heap_props;
    XGL_QUEUE queues[MAX_QUEUE_TYPES];

    XGL_FORMAT_PROPERTIES format_props[XGL_MAX_CH_FMT][XGL_MAX_NUM_FMT];
};

struct app_gpu {
    XGL_UINT id;
    XGL_PHYSICAL_GPU obj;

    XGL_PHYSICAL_GPU_PROPERTIES props;
    XGL_PHYSICAL_GPU_PERFORMANCE perf;

    XGL_UINT queue_count;
    XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *queue_props;
    XGL_DEVICE_QUEUE_CREATE_INFO *queue_reqs;

    XGL_PHYSICAL_GPU_MEMORY_PROPERTIES memory_props;

    XGL_UINT extension_count;
    const XGL_CHAR **extensions;

    struct app_dev dev;
};

static const char *xgl_result_string(XGL_RESULT err)
{
    switch (err) {
#define STR(r) case r: return #r
    STR(XGL_SUCCESS);
    STR(XGL_UNSUPPORTED);
    STR(XGL_NOT_READY);
    STR(XGL_TIMEOUT);
    STR(XGL_EVENT_SET);
    STR(XGL_EVENT_RESET);
    STR(XGL_ERROR_UNKNOWN);
    STR(XGL_ERROR_UNAVAILABLE);
    STR(XGL_ERROR_INITIALIZATION_FAILED);
    STR(XGL_ERROR_OUT_OF_MEMORY);
    STR(XGL_ERROR_OUT_OF_GPU_MEMORY);
    STR(XGL_ERROR_DEVICE_ALREADY_CREATED);
    STR(XGL_ERROR_DEVICE_LOST);
    STR(XGL_ERROR_INVALID_POINTER);
    STR(XGL_ERROR_INVALID_VALUE);
    STR(XGL_ERROR_INVALID_HANDLE);
    STR(XGL_ERROR_INVALID_ORDINAL);
    STR(XGL_ERROR_INVALID_MEMORY_SIZE);
    STR(XGL_ERROR_INVALID_EXTENSION);
    STR(XGL_ERROR_INVALID_FLAGS);
    STR(XGL_ERROR_INVALID_ALIGNMENT);
    STR(XGL_ERROR_INVALID_FORMAT);
    STR(XGL_ERROR_INVALID_IMAGE);
    STR(XGL_ERROR_INVALID_DESCRIPTOR_SET_DATA);
    STR(XGL_ERROR_INVALID_QUEUE_TYPE);
    STR(XGL_ERROR_INVALID_OBJECT_TYPE);
    STR(XGL_ERROR_UNSUPPORTED_SHADER_IL_VERSION);
    STR(XGL_ERROR_BAD_SHADER_CODE);
    STR(XGL_ERROR_BAD_PIPELINE_DATA);
    STR(XGL_ERROR_TOO_MANY_MEMORY_REFERENCES);
    STR(XGL_ERROR_NOT_MAPPABLE);
    STR(XGL_ERROR_MEMORY_MAP_FAILED);
    STR(XGL_ERROR_MEMORY_UNMAP_FAILED);
    STR(XGL_ERROR_INCOMPATIBLE_DEVICE);
    STR(XGL_ERROR_INCOMPATIBLE_DRIVER);
    STR(XGL_ERROR_INCOMPLETE_COMMAND_BUFFER);
    STR(XGL_ERROR_BUILDING_COMMAND_BUFFER);
    STR(XGL_ERROR_MEMORY_NOT_BOUND);
    STR(XGL_ERROR_INCOMPATIBLE_QUEUE);
    STR(XGL_ERROR_NOT_SHAREABLE);
#undef STR
    default: return "UNKNOWN_RESULT";
    }
}

void app_dev_init_formats(struct app_dev *dev);
void app_dev_init(struct app_dev *dev, struct app_gpu *gpu);
void app_dev_destroy(struct app_dev *dev);
void app_gpu_init_extensions(struct app_gpu *gpu);
void app_gpu_init(struct app_gpu *gpu, XGL_UINT id, XGL_PHYSICAL_GPU obj);
void app_gpu_destroy(struct app_gpu *gpu);
void app_dev_init_queue(struct app_dev *dev, XGL_QUEUE_TYPE qtype);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // COMMON_H
