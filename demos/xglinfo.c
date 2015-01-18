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
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <xgl.h>

#define ERR(err) printf("%s:%d: failed with %s\n", \
    __FILE__, __LINE__, xgl_result_string(err));

#define ERR_EXIT(err) do { ERR(err); exit(-1); } while (0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define MAX_GPUS 8

#define MAX_QUEUE_TYPES 5

struct app_gpu;

struct app_dev {
    struct app_gpu *gpu; /* point back to the GPU */

    XGL_DEVICE obj;

    XGL_UINT heap_count;
    XGL_MEMORY_HEAP_PROPERTIES *heap_props;

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

static const char *xgl_gpu_type_string(XGL_PHYSICAL_GPU_TYPE type)
{
    switch (type) {
#define STR(r) case XGL_GPU_TYPE_ ##r: return #r
    STR(OTHER);
    STR(INTEGRATED);
    STR(DISCRETE);
    STR(VIRTUAL);
#undef STR
    default: return "UNKNOWN_GPU";
    }
}

static const char *xgl_heap_type_string(XGL_HEAP_MEMORY_TYPE type)
{
    switch (type) {
#define STR(r) case XGL_HEAP_MEMORY_ ##r: return #r
    STR(OTHER);
    STR(LOCAL);
    STR(REMOTE);
    STR(EMBEDDED);
#undef STR
    default: return "UNKNOWN_HEAP";
    }
}

static const char *xgl_channel_format_string(XGL_CHANNEL_FORMAT ch)
{
    switch (ch) {
#define STR(r) case XGL_CH_FMT_ ##r: return #r
    STR(UNDEFINED);
    STR(R4G4);
    STR(R4G4B4A4);
    STR(R5G6B5);
    STR(B5G6R5);
    STR(R5G5B5A1);
    STR(R8);
    STR(R8G8);
    STR(R8G8B8A8);
    STR(B8G8R8A8);
    STR(R10G11B11);
    STR(R11G11B10);
    STR(R10G10B10A2);
    STR(R16);
    STR(R16G16);
    STR(R16G16B16A16);
    STR(R32);
    STR(R32G32);
    STR(R32G32B32);
    STR(R32G32B32A32);
    STR(R16G8);
    STR(R32G8);
    STR(R9G9B9E5);
    STR(BC1);
    STR(BC2);
    STR(BC3);
    STR(BC4);
    STR(BC5);
    STR(BC6U);
    STR(BC6S);
    STR(BC7);
    STR(R8G8B8);
    STR(R16G16B16);
    STR(B10G10R10A2);
    STR(R64);
    STR(R64G64);
    STR(R64G64B64);
    STR(R64G64B64A64);
#undef STR
    default: return "UNKNOWN_CH";
    }
}

static const char *xgl_numeric_format_string(XGL_NUM_FORMAT num)
{
    switch (num) {
#define STR(r) case XGL_NUM_FMT_ ##r: return #r
    STR(UNDEFINED);
    STR(UNORM);
    STR(SNORM);
    STR(UINT);
    STR(SINT);
    STR(FLOAT);
    STR(SRGB);
    STR(DS);
    STR(USCALED);
    STR(SSCALED);
#undef STR
    default: return "UNKNOWN_NUM";
    }
}

static void app_dev_init_formats(struct app_dev *dev)
{
    XGL_CHANNEL_FORMAT ch;
    XGL_NUM_FORMAT num;

    for (ch = 0; ch < XGL_MAX_CH_FMT; ch++) {
        for (num = 0; num < XGL_MAX_NUM_FMT; num++) {
            const XGL_FORMAT fmt = {
                .channelFormat = ch,
                .numericFormat = num,
            };
            XGL_RESULT err;
            XGL_SIZE size = sizeof(dev->format_props[ch][num]);

            err = xglGetFormatInfo(dev->obj, fmt,
                                   XGL_INFO_TYPE_FORMAT_PROPERTIES,
                                   &size, &dev->format_props[ch][num]);
            if (err) {
                memset(&dev->format_props[ch][num], 0,
                       sizeof(dev->format_props[ch][num]));
            }
            else if (size != sizeof(dev->format_props[ch][num])) {
                ERR_EXIT(XGL_ERROR_UNKNOWN);
            }
        }
    }
}

static void app_dev_init(struct app_dev *dev, struct app_gpu *gpu)
{
    XGL_DEVICE_CREATE_INFO info = {
        .sType = XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 0,
        .pRequestedQueues = NULL,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .maxValidationLevel = XGL_VALIDATION_LEVEL_END_RANGE,
        .flags = XGL_DEVICE_CREATE_VALIDATION_BIT,
    };
    XGL_RESULT err;
    XGL_SIZE size;
    XGL_UINT i;

    /* request all queues */
    info.queueRecordCount = gpu->queue_count;
    info.pRequestedQueues = gpu->queue_reqs;

    /* enable all extensions */
    info.extensionCount = gpu->extension_count;
    info.ppEnabledExtensionNames = gpu->extensions;
    dev->gpu = gpu;
    err = xglCreateDevice(gpu->obj, &info, &dev->obj);
    if (err)
        ERR_EXIT(err);

    err = xglGetMemoryHeapCount(dev->obj, &dev->heap_count);
    if (err)
        ERR_EXIT(err);

    dev->heap_props =
            malloc(sizeof(dev->heap_props[0]) * dev->heap_count);
    if (!dev->heap_props)
        ERR_EXIT(XGL_ERROR_OUT_OF_MEMORY);

    for (i = 0; i < dev->heap_count; i++) {
        size = sizeof(dev->heap_props[0]);
        err = xglGetMemoryHeapInfo(dev->obj, i,
                                   XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES,
                                   &size, &dev->heap_props[i]);
        if (err || size != sizeof(dev->heap_props[0]))
            ERR_EXIT(err);
    }
}

static void app_dev_destroy(struct app_dev *dev)
{
    free(dev->heap_props);
    xglDestroyDevice(dev->obj);
}

static void app_gpu_init_extensions(struct app_gpu *gpu)
{
    XGL_RESULT err;
    XGL_UINT i;

    static const XGL_CHAR *known_extensions[] = {
        "XGL_WSI_X11",
    };

    for (i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        err = xglGetExtensionSupport(gpu->obj, known_extensions[i]);
        if (!err)
            gpu->extension_count++;
    }

    gpu->extensions =
            malloc(sizeof(gpu->extensions[0]) * gpu->extension_count);
    if (!gpu->extensions)
        ERR_EXIT(XGL_ERROR_OUT_OF_MEMORY);

    gpu->extension_count = 0;
    for (i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        err = xglGetExtensionSupport(gpu->obj, known_extensions[i]);
        if (!err)
            gpu->extensions[gpu->extension_count++] = known_extensions[i];
    }
}

static void app_gpu_init(struct app_gpu *gpu, XGL_UINT id, XGL_PHYSICAL_GPU obj)
{
    XGL_SIZE size;
    XGL_RESULT err;
    int i;

    memset(gpu, 0, sizeof(*gpu));

    gpu->id = id;
    gpu->obj = obj;
    size = sizeof(gpu->props);
    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &size, &gpu->props);
    if (err || size != sizeof(gpu->props))
        ERR_EXIT(err);

