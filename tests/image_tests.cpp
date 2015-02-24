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
#include "xgltestbinding.h"
#include "test_common.h"

class XglImageTest : public ::testing::Test {
public:
    void CreateImage(uint32_t w, uint32_t h);
    void DestroyImage();

    void CreateImageView(XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
                         XGL_IMAGE_VIEW* pView);
    void DestroyImageView(XGL_IMAGE_VIEW imageView);
    XGL_DEVICE device() {return m_device->obj();}

protected:
    xgl_testing::Device *m_device;
    XGL_APPLICATION_INFO app_info;
    XGL_PHYSICAL_GPU objs[XGL_MAX_PHYSICAL_GPUS];
    uint32_t gpu_count;
    XGL_INSTANCE inst;
    XGL_IMAGE m_image;
    XGL_GPU_MEMORY *m_image_mem;
    uint32_t m_num_mem;

    virtual void SetUp() {
        XGL_RESULT err;

        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_API_VERSION;

        err = xglCreateInstance(&app_info, NULL, &this->inst);
        ASSERT_XGL_SUCCESS(err);
        err = xglEnumerateGpus(this->inst, XGL_MAX_PHYSICAL_GPUS,
                               &this->gpu_count, objs);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_GE(this->gpu_count, 1) << "No GPU available";

        this->m_device = new xgl_testing::Device(objs[0]);
        this->m_device->init();
    }

    virtual void TearDown() {
        xglDestroyInstance(this->inst);
    }
};


void XglImageTest::CreateImage(uint32_t w, uint32_t h)
{
    XGL_RESULT err;
    uint32_t mipCount;
    size_t size;
    XGL_FORMAT fmt;
    XGL_FORMAT_PROPERTIES image_fmt;

    mipCount = 0;

    uint32_t _w = w;
    uint32_t _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

    fmt = XGL_FMT_R8G8B8A8_UINT;
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
    //        const void*                             pNext;                      // Pointer to next structure.
    //        XGL_IMAGE_TYPE                          imageType;
    //        XGL_FORMAT                              format;
    //        XGL_EXTENT3D                            extent;
    //        uint32_t                                mipLevels;
    //        uint32_t                                arraySize;
    //        uint32_t                                samples;
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

    XGL_MEMORY_REQUIREMENTS *mem_req;
    size_t mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
    XGL_IMAGE_MEMORY_REQUIREMENTS img_reqs;
    size_t img_reqs_size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    XGL_MEMORY_ALLOC_IMAGE_INFO img_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
        .pNext = NULL,
    };
    XGL_MEMORY_ALLOC_INFO mem_info = {};
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.pNext = &img_alloc;

    err = xglGetObjectInfo(m_image, XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                    &num_alloc_size, &num_allocations);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(num_alloc_size,sizeof(num_allocations));
    mem_req = (XGL_MEMORY_REQUIREMENTS *) malloc(num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    m_image_mem = (XGL_GPU_MEMORY *) malloc(num_allocations * sizeof(XGL_GPU_MEMORY));
    m_num_mem = num_allocations;
    err = xglGetObjectInfo(m_image,
                    XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(mem_reqs_size, num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    err = xglGetObjectInfo(m_image,
                        XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                        &img_reqs_size, &img_reqs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(img_reqs_size, sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS));
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;

    for (uint32_t i = 0; i < num_allocations; i ++) {
        ASSERT_NE(0, mem_req[i].size) << "xglGetObjectInfo (Image): Failed - expect images to require memory";
        mem_info.allocationSize = mem_req[i].size;
        mem_info.memProps = XGL_MEMORY_PROPERTY_SHAREABLE_BIT;
        mem_info.memType = XGL_MEMORY_TYPE_IMAGE;
        mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

        /* allocate memory */
        err = xglAllocMemory(device(), &mem_info, &m_image_mem[i]);
        ASSERT_XGL_SUCCESS(err);

        /* bind memory */
        err = xglBindObjectMemory(m_image, i, m_image_mem[i], 0);
        ASSERT_XGL_SUCCESS(err);
    }
}

void XglImageTest::DestroyImage()
{
    XGL_RESULT err;
    // All done with image memory, clean up
    ASSERT_XGL_SUCCESS(xglBindObjectMemory(m_image, 0, XGL_NULL_HANDLE, 0));

    for (uint32_t i = 0 ; i < m_num_mem; i++) {
        err = xglFreeMemory(m_image_mem[i]);
        ASSERT_XGL_SUCCESS(err);
    }


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

    fmt = XGL_FMT_R8G8B8A8_UINT;

    CreateImage(512, 256);

    //    typedef struct _XGL_IMAGE_VIEW_CREATE_INFO
    //    {
    //        XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
    //        const void*                             pNext;                  // Pointer to next structure
    //        XGL_IMAGE                               image;
    //        XGL_IMAGE_VIEW_TYPE                     viewType;
    //        XGL_FORMAT                              format;
    //        XGL_CHANNEL_MAPPING                     channels;
    //        XGL_IMAGE_SUBRESOURCE_RANGE             subresourceRange;
    //        float                                   minLod;
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
    xgl_testing::set_error_callback(test_error_callback);
    return RUN_ALL_TESTS();
}
