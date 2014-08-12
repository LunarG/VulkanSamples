#include "xglgpu.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

XglGpu::XglGpu(XGL_UINT id, XGL_PHYSICAL_GPU obj)
{
    this->id = id;
    this->gpuObj = obj;
    this->extension_count = 0;
    this->heap_count = 0;
    this->queue_count = 0;

    this->init_gpu();
    this->init_extensions();
    this->init_device();
    this->init_formats();
}

void XglGpu::init_gpu()
{
    int i;
    XGL_RESULT err;
    XGL_SIZE size;

    err = xglGetGpuInfo(this->gpuObj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &size, &this->props);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(size, sizeof(this->props));

    err = xglGetGpuInfo(this->gpuObj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE,
                        &size, &this->perf);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(size, sizeof(this->perf));

    /* get queue count */
    err = xglGetGpuInfo(this->gpuObj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &size, NULL);
    ASSERT_XGL_SUCCESS(err);
    this->queue_count = size / sizeof(this->queue_props[0]);
    ASSERT_EQ(this->queue_count*sizeof(this->queue_props[0]), size) << "invalid GPU_QUEUE_PROPERTIES size";

    this->queue_props = new XGL_PHYSICAL_GPU_QUEUE_PROPERTIES [this->queue_count];
    ASSERT_TRUE(NULL != this->queue_props) << "Out of memory";

    err = xglGetGpuInfo(this->gpuObj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &size, this->queue_props);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(this->queue_count*sizeof(this->queue_props[0]), size) << "invalid GPU_QUEUE_PROPERTIES size";

    /* set up queue requests */
    // this->queue_reqs = malloc(sizeof(*this->queue_reqs) * this->queue_count);
    this->queue_reqs = new XGL_DEVICE_QUEUE_CREATE_INFO [this->queue_count];
    ASSERT_TRUE(NULL != this->queue_reqs) << "Out of memory";

    for (i = 0; i < this->queue_count; i++) {
        this->queue_reqs[i].queueNodeIndex = i;
        this->queue_reqs[i].queueCount = this->queue_props[i].queueCount;
    }

    err = xglGetGpuInfo(this->gpuObj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES,
                        &size, &this->memory_props);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(size, sizeof(this->memory_props));
}

void XglGpu::init_extensions()
{
    XGL_RESULT err;
    XGL_UINT i;

    static const XGL_CHAR *known_extensions[] = {
        (const XGL_CHAR *) "some_extension",
    };
    this->extension_count = 0;

    for (i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        err = xglGetExtensionSupport(this->gpuObj, known_extensions[i]);
        if (!err)
            this->extension_count++;
    }

    if (this->extension_count == 0) {
        return;
    }

    this->extensions = new const XGL_CHAR *[this->extension_count];

    ASSERT_TRUE(NULL != this->extensions) << "Out of memory";

    this->extension_count = 0;
    for (i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        err = xglGetExtensionSupport(this->gpuObj, known_extensions[i]);
        if (!err)
            this->extensions[this->extension_count++] = known_extensions[i];
    }
}

void XglGpu::init_device()
{
    XGL_DEVICE_CREATE_INFO info = {};
    XGL_RESULT err;
    XGL_SIZE size;
    XGL_UINT i;

    info.sType = XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    info.maxValidationLevel = XGL_VALIDATION_LEVEL_END_RANGE;
    info.flags = XGL_DEVICE_CREATE_VALIDATION_BIT;

    /* request all queues */
    info.queueRecordCount = this->queue_count;
    info.pRequestedQueues = this->queue_reqs;

    /* enable all extensions */
    info.extensionCount = this->extension_count;
    info.ppEnabledExtensionNames = this->extensions;

    err = xglCreateDevice(this->gpuObj, &info, &this->devObj);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetMemoryHeapCount(this->devObj, &this->heap_count);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(1, this->heap_count) << "No memory heaps available";

    this->heap_props = new XGL_MEMORY_HEAP_PROPERTIES [this->heap_count];
    ASSERT_TRUE(NULL != this->heap_props) << "Out of memory";

    for (i = 0; i < this->heap_count; i++) {
        err = xglGetMemoryHeapInfo(this->devObj, i,
                                   XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES,
                                   &size, &this->heap_props[i]);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_EQ(size, sizeof(this->heap_props[0])) << "Invalid heap property size";
    }
}

void XglGpu::init_formats()
{
    XGL_CHANNEL_FORMAT ch;
    XGL_NUM_FORMAT num;

    for (int chInt = XGL_CH_FMT_UNDEFINED; chInt < XGL_MAX_CH_FMT; chInt++) {
        for (int numInt = 0; numInt < XGL_MAX_NUM_FMT; numInt++) {
            XGL_FORMAT fmt = {};
            XGL_RESULT err;
            XGL_SIZE size;

            fmt.channelFormat = static_cast<XGL_CHANNEL_FORMAT>(chInt);
            fmt.numericFormat = static_cast<XGL_NUM_FORMAT>(numInt);

            err = xglGetFormatInfo(this->devObj, fmt,
                                   XGL_INFO_TYPE_FORMAT_PROPERTIES,
                                   &size, &this->format_props[ch][num]);
            if (err) {
                memset(&this->format_props[ch][num], 0,
                       sizeof(this->format_props[ch][num]));
            }
            else if (size != sizeof(this->format_props[ch][num])) {
                ASSERT_EQ(size, sizeof(this->format_props[ch][num])) << "Incorrect data size";
            }
        }
    }
}

