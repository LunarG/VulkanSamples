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
}

void XglGpu::init_gpu()
{
    int i;
    XGL_RESULT err;
    XGL_SIZE size;

    size = sizeof(this->props);
    err = xglGetGpuInfo(this->gpuObj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &size, &this->props);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(size, sizeof(this->props));

    size = sizeof(this->perf);
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

    size = this->queue_count*sizeof(this->queue_props[0]);
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

    size = sizeof(this->memory_props);
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
        "XGL_COMPILE_GLSL",
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

bool XglGpu::extension_exist(const char *ext_name)
{
    XGL_UINT i;

    for (i=0; i<this->extension_count; i++) {
        if (strcmp(this->extensions[i], ext_name) == 0)
            return true;
    }

    return false;
}

