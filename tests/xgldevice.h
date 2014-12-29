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

    XGL_UINT id;
    XGL_PHYSICAL_GPU_PROPERTIES props;
    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *queue_props;

    XGL_QUEUE m_queue;
};

#endif // XGLDEVICE_H
