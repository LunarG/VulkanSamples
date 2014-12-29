#ifndef XGLDEVICE_H
#define XGLDEVICE_H

#include "test_common.h"

class XglImage;

class XglDevice : public xgl_testing::Device
{
public:
    XglDevice(XGL_UINT id, XGL_PHYSICAL_GPU obj);

    XGL_DEVICE device() { return obj(); }
    void get_device_queue();

    void CreateImage(XGL_UINT32 w, XGL_UINT32 h,
                     XGL_FORMAT fmt, XGL_FLAGS usage,
                     XglImage **pImage);

    XGL_UINT id;
    XGL_PHYSICAL_GPU_PROPERTIES props;
    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *queue_props;

    XGL_QUEUE m_queue;
};

#endif // XGLDEVICE_H
