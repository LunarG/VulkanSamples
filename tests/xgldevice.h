#ifndef XGLDEVICE_H
#define XGLDEVICE_H

#include "xglgpu.h"

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

private:
    XGL_DEVICE m_xgl_device_object;
    uint32_t m_flags;
};

#endif // XGLDEVICE_H
