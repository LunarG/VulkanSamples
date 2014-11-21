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

// Blit (copy, clear, and resolve) tests

#include <string.h>
#include <xgl.h>
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "xgldevice.h"
#include "xglimage.h"
#include "xgltestframework.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

class XglBlitTest : public XglTestFramework
{
protected:
    XGL_APPLICATION_INFO app_info;
    XglDevice *m_device;

    XGL_CMD_BUFFER m_cmd;

    XGL_MEMORY_REF m_mem_refs[8];
    XGL_UINT m_mem_ref_count;

    virtual void SetUp();
    virtual void TearDown();

    XGL_DEVICE device() { return m_device->device(); }

    void ClearMemoryRefs()
    {
        m_mem_ref_count = 0;
    }

    void AddMemoryRef(XGL_GPU_MEMORY mem, bool readonly)
    {
        ASSERT_LE(m_mem_ref_count, ARRAY_SIZE(m_mem_refs));

        m_mem_refs[m_mem_ref_count].mem = mem;
        m_mem_refs[m_mem_ref_count].flags =
            (readonly) ? XGL_MEMORY_REF_READ_ONLY_BIT : 0;
        m_mem_ref_count++;
    }

    XGL_GPU_MEMORY AllocMemory(XGL_GPU_SIZE size)
    {
        XGL_MEMORY_ALLOC_INFO info;
        XGL_GPU_MEMORY mem;
        XGL_RESULT err;

        memset(&info, 0, sizeof(info));
        info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        info.allocationSize = size;
        info.alignment = 1;
        info.heapCount = 1;
        info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

        err = xglAllocMemory(device(), &info, &mem);
        if (err)
            mem = XGL_NULL_HANDLE;

        return mem;
    }

    XGL_GPU_MEMORY AddMemory(XGL_GPU_SIZE size, bool readonly)
    {
        XGL_GPU_MEMORY mem;

        mem = AllocMemory(size);
        if (mem)
            AddMemoryRef(mem, readonly);

        return mem;
    }

    void BeginCmd()
    {
        XGL_RESULT err;

        err = xglBeginCommandBuffer(m_cmd,
                XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
                XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT);
        ASSERT_XGL_SUCCESS(err);
    }

    void EndAndSubmitCmd()
    {
        XGL_RESULT err;

        err = xglEndCommandBuffer(m_cmd);
        ASSERT_XGL_SUCCESS(err);

        err = xglQueueSubmit(m_device->m_queue, 1, &m_cmd, m_mem_ref_count, m_mem_refs, NULL );
        ASSERT_XGL_SUCCESS(err);

        err = xglQueueWaitIdle(m_device->m_queue);
        ASSERT_XGL_SUCCESS(err);
    }
};

void XglBlitTest::SetUp()
{
    XGL_CMD_BUFFER_CREATE_INFO cmd_info;
    XGL_PHYSICAL_GPU gpu;
    XGL_UINT count;
    XGL_RESULT err;

    memset(&app_info, 0, sizeof(app_info));
    app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = (const XGL_CHAR *) "blit tests";
    app_info.appVersion = 1;
    app_info.pEngineName = (const XGL_CHAR *) "unittest";
    app_info.engineVersion = 1;
    app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

    err = xglInitAndEnumerateGpus(&app_info, NULL, 1, &count, &gpu);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(1, count) << "No GPU available";

    m_device = new XglDevice(0, gpu);
    m_device->get_device_queue();

    memset(&cmd_info, 0, sizeof(cmd_info));
    cmd_info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmd_info.queueType = XGL_QUEUE_TYPE_GRAPHICS;

    err = xglCreateCommandBuffer(device(), &cmd_info, &m_cmd);
    ASSERT_XGL_SUCCESS(err) << "xglCreateCommandBuffer failed";
}

void XglBlitTest::TearDown()
{
    XGL_UINT dummy_count;

    xglDestroyObject(m_cmd);
    xglInitAndEnumerateGpus(&app_info, NULL, 0, &dummy_count, NULL);
}

