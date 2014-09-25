// Copyright 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


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


// Verify XGL driver initialization

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <xgl.h>
#include "gtest-1.7.0/include/gtest/gtest.h"

#include "xgldevice.h"

class XglImageTest : public ::testing::Test {
public:
    void CreateImage(XGL_UINT w, XGL_UINT h);
    void DestroyImage();

    void CreateImageView(XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
                         XGL_IMAGE_VIEW* pView);
    void DestroyImageView(XGL_IMAGE_VIEW imageView);
    XGL_DEVICE device() {return m_device->device();}

protected:
    XglDevice *m_device;
    XGL_APPLICATION_INFO app_info;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count;
    XGL_IMAGE m_image;
    XGL_GPU_MEMORY m_image_mem;

    virtual void SetUp() {
        XGL_RESULT err;

        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = (const XGL_CHAR *) "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = (const XGL_CHAR *) "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

        err = xglInitAndEnumerateGpus(&app_info, NULL,
                                      MAX_GPUS, &this->gpu_count, objs);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_GE(1, this->gpu_count) << "No GPU available";

        this->m_device = new XglDevice(0, objs[0]);
    }

    virtual void TearDown() {
        xglInitAndEnumerateGpus(&this->app_info, NULL, 0, &gpu_count, NULL);
    }
};


void XglImageTest::CreateImage(XGL_UINT w, XGL_UINT h)
{
    XGL_RESULT err;
    XGL_UINT mipCount;
    XGL_SIZE size;
    XGL_FORMAT fmt;
    XGL_FORMAT_PROPERTIES image_fmt;

    mipCount = 0;

    XGL_UINT _w = w;
    XGL_UINT _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

    fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    fmt.numericFormat = XGL_NUM_FMT_UINT;
    // TODO: Pick known good format rather than just expect common format
    /*
     * XXX: What should happen if given NULL HANDLE for the pData argument?
     * We're not requesting XGL_INFO_TYPE_MEMORY_REQUIREMENTS so there is
     * an expectation that pData is a valid pointer.
     * However, why include a returned size value? That implies that the
     * amount of data may vary and that doesn't work well for using a
     * fixed structure.
     */
    size = sizeof(image_fmt);
    err = xglGetFormatInfo(this->device(), fmt,
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
    imageCreateInfo.usage = XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT | XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //    XGL_RESULT XGLAPI xglCreateImage(
    //        XGL_DEVICE                                  device,
    //        const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    //        XGL_IMAGE*                                  pImage);
    err = xglCreateImage(device(), &imageCreateInfo, &m_image);
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_UINT data_size = sizeof(mem_req);
    err = xglGetObjectInfo(m_image, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(data_size, sizeof(mem_req));
    ASSERT_NE(0, mem_req.size) << "xglGetObjectInfo (Event): Failed - expect images to require memory";

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
    err = xglAllocMemory(device(), &mem_info, &m_image_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(m_image, m_image_mem, 0);
    ASSERT_XGL_SUCCESS(err);
}

void XglImageTest::DestroyImage()
{
    // All done with image memory, clean up
    ASSERT_XGL_SUCCESS(xglBindObjectMemory(m_image, XGL_NULL_HANDLE, 0));

    ASSERT_XGL_SUCCESS(xglFreeMemory(m_image_mem));

    ASSERT_XGL_SUCCESS(xglDestroyObject(m_image));
}

void XglImageTest::CreateImageView(XGL_IMAGE_VIEW_CREATE_INFO *pCreateInfo,
                                   XGL_IMAGE_VIEW *pView)
{
    pCreateInfo->image = this->m_image;
    ASSERT_XGL_SUCCESS(xglCreateImageView(device(), pCreateInfo, pView));
}

void XglImageTest::DestroyImageView(XGL_IMAGE_VIEW imageView)
{
    ASSERT_XGL_SUCCESS(xglDestroyObject(imageView));
}

TEST_F(XglImageTest, CreateImageViewTest) {
    XGL_FORMAT fmt;
    XGL_IMAGE_VIEW imageView;
    XGL_RESULT err;

    fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    fmt.numericFormat = XGL_NUM_FMT_UINT;

    CreateImage(512, 256);

    //    typedef struct _XGL_IMAGE_VIEW_CREATE_INFO
    //    {
    //        XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
    //        const XGL_VOID*                         pNext;                  // Pointer to next structure
    //        XGL_IMAGE                               image;
    //        XGL_IMAGE_VIEW_TYPE                     viewType;
    //        XGL_FORMAT                              format;
    //        XGL_CHANNEL_MAPPING                     channels;
    //        XGL_IMAGE_SUBRESOURCE_RANGE             subresourceRange;
    //        XGL_FLOAT                               minLod;
    //    } XGL_IMAGE_VIEW_CREATE_INFO;
    XGL_IMAGE_VIEW_CREATE_INFO viewInfo = {};
    viewInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = XGL_IMAGE_VIEW_2D;
    viewInfo.format = fmt;

    viewInfo.channels.r = XGL_CHANNEL_SWIZZLE_R;
    viewInfo.channels.g = XGL_CHANNEL_SWIZZLE_G;
    viewInfo.channels.b = XGL_CHANNEL_SWIZZLE_B;
    viewInfo.channels.a = XGL_CHANNEL_SWIZZLE_A;

    viewInfo.subresourceRange.baseArraySlice = 0;
    viewInfo.subresourceRange.arraySize = 1;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.mipLevels = 1;
    viewInfo.subresourceRange.aspect = XGL_IMAGE_ASPECT_COLOR;

    //    XGL_RESULT XGLAPI xglCreateImageView(
    //        XGL_DEVICE                                  device,
    //        const XGL_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
    //        XGL_IMAGE_VIEW*                             pView);

    CreateImageView(&viewInfo, &imageView);

    DestroyImageView(imageView);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
