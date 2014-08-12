#ifndef XGLGPU_H
#define XGLGPU_H

#include "test_common.h"

#include <xgl.h>

#define MAX_GPUS 8

#define MAX_QUEUE_TYPES 5

class XglGpu
{
public:
    XglGpu(XGL_UINT id, XGL_PHYSICAL_GPU gpuObj);
    void init_gpu();
    void init_extensions();
    void init_device();
    void init_formats();

private:
    XGL_UINT id;
    XGL_PHYSICAL_GPU gpuObj;

    XGL_PHYSICAL_GPU_PROPERTIES props;
    XGL_PHYSICAL_GPU_PERFORMANCE perf;

    XGL_UINT queue_count;
    XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *queue_props;
    XGL_DEVICE_QUEUE_CREATE_INFO *queue_reqs;

    XGL_PHYSICAL_GPU_MEMORY_PROPERTIES memory_props;

    XGL_UINT extension_count;
    const XGL_CHAR **extensions;

    // Device info
    // struct app_dev dev;
    XGL_DEVICE devObj;

    XGL_UINT heap_count;
    XGL_MEMORY_HEAP_PROPERTIES *heap_props;
    XGL_QUEUE queues[MAX_QUEUE_TYPES];

    XGL_FORMAT_PROPERTIES format_props[XGL_MAX_CH_FMT][XGL_MAX_NUM_FMT];

};

#endif // XGLGPU_H
