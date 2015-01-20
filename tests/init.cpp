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
#include "icd-bil.h"

class XglTest : public ::testing::Test {
public:
    void CreateImageTest();
    void CreateCommandBufferTest();
    void CreatePipelineTest();
    void CreateShaderTest();
    void CreateShader(XGL_SHADER *pshader);

    XGL_DEVICE device() {return m_device->obj();}

protected:
    XGL_APPLICATION_INFO app_info;
    XGL_PHYSICAL_GPU objs[XGL_MAX_PHYSICAL_GPUS];
    XGL_UINT gpu_count;

    XGL_UINT m_device_id;
    xgl_testing::Device *m_device;
    XGL_PHYSICAL_GPU_PROPERTIES props;
    XGL_PHYSICAL_GPU_QUEUE_PROPERTIES queue_props;

    virtual void SetUp() {
        XGL_RESULT err;

        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

        err = xglInitAndEnumerateGpus(&app_info, NULL,
                                      XGL_MAX_PHYSICAL_GPUS, &this->gpu_count, objs);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_GE(this->gpu_count, 1) << "No GPU available";

        m_device_id = 0;
        this->m_device = new xgl_testing::Device(objs[m_device_id]);
        this->m_device->init();

        props = m_device->gpu().properties();
        queue_props = m_device->gpu().queue_properties()[0];

    }

    virtual void TearDown() {
        xglInitAndEnumerateGpus(&this->app_info, NULL, 0, &gpu_count, NULL);
    }
};

