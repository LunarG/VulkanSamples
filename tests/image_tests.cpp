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


//  VK tests
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


// Verify VK driver initialization

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <vulkan.h>
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "vktestbinding.h"
#include "test_common.h"

class VkImageTest : public ::testing::Test {
public:
    void CreateImage(uint32_t w, uint32_t h);
    void DestroyImage();

    void CreateImageView(VK_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
                         VK_IMAGE_VIEW* pView);
    void DestroyImageView(VK_IMAGE_VIEW imageView);
    VK_DEVICE device() {return m_device->obj();}

protected:
    vk_testing::Device *m_device;
    VK_APPLICATION_INFO app_info;
    VK_PHYSICAL_GPU objs[VK_MAX_PHYSICAL_GPUS];
    uint32_t gpu_count;
    VK_INSTANCE inst;
    VK_IMAGE m_image;
    VK_GPU_MEMORY *m_image_mem;
    uint32_t m_num_mem;

    virtual void SetUp() {
        VK_RESULT err;

        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION;
        VkInstanceCreateInfo inst_info = {};
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pNext = NULL;
        inst_info.pAppInfo = &app_info;
        inst_info.pAllocCb = NULL;
        inst_info.extensionCount = 0;
        inst_info.ppEnabledExtensionNames = NULL;
        err = vkCreateInstance(&inst_info, &this->inst);
        ASSERT_VK_SUCCESS(err);
        err = vkEnumerateGpus(this->inst, VK_MAX_PHYSICAL_GPUS,
                               &this->gpu_count, objs);
        ASSERT_VK_SUCCESS(err);
        ASSERT_GE(this->gpu_count, 1) << "No GPU available";

        this->m_device = new vk_testing::Device(objs[0]);
        this->m_device->init();
    }

    virtual void TearDown() {
        vkDestroyInstance(this->inst);
    }
};


void VkImageTest::CreateImage(uint32_t w, uint32_t h)
{
    VK_RESULT err;
    uint32_t mipCount;
    size_t size;
    VK_FORMAT fmt;
    VK_FORMAT_PROPERTIES image_fmt;

    mipCount = 0;

    uint32_t _w = w;
    uint32_t _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

    fmt = VK_FMT_R8G8B8A8_UINT;
    // TODO: Pick known good format rather than just expect common format
    /*
     * XXX: What should happen if given NULL HANDLE for the pData argument?
     * We're not requesting VK_INFO_TYPE_MEMORY_REQUIREMENTS so there is
     * an expectation that pData is a valid pointer.
     * However, why include a returned size value? That implies that the
     * amount of data may vary and that doesn't work well for using a
     * fixed structure.
     */
    size = sizeof(image_fmt);
    err = vkGetFormatInfo(this->device(), fmt,
                           VK_INFO_TYPE_FORMAT_PROPERTIES,
                           &size, &image_fmt);
    ASSERT_VK_SUCCESS(err);

    //    typedef struct _VK_IMAGE_CREATE_INFO
    //    {
    //        VK_STRUCTURE_TYPE                      sType;                      // Must be VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
    //        const void*                             pNext;                      // Pointer to next structure.
    //        VK_IMAGE_TYPE                          imageType;
    //        VK_FORMAT                              format;
    //        VK_EXTENT3D                            extent;
    //        uint32_t                                mipLevels;
    //        uint32_t                                arraySize;
    //        uint32_t                                samples;
    //        VK_IMAGE_TILING                        tiling;
    //        VK_FLAGS                               usage;                      // VK_IMAGE_USAGE_FLAGS
    //        VK_FLAGS                               flags;                      // VK_IMAGE_CREATE_FLAGS
    //    } VK_IMAGE_CREATE_INFO;


    VK_IMAGE_CREATE_INFO imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_2D;
    imageCreateInfo.format = fmt;
    imageCreateInfo.arraySize = 1;
    imageCreateInfo.extent.width = w;
    imageCreateInfo.extent.height = h;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipCount;
    imageCreateInfo.samples = 1;
    if (image_fmt.linearTilingFeatures & VK_FORMAT_IMAGE_SHADER_READ_BIT) {
        imageCreateInfo.tiling = VK_LINEAR_TILING;
    }
    else if (image_fmt.optimalTilingFeatures & VK_FORMAT_IMAGE_SHADER_READ_BIT) {
        imageCreateInfo.tiling = VK_OPTIMAL_TILING;
    }
    else {
        ASSERT_TRUE(false) << "Cannot find supported tiling format - Exiting";
    }

    // Image usage flags
    //    typedef enum _VK_IMAGE_USAGE_FLAGS
    //    {
    //        VK_IMAGE_USAGE_SHADER_ACCESS_READ_BIT                  = 0x00000001,
    //        VK_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT                 = 0x00000002,
    //        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                    = 0x00000004,
    //        VK_IMAGE_USAGE_DEPTH_STENCIL_BIT                       = 0x00000008,
    //    } VK_IMAGE_USAGE_FLAGS;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //    VK_RESULT VKAPI vkCreateImage(
    //        VK_DEVICE                                  device,
    //        const VK_IMAGE_CREATE_INFO*                pCreateInfo,
    //        VK_IMAGE*                                  pImage);
    err = vkCreateImage(device(), &imageCreateInfo, &m_image);
    ASSERT_VK_SUCCESS(err);

    VK_MEMORY_REQUIREMENTS *mem_req;
    size_t mem_reqs_size = sizeof(VK_MEMORY_REQUIREMENTS);
    VK_IMAGE_MEMORY_REQUIREMENTS img_reqs;
    size_t img_reqs_size = sizeof(VK_IMAGE_MEMORY_REQUIREMENTS);
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    VkMemoryAllocImageInfo img_alloc = {};
    img_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO;
    img_alloc.pNext = NULL;

    VkMemoryAllocInfo mem_info = {};
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.pNext = &img_alloc;

    err = vkGetObjectInfo(m_image, VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                    &num_alloc_size, &num_allocations);
    ASSERT_VK_SUCCESS(err);
    ASSERT_EQ(num_alloc_size,sizeof(num_allocations));
    mem_req = (VK_MEMORY_REQUIREMENTS *) malloc(num_allocations * sizeof(VK_MEMORY_REQUIREMENTS));
    m_image_mem = (VK_GPU_MEMORY *) malloc(num_allocations * sizeof(VK_GPU_MEMORY));
    m_num_mem = num_allocations;
    err = vkGetObjectInfo(m_image,
                    VK_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_req);
    ASSERT_VK_SUCCESS(err);
    ASSERT_EQ(mem_reqs_size, num_allocations * sizeof(VK_MEMORY_REQUIREMENTS));
    err = vkGetObjectInfo(m_image,
                        VK_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                        &img_reqs_size, &img_reqs);
    ASSERT_VK_SUCCESS(err);
    ASSERT_EQ(img_reqs_size, sizeof(VK_IMAGE_MEMORY_REQUIREMENTS));
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;

    for (uint32_t i = 0; i < num_allocations; i ++) {
        ASSERT_NE(0, mem_req[i].size) << "vkGetObjectInfo (Image): Failed - expect images to require memory";
        mem_info.allocationSize = mem_req[i].size;
        mem_info.memProps = VK_MEMORY_PROPERTY_SHAREABLE_BIT;
        mem_info.memType = VK_MEMORY_TYPE_IMAGE;
        mem_info.memPriority = VK_MEMORY_PRIORITY_NORMAL;

        /* allocate memory */
        err = vkAllocMemory(device(), &mem_info, &m_image_mem[i]);
        ASSERT_VK_SUCCESS(err);

        /* bind memory */
        err = vkBindObjectMemory(m_image, i, m_image_mem[i], 0);
        ASSERT_VK_SUCCESS(err);
    }
}

