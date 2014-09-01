#ifndef XGLDEVICE_H
#define XGLDEVICE_H

#include "xglgpu.h"
class XglImage;

class XglDevice : public XglGpu
{
public:
    XglDevice(XGL_UINT id, XGL_PHYSICAL_GPU obj);

    XGL_DEVICE device() {return this->m_xgl_device_object;}
    void init_formats();
    void init_device();
    void get_device_queue(XGL_QUEUE_TYPE queue_type,
                          XGL_UINT queue_idx);
    void get_device_queue() {get_device_queue(XGL_QUEUE_TYPE_GRAPHICS, 0);}
    XGL_RESULT AllocAndBindGpuMemory(XGL_OBJECT obj, const std::string &objName, XGL_GPU_MEMORY *pMem);

    void CreateImage(XGL_UINT32 w, XGL_UINT32 h,
                     XGL_FORMAT fmt, XGL_FLAGS usage,
                     XglImage **pImage);
private:
    XGL_DEVICE m_xgl_device_object;
    uint32_t m_flags;
};

#endif // XGLDEVICE_H
