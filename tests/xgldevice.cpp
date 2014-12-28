

#include "xgldevice.h"
#include "xglimage.h"

XglDevice::XglDevice(XGL_UINT id, XGL_PHYSICAL_GPU obj) :
    xgl_testing::Device(obj), id(id)
{
    init();

    props = gpu().properties();
    queue_props = &gpu().queue_properties()[0];
}

void XglDevice::get_device_queue()
{
    ASSERT_NE(true, graphics_queues().empty());
    m_queue = graphics_queues()[0]->obj();
}

XGL_RESULT XglDevice::AllocAndBindGpuMemory(XGL_OBJECT obj, const std::string &objName, XGL_GPU_MEMORY *pMem)
{
    XGL_RESULT err;
    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_SIZE data_size = sizeof(mem_req);
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