TEST_F(XglBlitTest, FillMemory)
{
    const struct {
        XGL_GPU_SIZE offset;
        XGL_GPU_SIZE size;
        XGL_UINT value;
    } ranges[] = {
        {  0, 64, 0x11111111 }, // 16-byte aligned
        { 64, 12, 0x22222222 }, // 4-byte aligned
        { 76,  4, 0x33333333 }, // min. fill size
        { 80, 20, 0x44444444 },
        { 92,  8, 0x55555555 }, // overlapped
    };
    XGL_GPU_MEMORY mem;
    XGL_RESULT err;
    void *data;
    XGL_UINT i;

    ClearMemoryRefs();

    mem = AddMemory(256, false);
    ASSERT_NE((XGL_GPU_MEMORY) XGL_NULL_HANDLE, mem);

    BeginCmd();
    for (i = 0; i < ARRAY_SIZE(ranges); i++) {
        xglCmdFillMemory(m_cmd, mem, ranges[i].offset,
                ranges[i].size, ranges[i].value);
    }
    EndAndSubmitCmd();

    err = xglMapMemory(mem, 0, &data);
    ASSERT_XGL_SUCCESS(err);

    for (i = 0; i < ARRAY_SIZE(ranges); i++) {
        const XGL_UINT expected = ranges[i].value;
        const XGL_UINT *real = (const XGL_UINT *)
            ((char *) data + ranges[i].offset);
        XGL_UINT count, j;

        count = ranges[i].size / 4;

        /* check if the next range overlaps */
        if (i + 1 < ARRAY_SIZE(ranges)) {
            if (ranges[i].offset + ranges[i].size > ranges[i + 1].offset)
                count = (ranges[i + 1].offset - ranges[i].offset) / 4;
        }

        for (j = 0; j < count; j++)
            ASSERT_EQ(expected, real[j]);
    }

    xglUnmapMemory(mem);
    xglFreeMemory(mem);
}

TEST_F(XglBlitTest, CopyMemory)
{
    XGL_GPU_MEMORY src, dst;
    XGL_MEMORY_COPY regions[17];
    XGL_RESULT err;
    void *data;
    XGL_UINT i;

    ClearMemoryRefs();

    src = AddMemory(256, false);
    ASSERT_NE((XGL_GPU_MEMORY) XGL_NULL_HANDLE, src);

    err = xglMapMemory(src, 0, &data);
    ASSERT_XGL_SUCCESS(err);
    for (i = 0; i < 256; i++)
        ((char *) data)[i] = i;
    xglUnmapMemory(src);

    dst = AddMemory(256, false);
    ASSERT_NE((XGL_GPU_MEMORY) XGL_NULL_HANDLE, dst);

    /* copy with various alignments */
    for (i = 0; i < 16; i++) {
        regions[i].copySize = i + 1;
        regions[i].srcOffset = i * 8;

        if (i > 0) {
            regions[i].destOffset = regions[i - 1].destOffset +
                                    regions[i - 1].copySize;
        } else {
            regions[i].destOffset = 0;
        }
    }

    regions[i].srcOffset = 192;
    regions[i].destOffset = 192;
    regions[i].copySize = 64;

    BeginCmd();

    xglCmdCopyMemory(m_cmd, src, dst, 16, regions);
    xglCmdCopyMemory(m_cmd, src, dst, 1, &regions[16]);

    EndAndSubmitCmd();

    err = xglMapMemory(dst, 0, &data);
    ASSERT_XGL_SUCCESS(err);

    for (i = 0; i < ARRAY_SIZE(regions); i++) {
        const unsigned char *real = (const unsigned char *) data +
            regions[i].destOffset;
        XGL_UINT j;

        for (j = 0; j < regions[i].copySize; j++)
            ASSERT_EQ(regions[i].srcOffset + j, real[j]);
    }

    xglUnmapMemory(dst);
    xglFreeMemory(src);
    xglFreeMemory(dst);
}

