#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <xgl.h>
#include "common.h"

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
#undef STR
    default: return "UNKNOWN_NUM";
    }
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
    printf("\tstructSize = %u\n",       props->structSize);
    printf("\theapMemoryType = %s\n",   xgl_heap_type_string(props->heapMemoryType));
    printf("\theapSize = %u\n",         props->heapSize);
    printf("\tpagesSize = %u\n",        props->pageSize);

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
    printf("\tstructSize = %u\n",                   props->structSize);
    printf("\tapiVersion = %u\n",                   props->apiVersion);
    printf("\tdriverVersion = %u\n",                props->driverVersion);
    printf("\tvendorId = 0x%04x\n",                 props->vendorId);
    printf("\tdeviceId = 0x%04x\n",                 props->deviceId);
    printf("\tgpuType = %s\n",                      xgl_gpu_type_string(props->gpuType));
    printf("\tgpuName = %s\n",                      props->gpuName);
    printf("\tmaxMemRefsPerSubmission = %u\n",      props->maxMemRefsPerSubmission);
    printf("\tvirtualMemPageSize = %u\n",           props->virtualMemPageSize);
    printf("\tmaxInlineMemoryUpdateSize = %u\n",    props->maxInlineMemoryUpdateSize);
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

static void app_gpu_dump_queue_props(const struct app_gpu *gpu, XGL_UINT id)
{
    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *props = &gpu->queue_props[id];

    printf("XGL_PHYSICAL_GPU_QUEUE_PROPERTIES[%d]\n", id);
    printf("\tstructSize = %u\n",           props->structSize);
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
    printf("\tstructSize = %u\n",                       props->structSize);
    printf("\tsupportsMigration = %u\n",                props->supportsMigration);
    printf("\tsupportsVirtualMemoryRemapping = %u\n",   props->supportsVirtualMemoryRemapping);
    printf("\tsupportsPinning = %u\n",                  props->supportsPinning);
}

static void app_gpu_dump(const struct app_gpu *gpu)
{
    XGL_UINT i;

    printf("GPU%u\n", gpu->id);
    app_gpu_dump_props(gpu);
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
        .pAppName = (const XGL_CHAR *) "xglinfo",
        .appVersion = 1,
        .pEngineName = (const XGL_CHAR *) "xglinfo",
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