TEST(Initialization, xglInitAndEnumerateGpus) {
    XGL_APPLICATION_INFO app_info = {};
    XGL_PHYSICAL_GPU objs[XGL_MAX_PHYSICAL_GPUS];
    XGL_UINT gpu_count;
    XGL_RESULT err;
    xgl_testing::PhysicalGpu *gpu;
    XGL_CHAR *layers[16];
    XGL_SIZE layer_count;
    XGL_CHAR layer_buf[16][256];

    app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = "base";
    app_info.appVersion = 1;
    app_info.pEngineName = "unittest";
    app_info.engineVersion = 1;
    app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

    err = xglInitAndEnumerateGpus(&app_info, NULL,
                                  XGL_MAX_PHYSICAL_GPUS, &gpu_count, objs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(gpu_count, 1) << "No GPU available";

    for (int i = 0; i < 16; i++)
        layers[i] = &layer_buf[i][0];
    err = xglEnumerateLayers(objs[0], 16, 256, &layer_count, (XGL_CHAR * const *) layers, NULL);
    ASSERT_XGL_SUCCESS(err);
    for (int i = 0; i < layer_count; i++) {
        printf("Enumerated layers: %s ", layers[i]);
    }
    printf("\n");

    // TODO: Iterate over all GPUs
    gpu = new xgl_testing::PhysicalGpu(objs[0]);
    delete gpu;

    // TODO: Verify destroy functions
}

TEST_F(XglTest, AllocMemory) {
    XGL_RESULT err;
    XGL_MEMORY_ALLOC_INFO alloc_info = {};
    XGL_GPU_MEMORY gpu_mem;
    XGL_UINT8 *pData;
    XGL_UINT localHeap[1] = {0};

    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.allocationSize = 1024 * 1024; // 1MB
    alloc_info.alignment = 0;
    alloc_info.memProps = XGL_MEMORY_PROPERTY_SHAREABLE_BIT |
                          XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT;
    alloc_info.memType = XGL_MEMORY_TYPE_OTHER,
    alloc_info.heapCount = 1;
    alloc_info.pHeaps = localHeap;


    // TODO: Try variety of memory priorities
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(device(), &alloc_info, &gpu_mem);
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
    XGL_MEMORY_REQUIREMENTS mem_req;
    size_t data_size = sizeof(mem_req);
    XGL_RESULT err;

    //        typedef struct _XGL_EVENT_CREATE_INFO
    //        {
    //            XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO
    //            const XGL_VOID*                         pNext;      // Pointer to next structure
    //            XGL_FLAGS                               flags;      // Reserved
    //        } XGL_EVENT_CREATE_INFO;
    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = xglCreateEvent(device(), &event_info, &event);
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

    XGL_UINT heapInfo[mem_req.heapCount];
    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = mem_req.size;
    mem_info.alignment = mem_req.alignment;
    mem_info.memProps = XGL_MEMORY_PROPERTY_SHAREABLE_BIT;
    mem_info.heapCount = mem_req.heapCount;
    mem_info.pHeaps = heapInfo;
    memcpy(heapInfo, mem_req.pHeaps, sizeof(XGL_UINT)*mem_info.heapCount);
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    mem_info.memType = XGL_MEMORY_TYPE_OTHER;
    err = xglAllocMemory(device(), &mem_info, &event_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(event, 0, event_mem, 0);
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
    err = xglBindObjectMemory(event, 0, XGL_NULL_HANDLE, 0);
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

    err = xglCreateFence(device(), &fence_info, &fence);
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
    err = xglWaitForFences(device(), 1, &fence, XGL_TRUE, 0);
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
    size_t data_size;
    XGL_MEMORY_REQUIREMENTS mem_req;
    size_t query_result_size;
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

    err = xglCreateQueryPool(device(), &query_info, &query_pool);
    ASSERT_XGL_SUCCESS(err);

    data_size = sizeof(mem_req);
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

    XGL_UINT heapInfo[mem_req.heapCount];

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    // TODO: Is a simple multiple all that's needed here?
    mem_info.allocationSize = mem_req.size * MAX_QUERY_SLOTS;
    mem_info.alignment = mem_req.alignment;
    mem_info.heapCount = mem_req.heapCount;
    mem_info.pHeaps = heapInfo;
    memcpy(heapInfo, mem_req.pHeaps, sizeof(XGL_UINT)*mem_info.heapCount);
    mem_info.memProps = XGL_MEMORY_PROPERTY_SHAREABLE_BIT;
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    mem_info.memType = XGL_MEMORY_TYPE_OTHER;
    // TODO: Should this be pinned? Or maybe a separate test with pinned.
    err = xglAllocMemory(device(), &mem_info, &query_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(query_pool, 0, query_mem, 0);
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
    err = xglBindObjectMemory(query_pool, 0, XGL_NULL_HANDLE, 0);
    ASSERT_XGL_SUCCESS(err);

    err = xglDestroyObject(query_pool);
    ASSERT_XGL_SUCCESS(err);
}

void getQueue(xgl_testing::Device *device, XGL_QUEUE_TYPE qtype, const char *qname)
{
    int que_idx;
    XGL_RESULT err;
    XGL_QUEUE queue;

    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES props = device->gpu().queue_properties()[0];
    for (que_idx = 0; que_idx < props.queueCount; que_idx++) {
        err = xglGetDeviceQueue(device->obj(), qtype, que_idx, &queue);
        ASSERT_EQ(XGL_SUCCESS, err) << "xglGetDeviceQueue: " << qname << " queue #" << que_idx << ": Failed with error: " << xgl_result_string(err);
    }
}

TEST_F(XglTest, Queue)
{
    XGL_UINT que_idx;

    ASSERT_NE(0, queue_props.queueCount) << "No heaps available for GPU #" << m_device_id << ": " << props.gpuName;

//            XGL_RESULT XGLAPI xglGetDeviceQueue(
//                XGL_DEVICE                                  device,
//                XGL_QUEUE_TYPE                              queueType,
//                XGL_UINT                                    queueIndex,
//                XGL_QUEUE*                                  pQueue);
    /*
     * queue handles are retrieved from the device by calling
     * xglGetDeviceQueue() with a queue type and a requested logical
     * queue ID. The logical queue ID is a sequential number starting
     * from zero and referencing up to the number of queues requested
     * at device creation. Each queue type has its own sequence of IDs
     * starting at zero.
     */

    for (que_idx = 0; que_idx < queue_props.queueCount; que_idx++) {

//                typedef enum _XGL_QUEUE_FLAGS
//                {
//                    XGL_QUEUE_GRAPHICS_BIT                                  = 0x00000001,   // Queue supports graphics operations
//                    XGL_QUEUE_COMPUTE_BIT                                   = 0x00000002,   // Queue supports compute operations
//                    XGL_QUEUE_DMA_BIT                                       = 0x00000004,   // Queue supports DMA operations
//                    XGL_QUEUE_EXTENDED_BIT                                  = 0x80000000    // Extended queue
//                } XGL_QUEUE_FLAGS;

//                typedef enum _XGL_QUEUE_TYPE
//                {
//                    XGL_QUEUE_TYPE_GRAPHICS                                 = 0x1,
//                    XGL_QUEUE_TYPE_COMPUTE                                  = 0x2,
//                    XGL_QUEUE_TYPE_DMA                                      = 0x3,
//                    XGL_MAX_ENUM(_XGL_QUEUE_TYPE)
//                } XGL_QUEUE_TYPE;

        if (queue_props.queueFlags & XGL_QUEUE_GRAPHICS_BIT) {
            getQueue(m_device, XGL_QUEUE_TYPE_GRAPHICS, "Graphics");
        }

        if (queue_props.queueFlags & XGL_QUEUE_COMPUTE_BIT) {
            getQueue(m_device, XGL_QUEUE_TYPE_GRAPHICS, "Compute");
        }

        if (queue_props.queueFlags & XGL_QUEUE_DMA_BIT) {
            getQueue(m_device, XGL_QUEUE_TYPE_GRAPHICS, "DMA");
        }

        // TODO: What do we do about EXTENDED_BIT?

        /* Guide: pg 34:
         * The queue objects cannot be destroyed explicitly by an application
         * and are automatically destroyed when the associated device is destroyed.
         * Once the device is destroyed, attempting to use a queue results in
         * undefined behavior.
         */
    }
}

void XglTest::CreateImageTest()
{
    XGL_RESULT err;
    XGL_IMAGE image;
    XGL_UINT w, h, mipCount;
    XGL_SIZE size;
    XGL_FORMAT fmt;
    XGL_FORMAT_PROPERTIES image_fmt;
    size_t data_size;

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

    size = sizeof(image_fmt);
    err = xglGetFormatInfo(device(), fmt,
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
    err = xglCreateImage(device(), &imageCreateInfo, &image);
    ASSERT_XGL_SUCCESS(err);

    // Verify image resources
//    XGL_RESULT XGLAPI xglGetImageSubresourceInfo(
//        XGL_IMAGE                                   image,
//        const XGL_IMAGE_SUBRESOURCE*                pSubresource,
//        XGL_SUBRESOURCE_INFO_TYPE                   infoType,
//        XGL_SIZE*                                   pDataSize,
//        XGL_VOID*                                   pData);
//    typedef struct _XGL_SUBRESOURCE_LAYOUT
//    {
//        XGL_GPU_SIZE                            offset;                 // Specified in bytes
//        XGL_GPU_SIZE                            size;                   // Specified in bytes
//        XGL_GPU_SIZE                            rowPitch;               // Specified in bytes
//        XGL_GPU_SIZE                            depthPitch;             // Specified in bytes
//    } XGL_SUBRESOURCE_LAYOUT;

//    typedef struct _XGL_IMAGE_SUBRESOURCE
//    {
//        XGL_IMAGE_ASPECT                        aspect;
//        XGL_UINT                                mipLevel;
//        XGL_UINT                                arraySlice;
//    } XGL_IMAGE_SUBRESOURCE;
//    typedef enum _XGL_SUBRESOURCE_INFO_TYPE
//    {
//        // Info type for xglGetImageSubresourceInfo()
//        XGL_INFO_TYPE_SUBRESOURCE_LAYOUT                        = 0x00000000,

//        XGL_MAX_ENUM(_XGL_SUBRESOURCE_INFO_TYPE)
//    } XGL_SUBRESOURCE_INFO_TYPE;
    XGL_IMAGE_SUBRESOURCE subresource = {};
    subresource.aspect = XGL_IMAGE_ASPECT_COLOR;
    subresource.arraySlice = 0;

    _w = w;
    _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        XGL_SUBRESOURCE_LAYOUT layout = {};
        data_size = sizeof(layout);
        err = xglGetImageSubresourceInfo(image, &subresource, XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                         &data_size, &layout);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_EQ(sizeof(XGL_SUBRESOURCE_LAYOUT), data_size) << "Invalid structure (XGL_SUBRESOURCE_LAYOUT) size";

        // TODO: 4 should be replaced with pixel size for given format
        EXPECT_LE(_w * 4, layout.rowPitch) << "Pitch does not match expected image pitch";
        _w >>= 1;
        _h >>= 1;

        subresource.mipLevel++;
    }

    XGL_MEMORY_ALLOC_IMAGE_INFO img_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
        .pNext = NULL,

    };
    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_IMAGE_MEMORY_REQUIREMENTS img_reqs;
    XGL_SIZE img_reqs_size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
    data_size = sizeof(mem_req);
    err = xglGetObjectInfo(image, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(data_size, sizeof(mem_req));
    ASSERT_NE(0, mem_req.size) << "xglGetObjectInfo (Event): Failed - expect images to require memory";
    err = xglGetObjectInfo(image, XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                           &img_reqs_size, &img_reqs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(img_reqs_size, sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS));
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;
    //        XGL_RESULT XGLAPI xglAllocMemory(
    //            XGL_DEVICE                                  device,
    //            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    //            XGL_GPU_MEMORY*                             pMem);
    XGL_MEMORY_ALLOC_INFO mem_info = {};
    XGL_GPU_MEMORY image_mem;

    XGL_UINT heapInfo[mem_req.heapCount];
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.pNext = &img_alloc;
    mem_info.allocationSize = mem_req.size;
    mem_info.alignment = mem_req.alignment;
    mem_info.heapCount = mem_req.heapCount;
    mem_info.pHeaps = heapInfo;
    memcpy(heapInfo, mem_req.pHeaps, sizeof(XGL_UINT)*mem_info.heapCount);
    mem_info.memProps = XGL_MEMORY_PROPERTY_SHAREABLE_BIT;
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    mem_info.memType = XGL_MEMORY_TYPE_IMAGE;
    err = xglAllocMemory(device(), &mem_info, &image_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(image, 0, image_mem, 0);
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

    err = xglCreateImageView(device(), &viewInfo, &view);
    ASSERT_XGL_SUCCESS(err) << "xglCreateImageView failed";

    // TODO: Test image memory.

    // All done with image memory, clean up
    ASSERT_XGL_SUCCESS(xglBindObjectMemory(image, 0, XGL_NULL_HANDLE, 0));

    ASSERT_XGL_SUCCESS(xglFreeMemory(image_mem));

    ASSERT_XGL_SUCCESS(xglDestroyObject(image));
}

TEST_F(XglTest, CreateImage) {
    CreateImageTest();
}

void XglTest::CreateCommandBufferTest()
{
    XGL_RESULT err;
    XGL_CMD_BUFFER_CREATE_INFO info = {};
    XGL_CMD_BUFFER cmdBuffer;

//    typedef struct _XGL_CMD_BUFFER_CREATE_INFO
//    {
//        XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO
//        const XGL_VOID*                         pNext;
//        XGL_QUEUE_TYPE                          queueType;
//        XGL_FLAGS                               flags;
//    } XGL_CMD_BUFFER_CREATE_INFO;

    info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    info.queueType = XGL_QUEUE_TYPE_GRAPHICS;
    err = xglCreateCommandBuffer(device(), &info, &cmdBuffer);
    ASSERT_XGL_SUCCESS(err) << "xglCreateCommandBuffer failed";

    ASSERT_XGL_SUCCESS(xglDestroyObject(cmdBuffer));
}

TEST_F(XglTest, TestComandBuffer) {
    CreateCommandBufferTest();
}

void XglTest::CreateShader(XGL_SHADER *pshader)
{
    void *code;
    uint32_t codeSize;
    struct icd_bil_header *pBIL;
    XGL_RESULT err;

    codeSize = sizeof(struct icd_bil_header) + 100;
    code = malloc(codeSize);
    ASSERT_TRUE(NULL != code) << "malloc failed!";

    memset(code, 0, codeSize);

    // Indicate that this is BIL data.
    pBIL = (struct icd_bil_header *) code;
    pBIL->magic = ICD_BIL_MAGIC;
    pBIL->version = ICD_BIL_VERSION;

//    typedef struct _XGL_SHADER_CREATE_INFO
//    {
//        XGL_STRUCTURE_TYPE                      sType;              // Must be XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO
//        const XGL_VOID*                         pNext;              // Pointer to next structure
//        XGL_SIZE                                codeSize;           // Specified in bytes
//        const XGL_VOID*                         pCode;
//        XGL_FLAGS                               flags;              // Reserved
//    } XGL_SHADER_CREATE_INFO;

    XGL_SHADER_CREATE_INFO createInfo;
    XGL_SHADER shader;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.pCode = code;
    createInfo.codeSize = codeSize;
    createInfo.flags = 0;
    err = xglCreateShader(device(), &createInfo, &shader);
    ASSERT_XGL_SUCCESS(err);

    *pshader = shader;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    xgl_testing::set_error_callback(test_error_callback);
    return RUN_ALL_TESTS();
}
