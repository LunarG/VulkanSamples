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

#include "xglgpu.h"

/**
*******************************************************************************
* @brief xglImage class wraps an XGL Image object
*
* Use this class to allocate and manage XGL image objects and associated
* bound memory and views.
*
********************************************************************************
*/
class XglImage
{
public:
    XglImage();

    virtual ~XglImage();

    // Image usage flags
    //    typedef enum _XGL_IMAGE_USAGE_FLAGS
    //    {
    //        XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT                  = 0x00000001,
    //        XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT                 = 0x00000002,
    //        XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                    = 0x00000004,
    //        XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT                       = 0x00000008,
    //    } XGL_IMAGE_USAGE_FLAGS;

    XGL_RESULT init( XGL_UINT32 heap, XGL_UINT32 w, XGL_UINT32 h,
                     XGL_FORMAT fmt, XGL_FLAGS usage, XGL_UINT32 mipCount,
                     bool cpuAccess = false );

    //    void clear( CommandBuffer*, XGL_UINT[4] );
    //    void prepare( CommandBuffer*, XGL_IMAGE_STATE );

    void state( XGL_IMAGE_STATE state )
    {
        m_imageInfo.state = state;
    }

    XGL_IMAGE image() const
    {
        return m_image;
    }
    XGL_COLOR_TARGET_VIEW targetView()const
    {
        return m_targetView;
    }
    XGL_IMAGE_VIEW_ATTACH_INFO imageView()const
    {
        return m_imageInfo;
    }
    const XGL_GPU_MEMORY& memory() const
    {
        return m_memory;
    }
    const XGL_GPU_SIZE& offset() const
    {
        return m_offset;
    }

    XGL_IMAGE_STATE state() const
    {
        return ( XGL_IMAGE_STATE )m_imageInfo.state;
    }
    XGL_FORMAT format() const
    {
        return m_format;
    }
    XGL_UINT32 width() const
    {
        return m_width;
    }
    XGL_UINT32 height() const
    {
        return m_height;
    }
    XGL_UINT32 mipCount() const
    {
        return m_mipCount;
    }
    XGL_CHANNEL_MAPPING channelMapping() const
    {
        return m_channelMapping;
    }

    bool IsBCCompressed( XGL_FORMAT format )
    {
        return format.channelFormat >= XGL_CH_FMT_BC1 && format.channelFormat <= XGL_CH_FMT_BC7;
    }

protected:
    XglGpu *m_pgpu;

    XGL_IMAGE                    m_image;
    XGL_COLOR_TARGET_VIEW        m_targetView;
    XGL_IMAGE_VIEW_ATTACH_INFO   m_imageInfo;

    XGL_GPU_MEMORY               m_memory;
    XGL_GPU_SIZE                 m_offset;
    XGL_FORMAT                   m_format;

    XGL_CHANNEL_MAPPING          m_channelMapping;
    XGL_UINT32                   m_width;
    XGL_UINT32                   m_height;
    XGL_UINT32                   m_mipCount;
}

#endif // XGLIMAGE_H
