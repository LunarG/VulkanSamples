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

XglImage::XglImage(XglDevice *dev) :
    m_image( XGL_NULL_HANDLE ),
    m_targetView( XGL_NULL_HANDLE ),
    m_memory( XGL_NULL_HANDLE ),
    m_offset( 0 ),
    m_mipCount( 0 ),
    m_width( 0 ),
    m_height( 0 )
{
    m_device = dev;
    m_format.channelFormat = XGL_CH_FMT_UNDEFINED;
    m_format.numericFormat = XGL_NUM_FMT_UNDEFINED;
    m_imageInfo.view = XGL_NULL_HANDLE;
    m_imageInfo.state = XGL_IMAGE_STATE_UNINITIALIZED_TARGET;
}

XglImage::~XglImage()
{
    if (m_memory != XGL_NULL_HANDLE) xglFreeMemory(m_memory);
    if (m_image != XGL_NULL_HANDLE) xglDestroyObject(m_image);
    if (m_targetView != XGL_NULL_HANDLE) xglDestroyObject(m_targetView);
    if (m_imageInfo.view != XGL_NULL_HANDLE) xglDestroyObject(m_imageInfo.view);
}

void XglImage::init(XGL_UINT32 w, XGL_UINT32 h,
               XGL_FORMAT fmt, XGL_FLAGS usage)
{
    XGL_RESULT err;
    XGL_UINT mipCount;
    XGL_SIZE size;
    XGL_FORMAT_PROPERTIES image_fmt;

    mipCount = 0;

    m_width = w;
    m_height = h;
    m_format = fmt;
//    m_screen.Init(true, w, h);

    XGL_UINT _w = w;
    XGL_UINT _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

//    fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
//    fmt.numericFormat = XGL_NUM_FMT_UNORM;
    // TODO: Pick known good format rather than just expect common format
    /*
     * XXX: What should happen if given NULL HANDLE for the pData argument?
     * We're not requesting XGL_INFO_TYPE_MEMORY_REQUIREMENTS so there is
     * an expectation that pData is a valid pointer.
     * However, why include a returned size value? That implies that the
     * amount of data may vary and that doesn't work well for using a
     * fixed structure.
     */

    err = xglGetFormatInfo(this->m_device->device(), fmt,
                           XGL_INFO_TYPE_FORMAT_PROPERTIES,
                           &size, &image_fmt);
    ASSERT_XGL_SUCCESS(err);

    //    typedef struct _XGL_IMAGE_CREATE_INFO
    //    {
    //        XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO
    //        const XGL_VOID*                         pNext;                      // Pointer to next structure.
    //        XGL_IMAGE_TYPE                          imageType;
    //        XGL_FORMAT                              format;
    //        XGL_EXTENT3D                            extent;
    //        XGL_UINT                                mipLevels;
    //        XGL_UINT                                arraySize;
    //        XGL_UINT                                samples;
    //        XGL_IMAGE_TILING                        tiling;
    //        XGL_FLAGS                               usage;                      // XGL_IMAGE_USAGE_FLAGS
    //        XGL_FLAGS                               flags;                      // XGL_IMAGE_CREATE_FLAGS
    //    } XGL_IMAGE_CREATE_INFO;


    XGL_IMAGE_CREATE_INFO imageCreateInfo = {};
    imageCreateInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = XGL_IMAGE_2D;
    imageCreateInfo.format = fmt;
    imageCreateInfo.arraySize = 1;
    imageCreateInfo.extent.width = w;
    imageCreateInfo.extent.height = h;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipCount;
    imageCreateInfo.samples = 1;
    imageCreateInfo.tiling = XGL_LINEAR_TILING;

    // Image usage flags
    //    typedef enum _XGL_IMAGE_USAGE_FLAGS
    //    {
    //        XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT                  = 0x00000001,
    //        XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT                 = 0x00000002,
    //        XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                    = 0x00000004,
    //        XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT                       = 0x00000008,
    //    } XGL_IMAGE_USAGE_FLAGS;
//    imageCreateInfo.usage = XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT | XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageCreateInfo.usage = usage;

    //    XGL_RESULT XGLAPI xglCreateImage(
    //        XGL_DEVICE                                  device,
    //        const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    //        XGL_IMAGE*                                  pImage);
    err = xglCreateImage(device(), &imageCreateInfo, &m_image);
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_UINT data_size;
    err = xglGetObjectInfo(m_image, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(data_size, sizeof(mem_req));
    ASSERT_NE(0, mem_req.size) << "xglGetObjectInfo (Event): Failed - expect images to require memory";

    m_imageInfo.state = XGL_IMAGE_STATE_UNINITIALIZED_TARGET;

    //        XGL_RESULT XGLAPI xglAllocMemory(
    //            XGL_DEVICE                                  device,
    //            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    //            XGL_GPU_MEMORY*                             pMem);
    XGL_MEMORY_ALLOC_INFO mem_info;

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = mem_req.size;
    mem_info.alignment = mem_req.alignment;
    mem_info.heapCount = mem_req.heapCount;
    memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    mem_info.flags = XGL_MEMORY_ALLOC_SHAREABLE_BIT;
    err = xglAllocMemory(device(), &mem_info, &m_memory);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(m_image, m_memory, 0);
    ASSERT_XGL_SUCCESS(err);

    XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO createView = {
        XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
        XGL_NULL_HANDLE,
        m_image,
        {XGL_CH_FMT_R8G8B8A8, XGL_NUM_FMT_UNORM},
        0,
        0,
        1
    };

    err = xglCreateColorAttachmentView(device(), &createView, &m_targetView);
    ASSERT_XGL_SUCCESS(err);
}

void XglImage::WritePPM( const char *basename )
{
    string filename;
    XGL_RESULT err;
    int x, y;

    filename.append(basename);
    filename.append(".ppm");

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    XGL_UINT data_size;

    err = xglGetImageSubresourceInfo( m_image, &sr, XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    const char *ptr;

    err = xglMapMemory( m_memory, 0, (XGL_VOID **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    ofstream file (filename.c_str());
    ASSERT_TRUE(file.is_open()) << "Unable to open file: " << filename;

    file << "P6\n";
    file << m_width << "\n";
    file << m_height << "\n";
    file << 255 << "\n";

    for (y = 0; y < m_height; y++) {
        const char *row = ptr;

        for (x = 0; x < m_width; x++) {
            file.write(row, 3);
            row += 4;
        }

        ptr += sr_layout.rowPitch;
    }

    file.close();

    err = xglUnmapMemory( m_memory );
    ASSERT_XGL_SUCCESS( err );
}

