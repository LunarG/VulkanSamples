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

// #include "gtest/gtest.h"
#include "xglgpu.h"

class XglTest : public ::testing::Test {
public:
    XglGpu *gpu;
    void CreateImageTest();

protected:
    XGL_APPLICATION_INFO app_info;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count;

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

        this->gpu = new XglGpu(0, objs[0]);
    }

    virtual void TearDown() {
        xglInitAndEnumerateGpus(&this->app_info, NULL, 0, &gpu_count, NULL);
    }
};

TEST(Initialization, xglInitAndEnumerateGpus) {
    XGL_APPLICATION_INFO app_info = {};
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count;
    XGL_RESULT err;
    XglGpu *gpu;

    app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = (const XGL_CHAR *) "base";
    app_info.appVersion = 1;
    app_info.pEngineName = (const XGL_CHAR *) "unittest";
    app_info.engineVersion = 1;
    app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

    err = xglInitAndEnumerateGpus(&app_info, NULL,
                                  MAX_GPUS, &gpu_count, objs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(1, gpu_count) << "No GPU available";

    // TODO: Iterate over all GPUs
    gpu = new XglGpu(0, objs[0]);

    // TODO: Verify destroy functions
}

TEST_F(XglTest, AllocMemory) {
    XGL_RESULT err;
    XGL_MEMORY_ALLOC_INFO alloc_info = {};
    XGL_GPU_MEMORY gpu_mem;
    XGL_UINT8 *pData;

    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.allocationSize = 1024 * 1024; // 1MB
    alloc_info.alignment = 0;
    alloc_info.heapCount = 1;
    alloc_info.heaps[0] = 0; // TODO: Reference other heaps

    // TODO: Pick heap properties indicated by heap info
    alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;

    // TODO: Try variety of memory priorities
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(this->gpu->device(), &alloc_info, &gpu_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglMapMemory(gpu_mem, 0, (XGL_VOID **) &pData);
    ASSERT_XGL_SUCCESS(err);

    memset(pData, 0x55, alloc_info.allocationSize);
    EXPECT_EQ(0x55, pData[0]) << "Memory read not same a write";

    err = xglUnmapMemory(gpu_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglFreeMemory(gpu_mem);
    ASSERT_XGL_SUCCESS(err);
}

TEST_F(XglTest, Event) {
    XGL_EVENT_CREATE_INFO event_info;
    XGL_EVENT event;
    XGL_UINT data_size;
    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_RESULT err;

    //        typedef struct _XGL_EVENT_CREATE_INFO
    //        {
    //            XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO
    //            const XGL_VOID*                         pNext;      // Pointer to next structure
    //            XGL_FLAGS                               flags;      // Reserved
    //        } XGL_EVENT_CREATE_INFO;
    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = xglCreateEvent(this->gpu->device(), &event_info, &event);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetObjectInfo(event, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);

    //        XGL_RESULT XGLAPI xglAllocMemory(
    //            XGL_DEVICE                                  device,
    //            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    //            XGL_GPU_MEMORY*                             pMem);
    XGL_MEMORY_ALLOC_INFO mem_info;
    XGL_GPU_MEMORY event_mem;

    ASSERT_NE(0, mem_req.size) << "xglGetObjectInfo (Event): Failed - expect events to require memory";

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = mem_req.size;
    mem_info.alignment = mem_req.alignment;
    mem_info.heapCount = mem_req.heapCount;
    memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    mem_info.flags = XGL_MEMORY_ALLOC_SHAREABLE_BIT;
    err = xglAllocMemory(this->gpu->device(), &mem_info, &event_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(event, event_mem, 0);
    ASSERT_XGL_SUCCESS(err);

    err = xglResetEvent(event);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetEventStatus(event);
    ASSERT_EQ(XGL_EVENT_RESET, err);

    err = xglSetEvent(event);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetEventStatus(event);
    ASSERT_EQ(XGL_EVENT_SET, err);

    // TODO: Test actual synchronization with command buffer event.

    // All done with event memory, clean up
    err = xglBindObjectMemory(event, XGL_NULL_HANDLE, 0);
    ASSERT_XGL_SUCCESS(err);

    err = xglDestroyObject(event);
    ASSERT_XGL_SUCCESS(err);
}

TEST_F(XglTest, Fence) {
    XGL_RESULT err;
    XGL_FENCE_CREATE_INFO fence_info;
    XGL_FENCE fence;

    memset(&fence_info, 0, sizeof(fence_info));

    //            typedef struct _XGL_FENCE_CREATE_INFO
    //            {
    //                XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO
    //                const XGL_VOID*                         pNext;      // Pointer to next structure
    //                XGL_FLAGS                               flags;      // Reserved
    fence_info.sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    err = xglCreateFence(this->gpu->device(), &fence_info, &fence);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetFenceStatus(fence);
    // We've not submitted this fence on a command buffer so should get
    // XGL_ERROR_UNAVAILABLE
    EXPECT_EQ(XGL_ERROR_UNAVAILABLE, err);

    // Test glxWaitForFences
    //        XGL_RESULT XGLAPI xglWaitForFences(
    //            XGL_DEVICE                                  device,
    //            XGL_UINT                                    fenceCount,
    //            const XGL_FENCE*                            pFences,
    //            XGL_BOOL                                    waitAll,
    //            XGL_UINT64                                  timeout);
    err = xglWaitForFences(this->gpu->device(), 1, &fence, XGL_TRUE, 0);
    EXPECT_EQ(XGL_ERROR_UNAVAILABLE, err);

    // TODO: Attached to command buffer and test GetFenceStatus
    // TODO: Add some commands and submit the command buffer

    err = xglDestroyObject(fence);
    ASSERT_XGL_SUCCESS(err);

}

#define MAX_QUERY_SLOTS 10

TEST_F(XglTest, Query) {
    XGL_QUERY_POOL_CREATE_INFO query_info;
    XGL_QUERY_POOL query_pool;
    XGL_UINT data_size;
    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_UINT query_result_size;
    XGL_UINT *query_result_data;
    XGL_RESULT err;

    //        typedef enum _XGL_QUERY_TYPE
    //        {
    //            XGL_QUERY_OCCLUSION                                     = 0x00000000,
    //            XGL_QUERY_PIPELINE_STATISTICS                           = 0x00000001,

    //            XGL_QUERY_TYPE_BEGIN_RANGE                              = XGL_QUERY_OCCLUSION,
    //            XGL_QUERY_TYPE_END_RANGE                                = XGL_QUERY_PIPELINE_STATISTICS,
    //            XGL_NUM_QUERY_TYPE                                      = (XGL_QUERY_TYPE_END_RANGE - XGL_QUERY_TYPE_BEGIN_RANGE + 1),
    //            XGL_MAX_ENUM(_XGL_QUERY_TYPE)
    //        } XGL_QUERY_TYPE;

    //        typedef struct _XGL_QUERY_POOL_CREATE_INFO
    //        {
    //            XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
    //            const XGL_VOID*                         pNext;      // Pointer to next structure
    //            XGL_QUERY_TYPE                          queryType;
    //            XGL_UINT                                slots;
    //        } XGL_QUERY_POOL_CREATE_INFO;

    memset(&query_info, 0, sizeof(query_info));
    query_info.sType = XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_info.queryType = XGL_QUERY_OCCLUSION;
    query_info.slots = MAX_QUERY_SLOTS;

    //        XGL_RESULT XGLAPI xglCreateQueryPool(
    //            XGL_DEVICE                                  device,
    //            const XGL_QUERY_POOL_CREATE_INFO*           pCreateInfo,
    //            XGL_QUERY_POOL*                             pQueryPool);

    err = xglCreateQueryPool(this->gpu->device(), &query_info, &query_pool);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetObjectInfo(query_pool, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_NE(0, data_size) << "Invalid data_size";

    //        XGL_RESULT XGLAPI xglAllocMemory(
    //            XGL_DEVICE                                  device,
    //            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    //            XGL_GPU_MEMORY*                             pMem);
    XGL_MEMORY_ALLOC_INFO mem_info;
    XGL_GPU_MEMORY query_mem;

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    // TODO: Is a simple multiple all that's needed here?
    mem_info.allocationSize = mem_req.size * MAX_QUERY_SLOTS;
    mem_info.alignment = mem_req.alignment;
    mem_info.heapCount = mem_req.heapCount;
    memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    // TODO: are the flags right?
    // TODO: Should this be pinned? Or maybe a separate test with pinned.
    mem_info.flags = XGL_MEMORY_ALLOC_SHAREABLE_BIT;
    err = xglAllocMemory(this->gpu->device(), &mem_info, &query_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(query_pool, query_mem, 0);
    ASSERT_XGL_SUCCESS(err);

    // TODO: Test actual synchronization with command buffer event.
    // TODO: Create command buffer
    // TODO: xglCmdResetQueryPool
    // TODO: xglCmdBeginQuery
    // TODO: commands
    // TOOD: xglCmdEndQuery

    err = xglGetQueryPoolResults(query_pool, 0, MAX_QUERY_SLOTS,
                                 &query_result_size, XGL_NULL_HANDLE);
    ASSERT_XGL_SUCCESS(err);

    if (query_result_size > 0) {
        query_result_data = new XGL_UINT [query_result_size];
        err = xglGetQueryPoolResults(query_pool, 0, MAX_QUERY_SLOTS,
                                     &query_result_size, query_result_data);
        ASSERT_XGL_SUCCESS(err);

        // TODO: Test Query result data.

    }

    // All done with QueryPool memory, clean up
    err = xglBindObjectMemory(query_pool, XGL_NULL_HANDLE, 0);
    ASSERT_XGL_SUCCESS(err);

    err = xglDestroyObject(query_pool);
    ASSERT_XGL_SUCCESS(err);
}

void XglTest::CreateImageTest()
{
    XGL_RESULT err;
    XGL_IMAGE image;
    XGL_UINT w, h, mipCount;
    XGL_SIZE size;
    XGL_FORMAT fmt;
    XGL_FORMAT_PROPERTIES image_fmt;

    w =512;
    h = 256;
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

    err = xglGetFormatInfo(this->gpu->device(), fmt,
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
    err = xglCreateImage(this->gpu->device(), &imageCreateInfo, &image);
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_UINT data_size;
    err = xglGetObjectInfo(image, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(data_size, sizeof(mem_req));
    ASSERT_NE(0, mem_req.size) << "xglGetObjectInfo (Event): Failed - expect images to require memory";

    //        XGL_RESULT XGLAPI xglAllocMemory(
    //            XGL_DEVICE                                  device,
    //            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    //            XGL_GPU_MEMORY*                             pMem);
    XGL_MEMORY_ALLOC_INFO mem_info;
    XGL_GPU_MEMORY image_mem;

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = mem_req.size;
    mem_info.alignment = mem_req.alignment;
    mem_info.heapCount = mem_req.heapCount;
    memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    mem_info.flags = XGL_MEMORY_ALLOC_SHAREABLE_BIT;
    err = xglAllocMemory(this->gpu->device(), &mem_info, &image_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(image, image_mem, 0);
    ASSERT_XGL_SUCCESS(err);

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
    XGL_IMAGE_VIEW view;
    viewInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
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

    err = xglCreateImageView(gpu->device(), &viewInfo, &view);
    ASSERT_XGL_SUCCESS(err);

    // TODO: Test image memory.

    // All done with image memory, clean up
    ASSERT_XGL_SUCCESS(xglBindObjectMemory(image, XGL_NULL_HANDLE, 0));

    ASSERT_XGL_SUCCESS(xglFreeMemory(image_mem));

    ASSERT_XGL_SUCCESS(xglDestroyObject(image));
}

TEST_F(XglTest, CreateImage) {
    CreateImageTest();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
