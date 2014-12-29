

#include "xglrenderframework.h"
#include "xgldevice.h"

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

void XglDevice::CreateImage(XGL_UINT32 w, XGL_UINT32 h,
                 XGL_FORMAT fmt, XGL_FLAGS usage,
                 XglImage **pImage)
{
    XglImage *new_image;

    new_image = new XglImage(this);
    new_image->init(w, h, fmt, usage);

    *pImage = new_image;
}