    size = sizeof(gpu->perf);
    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE,
                        &size, &gpu->perf);
    if (err || size != sizeof(gpu->perf))
        ERR_EXIT(err);

    /* get queue count */
    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &size, NULL);
    if (err || size % sizeof(gpu->queue_props[0]))
        ERR_EXIT(err);
    gpu->queue_count = size / sizeof(gpu->queue_props[0]);

    gpu->queue_props =
            malloc(sizeof(gpu->queue_props[0]) * gpu->queue_count);
    size = sizeof(gpu->queue_props[0]) * gpu->queue_count;
    if (!gpu->queue_props)
        ERR_EXIT(XGL_ERROR_OUT_OF_MEMORY);
    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &size, gpu->queue_props);
    if (err || size != sizeof(gpu->queue_props[0]) * gpu->queue_count)
        ERR_EXIT(err);

    /* set up queue requests */
    size = sizeof(*gpu->queue_reqs) * gpu->queue_count;
    gpu->queue_reqs = malloc(sizeof(*gpu->queue_reqs) * gpu->queue_count);
    if (!gpu->queue_reqs)
        ERR_EXIT(XGL_ERROR_OUT_OF_MEMORY);
    for (i = 0; i < gpu->queue_count; i++) {
        gpu->queue_reqs[i].queueNodeIndex = i;
        gpu->queue_reqs[i].queueCount = gpu->queue_props[i].queueCount;
    }

    size = sizeof(gpu->memory_props);
    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES,
                        &size, &gpu->memory_props);
    if (err || size != sizeof(gpu->memory_props))
        ERR_EXIT(err);

    app_gpu_init_extensions(gpu);
    app_dev_init(&gpu->dev, gpu);
    app_dev_init_formats(&gpu->dev);
}

