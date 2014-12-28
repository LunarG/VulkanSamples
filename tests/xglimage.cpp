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

#include "xglimage.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

XglImage::XglImage(XglDevice *dev)
{
    m_device = dev;
    m_imageInfo.view = XGL_NULL_HANDLE;
    m_imageInfo.state = XGL_IMAGE_STATE_UNINITIALIZED_TARGET;
}

void XglImage::init(XGL_UINT32 w, XGL_UINT32 h,
               XGL_FORMAT fmt, XGL_FLAGS usage,
               XGL_IMAGE_TILING tiling)
{
    XGL_UINT mipCount;

    mipCount = 0;

    XGL_UINT _w = w;
    XGL_UINT _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

    XGL_IMAGE_CREATE_INFO imageCreateInfo = xgl_testing::Image::create_info();
    imageCreateInfo.imageType = XGL_IMAGE_2D;
    imageCreateInfo.format = fmt;
    imageCreateInfo.extent.width = w;
    imageCreateInfo.extent.height = h;
    imageCreateInfo.mipLevels = mipCount;
    imageCreateInfo.tiling = tiling;

    imageCreateInfo.usage = usage;

    xgl_testing::Image::init(*m_device, imageCreateInfo);

    m_imageInfo.state = XGL_IMAGE_STATE_UNINITIALIZED_TARGET;

    XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO createView = {
        XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
        XGL_NULL_HANDLE,
        obj(),
        {XGL_CH_FMT_R8G8B8A8, XGL_NUM_FMT_UNORM},
        0,
        0,
        1
    };

    m_targetView.init(*m_device, createView);
}

XGL_RESULT XglImage::MapMemory(XGL_VOID** ptr)
{
    *ptr = map();
    return (*ptr) ? XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

XGL_RESULT XglImage::UnmapMemory()
{
    unmap();
    return XGL_SUCCESS;
}
