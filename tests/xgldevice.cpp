

#include "xgldevice.h"
#include "xglimage.h"

XglDevice::XglDevice(XGL_UINT id, XGL_PHYSICAL_GPU obj) :
    m_flags(0),
    XglGpu(id, obj)
{
    init_device();
    init_formats();
}


void XglDevice::init_device()
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

    err = xglCreateDevice(this->gpuObj, &info, &m_xgl_device_object);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetMemoryHeapCount(m_xgl_device_object, &this->heap_count);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(1, this->heap_count) << "No memory heaps available";

    this->heap_props = new XGL_MEMORY_HEAP_PROPERTIES [this->heap_count];
    ASSERT_TRUE(NULL != this->heap_props) << "Out of memory";

    for (i = 0; i < this->heap_count; i++) {
        err = xglGetMemoryHeapInfo(m_xgl_device_object, i,
                                   XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES,
                                   &size, &this->heap_props[i]);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_EQ(size, sizeof(this->heap_props[0])) << "Invalid heap property size";
    }
}

void XglDevice::init_formats()
{
    for (int chInt = XGL_CH_FMT_UNDEFINED; chInt < XGL_MAX_CH_FMT; chInt++) {
        for (int numInt = 0; numInt < XGL_MAX_NUM_FMT; numInt++) {
            XGL_FORMAT fmt = {};
            XGL_RESULT err;
            XGL_SIZE size;

            fmt.channelFormat = static_cast<XGL_CHANNEL_FORMAT>(chInt);
            fmt.numericFormat = static_cast<XGL_NUM_FORMAT>(numInt);

            err = xglGetFormatInfo(m_xgl_device_object, fmt,
                                   XGL_INFO_TYPE_FORMAT_PROPERTIES,
                                   &size, &this->format_props[chInt][numInt]);
            if (err) {
                memset(&this->format_props[chInt][numInt], 0,
                       sizeof(this->format_props[chInt][numInt]));
            }
            else if (size != sizeof(this->format_props[chInt][numInt])) {
                ASSERT_EQ(size, sizeof(this->format_props[chInt][numInt])) << "Incorrect data size";
            }
        }
    }
}


void XglDevice::get_device_queue(XGL_QUEUE_TYPE queue_type, XGL_UINT queue_idx)
{
    XGL_RESULT err;

    err = xglGetDeviceQueue(this->device(), queue_type, queue_idx, &this->m_queue);
    ASSERT_XGL_SUCCESS(err) << "xglGetDeviceQueue failed";
}

XGL_RESULT XglDevice::AllocAndBindGpuMemory(XGL_OBJECT obj, const std::string &objName, XGL_GPU_MEMORY *pMem)
{
    XGL_RESULT err;
    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_UINT data_size;
    err = xglGetObjectInfo(obj, XGL_INFO_TYPE_MEMORY_REQUIREMENTS, &data_size, &mem_req);
    if (err != XGL_SUCCESS) return err;

    if (mem_req.size > 0) {
        XGL_MEMORY_ALLOC_INFO mem_info = {
            XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
            XGL_NULL_HANDLE,
            mem_req.size,                                   // allocationSize
            mem_req.alignment,                              // alignment
            XGL_MEMORY_ALLOC_SHAREABLE_BIT,                 // XGL_MEMORY_ALLOC_FLAGS
            mem_req.heapCount,                              // heapCount
            {0},                                            // heaps
            XGL_MEMORY_PRIORITY_NORMAL                      // XGL_MEMORY_PRIORITY
        };
        memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);

        err = xglAllocMemory(device(), &mem_info, pMem);
        if (err != XGL_SUCCESS) return err;

        err = xglBindObjectMemory(obj, *pMem, 0);
        if (err != XGL_SUCCESS) return err;
    }

    return err;
}

void XglDevice::CreateImage(XGL_UINT32 w, XGL_UINT32 h,
                 XGL_FORMAT fmt, XGL_FLAGS usage,
                 XglImage **pImage)
{
    XglImage *new_image;

    new_image = new XglImage(this);
    new_image->init(w, h, fmt, usage);

    *pImage = new_image;
}