TEST_F(XglBlitTest, ClearColorImageBasic)
{
    const XGL_FLOAT color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    const XGL_UINT width = 64;
    const XGL_UINT height = 64;
    XglImage *img;
    XGL_FORMAT format;
    XGL_IMAGE_SUBRESOURCE subres;
    XGL_IMAGE_SUBRESOURCE_RANGE subres_range;
    XGL_IMAGE_STATE_TRANSITION transition;
    XGL_RESULT err;

    format.channelFormat = XGL_CH_FMT_R8G8B8A8;
    format.numericFormat = XGL_NUM_FMT_UNORM;

    subres.aspect = XGL_IMAGE_ASPECT_COLOR;
    subres.mipLevel = 0;
    subres.arraySlice = 0;

    subres_range.aspect = XGL_IMAGE_ASPECT_COLOR;
    subres_range.baseMipLevel = 0;
    subres_range.mipLevels = 1;
    subres_range.baseArraySlice = 0;
    subres_range.arraySize = 1;

    img = new XglImage(m_device);
    img->init(width, height, format, XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    ClearMemoryRefs();
    AddMemoryRef(img->memory(), false);

    BeginCmd();

    transition.image = img->image();
    transition.oldState = XGL_IMAGE_STATE_UNINITIALIZED_TARGET;
    transition.newState = XGL_IMAGE_STATE_CLEAR;
    transition.subresourceRange = subres_range;
    xglCmdPrepareImages(m_cmd, 1, &transition);

    xglCmdClearColorImage(m_cmd, img->image(), color, 1, &subres_range);

    EndAndSubmitCmd();

    {
        XGL_SUBRESOURCE_LAYOUT layout;
        XGL_SIZE layout_size= sizeof(layout);
        XGL_UINT x, y;
        void *data;

        err = img->MapMemory(&data);
        ASSERT_XGL_SUCCESS(err);

        err = xglGetImageSubresourceInfo(img->image(), &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_EQ(sizeof(layout), layout_size);

        for (y = 0; y < height; y++) {
            const XGL_UINT *real = (const XGL_UINT *)
                ((char *) data + layout.offset + layout.rowPitch * y);

            for (x = 0; x < width; x++)
                ASSERT_EQ(0xff00ff00, real[x]);
        }

        img->UnmapMemory();
    }

    delete img;
}

TEST_F(XglBlitTest, ClearDepthStencilBasic)
{
    const XGL_FLOAT clear_depth = 0.4f;
    const XGL_UINT width = 64;
    const XGL_UINT height = 64;
    XglImage *img;
    XGL_FORMAT format;
    XGL_IMAGE_SUBRESOURCE subres;
    XGL_IMAGE_SUBRESOURCE_RANGE subres_range;
    XGL_IMAGE_STATE_TRANSITION transition;
    XGL_RESULT err;

    format.channelFormat = XGL_CH_FMT_R32;
    format.numericFormat = XGL_NUM_FMT_DS;

    subres.aspect = XGL_IMAGE_ASPECT_DEPTH;
    subres.mipLevel = 0;
    subres.arraySlice = 0;

    subres_range.aspect = XGL_IMAGE_ASPECT_DEPTH;
    subres_range.baseMipLevel = 0;
    subres_range.mipLevels = 1;
    subres_range.baseArraySlice = 0;
    subres_range.arraySize = 1;

    img = new XglImage(m_device);
    img->init(width, height, format, XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT,
              XGL_OPTIMAL_TILING);

    ClearMemoryRefs();
    AddMemoryRef(img->memory(), false);

    BeginCmd();

    transition.image = img->image();
    transition.oldState = XGL_IMAGE_STATE_UNINITIALIZED_TARGET;
    transition.newState = XGL_IMAGE_STATE_CLEAR;
    transition.subresourceRange = subres_range;
    xglCmdPrepareImages(m_cmd, 1, &transition);

    xglCmdClearDepthStencil(m_cmd, img->image(), clear_depth,
            0, 1, &subres_range);

    EndAndSubmitCmd();

    /*
     * TODO xglCmdCopyImageToMemory to linearize
     *
     * This works only because xglMapMemory calls intel_bo_map_gtt_async.
     */
    {
        XGL_SUBRESOURCE_LAYOUT layout;
        XGL_SIZE layout_size = sizeof(layout);
        XGL_UINT x, y;
        void *data;

        err = img->MapMemory(&data);
        ASSERT_XGL_SUCCESS(err);

        err = xglGetImageSubresourceInfo(img->image(), &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_EQ(sizeof(layout), layout_size);

        for (y = 0; y < height; y++) {
            const float *real = (const float *)
                ((char *) data + layout.offset + layout.rowPitch * y);

            for (x = 0; x < width; x++)
                ASSERT_EQ(clear_depth, real[x]);
        }

        img->UnmapMemory();
    }
}

int main(int argc, char **argv)
{
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    XglTestFramework::InitArgs(&argc, argv);

    ::testing::Environment* const xgl_test_env = ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    XglTestFramework::Finish();
    return result;
}