static void app_gpu_destroy(struct app_gpu *gpu)
{
    app_dev_destroy(&gpu->dev);
    free(gpu->extensions);
    free(gpu->queue_reqs);
    free(gpu->queue_props);
}

static void app_dev_dump_format_props(const struct app_dev *dev, XGL_CHANNEL_FORMAT ch, XGL_NUM_FORMAT num)
{
    const XGL_FORMAT_PROPERTIES *props = &dev->format_props[ch][num];
    struct {
        const char *name;
        XGL_FLAGS flags;
    } tilings[2];
    XGL_UINT i;

    if (!props->linearTilingFeatures && !props->optimalTilingFeatures)
        return;

    tilings[0].name = "linear";
    tilings[0].flags = props->linearTilingFeatures;
    tilings[1].name = "optimal";
    tilings[1].flags = props->optimalTilingFeatures;

    printf("FORMAT_%s_%s\n", xgl_channel_format_string(ch),
            xgl_numeric_format_string(num));
    for (i = 0; i < ARRAY_SIZE(tilings); i++) {
        if (!tilings[i].flags)
            continue;

        printf("\t%s tiling image =%s%s%s\n", tilings[i].name,
                (tilings[i].flags & XGL_FORMAT_IMAGE_SHADER_READ_BIT)      ? " read" : "",
                (tilings[i].flags & XGL_FORMAT_IMAGE_SHADER_WRITE_BIT)     ? " write" : "",
                (tilings[i].flags & XGL_FORMAT_IMAGE_COPY_BIT)             ? " copy" : "");
        printf("\t%s tiling memory =%s\n", tilings[i].name,
                (tilings[i].flags & XGL_FORMAT_MEMORY_SHADER_ACCESS_BIT)   ? " access" : "");
        printf("\t%s tiling attachment =%s%s%s%s%s\n", tilings[i].name,
                (tilings[i].flags & XGL_FORMAT_COLOR_ATTACHMENT_WRITE_BIT) ? " color" : "",
                (tilings[i].flags & XGL_FORMAT_COLOR_ATTACHMENT_BLEND_BIT) ? " blend" : "",
                (tilings[i].flags & XGL_FORMAT_DEPTH_ATTACHMENT_BIT)       ? " depth" : "",
                (tilings[i].flags & XGL_FORMAT_STENCIL_ATTACHMENT_BIT)     ? " stencil" : "",
                (tilings[i].flags & XGL_FORMAT_MSAA_ATTACHMENT_BIT)        ? " msaa" : "");
        printf("\t%s tiling conversion = %u\n", tilings[i].name,
                (bool) (tilings[i].flags & XGL_FORMAT_CONVERSION_BIT));
    }
}

