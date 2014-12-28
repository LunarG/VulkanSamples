//  XGL tests
//
//  Copyright (C) 2014 LunarG, Inc.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#ifndef XGLIMAGE_H
#define XGLIMAGE_H

#include "xgldevice.h"

/**
*******************************************************************************
* @brief xglImage class wraps an XGL Image object
*
* Use this class to allocate and manage XGL image objects and associated
* bound memory and views.
*
********************************************************************************
*/
class XglImage : public xgl_testing::Image
{
public:
    XglImage(XglDevice *dev);

    // Image usage flags
    //    typedef enum _XGL_IMAGE_USAGE_FLAGS
    //    {
    //        XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT                  = 0x00000001,
    //        XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT                 = 0x00000002,
    //        XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                    = 0x00000004,
    //        XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT                       = 0x00000008,
    //    } XGL_IMAGE_USAGE_FLAGS;
public:
    void init( XGL_UINT32 w, XGL_UINT32 h,
                     XGL_FORMAT fmt, XGL_FLAGS usage,
                     XGL_IMAGE_TILING tiling=XGL_LINEAR_TILING);

    //    void clear( CommandBuffer*, XGL_UINT[4] );
    //    void prepare( CommandBuffer*, XGL_IMAGE_STATE );

    void state( XGL_IMAGE_STATE state )
    {
        m_imageInfo.state = state;
    }
    XGL_GPU_MEMORY memory() const
    {
        const std::vector<XGL_GPU_MEMORY> mems = memories();
        return mems.empty() ? XGL_NULL_HANDLE : mems[0];
    }


    XGL_IMAGE image() const
    {
        return obj();
    }
    XGL_COLOR_ATTACHMENT_VIEW targetView()const
    {
        return m_targetView.obj();
    }

    XGL_IMAGE_STATE state() const
    {
        return ( XGL_IMAGE_STATE )m_imageInfo.state;
    }
    XGL_UINT32 width() const
    {
        return extent().width;
    }
    XGL_UINT32 height() const
    {
        return extent().height;
    }

    XGL_RESULT MapMemory(XGL_VOID** ptr);
    XGL_RESULT UnmapMemory();

protected:
    XglDevice *m_device;

    xgl_testing::ColorAttachmentView m_targetView;
    XGL_IMAGE_VIEW_ATTACH_INFO   m_imageInfo;
};

#endif // XGLIMAGE_H