void VkImageTest::DestroyImage()
{
    VK_RESULT err;
    // All done with image memory, clean up
    ASSERT_VK_SUCCESS(vkBindObjectMemory(m_image, 0, VK_NULL_HANDLE, 0));

    for (uint32_t i = 0 ; i < m_num_mem; i++) {
        err = vkFreeMemory(m_image_mem[i]);
        ASSERT_VK_SUCCESS(err);
    }


    ASSERT_VK_SUCCESS(vkDestroyObject(m_image));
}

void VkImageTest::CreateImageView(VK_IMAGE_VIEW_CREATE_INFO *pCreateInfo,
                                   VK_IMAGE_VIEW *pView)
{
    pCreateInfo->image = this->m_image;
    ASSERT_VK_SUCCESS(vkCreateImageView(device(), pCreateInfo, pView));
}

void VkImageTest::DestroyImageView(VK_IMAGE_VIEW imageView)
{
    ASSERT_VK_SUCCESS(vkDestroyObject(imageView));
}

TEST_F(VkImageTest, CreateImageViewTest) {
    VK_FORMAT fmt;
    VK_IMAGE_VIEW imageView;

    fmt = VK_FMT_R8G8B8A8_UINT;

    CreateImage(512, 256);

    //    typedef struct _VK_IMAGE_VIEW_CREATE_INFO
    //    {
    //        VK_STRUCTURE_TYPE                      sType;                  // Must be VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
    //        const void*                             pNext;                  // Pointer to next structure
    //        VK_IMAGE                               image;
    //        VK_IMAGE_VIEW_TYPE                     viewType;
    //        VK_FORMAT                              format;
    //        VK_CHANNEL_MAPPING                     channels;
    //        VK_IMAGE_SUBRESOURCE_RANGE             subresourceRange;
    //        float                                   minLod;
    //    } VK_IMAGE_VIEW_CREATE_INFO;
    VK_IMAGE_VIEW_CREATE_INFO viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_2D;
    viewInfo.format = fmt;

    viewInfo.channels.r = VK_CHANNEL_SWIZZLE_R;
    viewInfo.channels.g = VK_CHANNEL_SWIZZLE_G;
    viewInfo.channels.b = VK_CHANNEL_SWIZZLE_B;
    viewInfo.channels.a = VK_CHANNEL_SWIZZLE_A;

    viewInfo.subresourceRange.baseArraySlice = 0;
    viewInfo.subresourceRange.arraySize = 1;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.mipLevels = 1;
    viewInfo.subresourceRange.aspect = VK_IMAGE_ASPECT_COLOR;

    //    VK_RESULT VKAPI vkCreateImageView(
    //        VK_DEVICE                                  device,
    //        const VK_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
    //        VK_IMAGE_VIEW*                             pView);

    CreateImageView(&viewInfo, &imageView);

    DestroyImageView(imageView);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    vk_testing::set_error_callback(test_error_callback);
    return RUN_ALL_TESTS();
}