static void app_dev_dump_heap_props(const struct app_dev *dev, XGL_UINT id)
{
    const XGL_MEMORY_HEAP_PROPERTIES *props = &dev->heap_props[id];

    printf("XGL_MEMORY_HEAP_PROPERTIES[%u]\n", id);
    printf("\tstructSize = %zu\n",      props->structSize);
    printf("\theapMemoryType = %s\n",   xgl_heap_type_string(props->heapMemoryType));
    printf("\theapSize = %zu\n",        props->heapSize);
    printf("\tpagesSize = %zu\n",       props->pageSize);

    printf("\tflags =%s%s%s%s%s%s\n",
            (props->flags & XGL_MEMORY_HEAP_CPU_VISIBLE_BIT)        ? " visible" : "",
            (props->flags & XGL_MEMORY_HEAP_CPU_GPU_COHERENT_BIT)   ? " coherent" : "",
            (props->flags & XGL_MEMORY_HEAP_CPU_UNCACHED_BIT)       ? " uc" : "",
            (props->flags & XGL_MEMORY_HEAP_CPU_WRITE_COMBINED_BIT) ? " wc" : "",
            (props->flags & XGL_MEMORY_HEAP_HOLDS_PINNED_BIT)       ? " pinnable" : "",
            (props->flags & XGL_MEMORY_HEAP_SHAREABLE_BIT)          ? " shareable" : "");

    printf("\tgpuReadPerfRating = %f\n",    props->gpuReadPerfRating);
    printf("\tgpuWritePerfRating = %f\n",   props->gpuWritePerfRating);
    printf("\tcpuReadPerfRating = %f\n",    props->cpuReadPerfRating);
    printf("\tcpuWritePerfRating = %f\n",   props->cpuWritePerfRating);
}

static void
app_dev_dump(const struct app_dev *dev)
{
    XGL_CHANNEL_FORMAT ch;
    XGL_NUM_FORMAT num;
    XGL_UINT i;

    for (i = 0; i < dev->heap_count; i++) {
        app_dev_dump_heap_props(dev, i);
        printf("\n");
    }

    for (ch = 0; ch < XGL_MAX_CH_FMT; ch++) {
        for (num = 0; num < XGL_MAX_NUM_FMT; num++)
            app_dev_dump_format_props(dev, ch, num);
    }
}

static void app_gpu_dump_multi_compat(const struct app_gpu *gpu, const struct app_gpu *other,
        const XGL_GPU_COMPATIBILITY_INFO *info)
{
    printf("XGL_GPU_COMPATIBILITY_INFO[GPU%d]\n", other->id);

#define TEST(info, b) printf(#b " = %u\n", (bool) (info->compatibilityFlags & XGL_GPU_COMPAT_ ##b## _BIT))
    TEST(info, ASIC_FEATURES);
    TEST(info, IQ_MATCH);
    TEST(info, PEER_TRANSFER);
    TEST(info, SHARED_MEMORY);
    TEST(info, SHARED_SYNC);
    TEST(info, SHARED_GPU0_DISPLAY);
    TEST(info, SHARED_GPU1_DISPLAY);
#undef TEST
}

static void app_gpu_multi_compat(struct app_gpu *gpus, XGL_UINT gpu_count)
{
        XGL_RESULT err;
        XGL_UINT i, j;

        for (i = 0; i < gpu_count; i++) {
                for (j = 0; j < gpu_count; j++) {
                        XGL_GPU_COMPATIBILITY_INFO info;

                        if (i == j)
                                continue;

                        err = xglGetMultiGpuCompatibility(gpus[i].obj,
                                        gpus[j].obj, &info);
                        if (err)
                                ERR_EXIT(err);

                        app_gpu_dump_multi_compat(&gpus[i], &gpus[j], &info);
                }
        }
}

static void app_gpu_dump_props(const struct app_gpu *gpu)
{
    const XGL_PHYSICAL_GPU_PROPERTIES *props = &gpu->props;

    printf("XGL_PHYSICAL_GPU_PROPERTIES\n");
    printf("\tstructSize = %zu\n",                  props->structSize);
    printf("\tapiVersion = %u\n",                   props->apiVersion);
    printf("\tdriverVersion = %u\n",                props->driverVersion);
    printf("\tvendorId = 0x%04x\n",                 props->vendorId);
    printf("\tdeviceId = 0x%04x\n",                 props->deviceId);
    printf("\tgpuType = %s\n",                      xgl_gpu_type_string(props->gpuType));
    printf("\tgpuName = %s\n",                      props->gpuName);
    printf("\tmaxMemRefsPerSubmission = %u\n",      props->maxMemRefsPerSubmission);
    printf("\tmaxInlineMemoryUpdateSize = %zu\n",   props->maxInlineMemoryUpdateSize);
    printf("\tmaxBoundDescriptorSets = %u\n",       props->maxBoundDescriptorSets);
    printf("\tmaxThreadGroupSize = %u\n",           props->maxThreadGroupSize);
    printf("\ttimestampFrequency = %lu\n",          props->timestampFrequency);
    printf("\tmultiColorAttachmentClears = %u\n",   props->multiColorAttachmentClears);
}

static void app_gpu_dump_perf(const struct app_gpu *gpu)
{
    const XGL_PHYSICAL_GPU_PERFORMANCE *perf = &gpu->perf;

    printf("XGL_PHYSICAL_GPU_PERFORMANCE\n");
    printf("\tmaxGpuClock = %f\n",      perf->maxGpuClock);
    printf("\taluPerClock = %f\n",      perf->aluPerClock);
    printf("\ttexPerClock = %f\n",      perf->texPerClock);
    printf("\tprimsPerClock = %f\n",    perf->primsPerClock);
    printf("\tpixelsPerClock = %f\n",   perf->pixelsPerClock);
}

static void app_gpu_dump_extensions(const struct app_gpu *gpu)
{
    int i;
    printf("Extensions");
    printf("\tcount = %d\n",            gpu->extension_count);
    printf("\t");
    for (i=0; i< gpu->extension_count; i++) {
        if (i>0)
            printf(", "); // separator between extension names
        printf("%s",                    gpu->extensions[i]);
    }
    printf("\n");
}

static void app_gpu_dump_queue_props(const struct app_gpu *gpu, XGL_UINT id)
{
    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *props = &gpu->queue_props[id];

    printf("XGL_PHYSICAL_GPU_QUEUE_PROPERTIES[%d]\n", id);
    printf("\tstructSize = %zu\n",          props->structSize);
    printf("\tqueueFlags = %c%c%c%c\n",
            (props->queueFlags & XGL_QUEUE_GRAPHICS_BIT) ? 'G' : '.',
            (props->queueFlags & XGL_QUEUE_COMPUTE_BIT)  ? 'C' : '.',
            (props->queueFlags & XGL_QUEUE_DMA_BIT)      ? 'D' : '.',
            (props->queueFlags & XGL_QUEUE_EXTENDED_BIT) ? 'X' : '.');
    printf("\tqueueCount = %u\n",           props->queueCount);
    printf("\tmaxAtomicCounters = %u\n",    props->maxAtomicCounters);
    printf("\tsupportsTimestamps = %u\n",   props->supportsTimestamps);
}

static void app_gpu_dump_memory_props(const struct app_gpu *gpu)
{
    const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES *props = &gpu->memory_props;

    printf("XGL_PHYSICAL_GPU_MEMORY_PROPERTIES\n");
    printf("\tstructSize = %zu\n",                      props->structSize);
    printf("\tsupportsMigration = %u\n",                props->supportsMigration);
    printf("\tsupportsPinning = %u\n",                  props->supportsPinning);
}

static void app_gpu_dump(const struct app_gpu *gpu)
{
    XGL_UINT i;

    printf("GPU%u\n", gpu->id);
    app_gpu_dump_props(gpu);
    printf("\n");
    app_gpu_dump_extensions(gpu);
    printf("\n");
    app_gpu_dump_perf(gpu);
    printf("\n");
    for (i = 0; i < gpu->queue_count; i++) {
        app_gpu_dump_queue_props(gpu, i);
        printf("\n");
    }
    app_gpu_dump_memory_props(gpu);
    printf("\n");
    app_dev_dump(&gpu->dev);
}

int main(int argc, char **argv)
{
    static const XGL_APPLICATION_INFO app_info = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = "xglinfo",
        .appVersion = 1,
        .pEngineName = "xglinfo",
        .engineVersion = 1,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
    struct app_gpu gpus[MAX_GPUS];
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count, i;
    XGL_RESULT err;

    err = xglInitAndEnumerateGpus(&app_info, NULL,
            MAX_GPUS, &gpu_count, objs);
    if (err)
        ERR_EXIT(err);

    for (i = 0; i < gpu_count; i++) {
        app_gpu_init(&gpus[i], i, objs[i]);
        app_gpu_dump(&gpus[i]);
        printf("\n\n");
    }

    app_gpu_multi_compat(gpus, gpu_count);

    for (i = 0; i < gpu_count; i++)
        app_gpu_destroy(&gpus[i]);

    xglInitAndEnumerateGpus(&app_info, NULL, 0, &gpu_count, NULL);

    return 0;
}
