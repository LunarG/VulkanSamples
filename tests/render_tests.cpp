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


// Basic rendering tests

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

#include <xgl.h>
#include "gtest-1.7.0/include/gtest/gtest.h"

#include "xgldevice.h"
#include "shader_il.h"

class XglRenderTest : public ::testing::Test {
public:
    void CreateImage(XGL_UINT w, XGL_UINT h);
    void DestroyImage();

    void CreateImageView(XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
                         XGL_IMAGE_VIEW* pView);
    void DestroyImageView(XGL_IMAGE_VIEW imageView);
    XGL_DEVICE device() {return m_device->device();}
    void CreateShader(const char *filename, XGL_SHADER *pshader);
    void InitPipeline();

protected:
    XGL_APPLICATION_INFO app_info;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count;
    XGL_IMAGE m_image;
    XGL_GPU_MEMORY m_image_mem;
    XglDevice *m_device;
    XGL_CMD_BUFFER m_cmdBuffer;

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

        m_device = new XglDevice(0, objs[0]);
        m_device->get_device_queue();

        XGL_CMD_BUFFER_CREATE_INFO info = {};

        info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
        info.queueType = XGL_QUEUE_TYPE_GRAPHICS;
        err = xglCreateCommandBuffer(device(), &info, &m_cmdBuffer);
        ASSERT_XGL_SUCCESS(err) << "xglCreateCommandBuffer failed";
    }

    virtual void TearDown() {
        xglInitAndEnumerateGpus(&this->app_info, XGL_NULL_HANDLE, 0, &gpu_count, XGL_NULL_HANDLE);
    }
};


void XglRenderTest::CreateImage(XGL_UINT w, XGL_UINT h)
{
    XGL_RESULT err;
    XGL_IMAGE image;
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
    imageCreateInfo.usage = XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT | XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //    XGL_RESULT XGLAPI xglCreateImage(
    //        XGL_DEVICE                                  device,
    //        const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    //        XGL_IMAGE*                                  pImage);
    err = xglCreateImage(device(), &imageCreateInfo, &m_image);
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

    err = xglBindObjectMemory(image, m_image_mem, 0);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::DestroyImage()
{
    // All done with image memory, clean up
    ASSERT_XGL_SUCCESS(xglBindObjectMemory(m_image, XGL_NULL_HANDLE, 0));

    ASSERT_XGL_SUCCESS(xglFreeMemory(m_image_mem));

    ASSERT_XGL_SUCCESS(xglDestroyObject(m_image));
}

void XglRenderTest::CreateImageView(XGL_IMAGE_VIEW_CREATE_INFO *pCreateInfo,
                                   XGL_IMAGE_VIEW *pView)
{
    pCreateInfo->image = this->m_image;
    ASSERT_XGL_SUCCESS(xglCreateImageView(device(), pCreateInfo, pView));
}

void XglRenderTest::DestroyImageView(XGL_IMAGE_VIEW imageView)
{
    ASSERT_XGL_SUCCESS(xglDestroyObject(imageView));
}

void XglRenderTest::CreateShader(const char *filename, XGL_SHADER *pshader)
{
    struct bil_header *pBIL;
    streampos size;
    char * memblock;
    XGL_RESULT err;

    //    codeSize = sizeof(struct bil_header) + 100;
    //    code = malloc(codeSize);
    //    ASSERT_TRUE(NULL != code) << "malloc failed!";

    //    memset(code, 0, codeSize);

    //    // Indicate that this is BIL data.
    //    pBIL = (struct bil_header *) code;
    //    pBIL->bil_magic = BILMagicNumber;
    //    pBIL->bil_version = BILVersion;

    // reading an entire binary file
    ifstream file (filename, ios::in|ios::binary|ios::ate);
        ASSERT_TRUE(file.is_open()) << "Unable to open file: " << filename;

//    if (file.is_open()) {
//    ASSERT_TRUE(file.is_open() == true);
        size = file.tellg();
        memblock = new char [size];
        ASSERT_TRUE(memblock != NULL) << "memory allocation failed";

        file.seekg (0, ios::beg);
        file.read (memblock, size);
        file.close();

        XGL_SHADER_CREATE_INFO createInfo;
        XGL_SHADER shader;

        createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
        createInfo.pNext = NULL;
        createInfo.pCode = memblock;
        createInfo.codeSize = size;
        createInfo.flags = 0;
        err = xglCreateShader(device(), &createInfo, &shader);
        ASSERT_XGL_SUCCESS(err);

        delete[] memblock;

        *pshader = shader;
//    }
}

TEST_F(XglRenderTest, DrawTriangleTest) {
    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_SHADER vs, ps;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;
    XGL_PIPELINE pipeline;

    /*
     * Define descriptor slots for vertex shader.
     */
    XGL_DESCRIPTOR_SLOT_INFO ds_vs = {
        XGL_SLOT_SHADER_RESOURCE,    // XGL_DESCRIPTOR_SET_SLOT_TYPE
        1                            // shaderEntityIndex
    };

    CreateShader("vs-kernel.bin", &vs);

    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = vs;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = 1;
    vs_stage.shader.descriptorSetMapping[0].pDescriptorInfo = &ds_vs;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    CreateShader("wm-kernel.bin", &ps);

    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = ps;
    ps_stage.shader.descriptorSetMapping[0].descriptorCount = 1;
    // TODO: Do we need a descriptor set mapping for fragment?
    ps_stage.shader.descriptorSetMapping[0].pDescriptorInfo = &ds_vs;
    ps_stage.shader.linkConstBufferCount = 0;
    ps_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    ps_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    ps_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    XGL_PIPELINE_IA_STATE_CREATE_INFO ia_state = {
        XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO,  // sType
        &ps_stage,                                         // pNext
        XGL_TOPOLOGY_TRIANGLE_LIST,                        // XGL_PRIMITIVE_TOPOLOGY
        XGL_FALSE,                                         // disableVertexReuse
        XGL_PROVOKING_VERTEX_LAST,                         // XGL_PROVOKING_VERTEX_CONVENTION
        XGL_FALSE,                                         // primitiveRestartEnable
        0                                                  // primitiveRestartIndex
    };

    XGL_PIPELINE_RS_STATE_CREATE_INFO rs_state = {
        XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO,
        &ia_state,
        XGL_FALSE,                                          // depthClipEnable
        XGL_FALSE,                                          // rasterizerDiscardEnable
        1.0                                                 // pointSize
    };

    XGL_PIPELINE_CB_STATE cb_state = {
        XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO,
        &rs_state,
        XGL_FALSE,                                          // alphaToCoverageEnable
        XGL_FALSE,                                          // dualSourceBlendEnable
        XGL_LOGIC_OP_COPY,                                  // XGL_LOGIC_OP
        {                                                   // XGL_PIPELINE_CB_ATTACHMENT_STATE
            {
                XGL_FALSE,                                  // blendEnable
                {XGL_CH_FMT_R8G8B8A8, XGL_NUM_FMT_UINT},    // XGL_FORMAT
                0xF                                         // channelWriteMask
            }
        }
    };

    // TODO: Should take depth buffer format from queried formats
    XGL_PIPELINE_DB_STATE_CREATE_INFO db_state = {
        XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO,
        &cb_state,
        {XGL_CH_FMT_R32, XGL_NUM_FMT_DS}                    // XGL_FORMAT
    };

    info.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = &db_state;
    info.flags = 0;
    err = xglCreateGraphicsPipeline(device(), &info, &pipeline);
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_UINT data_size;
    err = xglGetObjectInfo(pipeline, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(data_size, sizeof(mem_req));
    ASSERT_NE(0, mem_req.size) << "xglGetObjectInfo (Pipeline): Failed - expect pipeline to require memory";

    //        XGL_RESULT XGLAPI xglAllocMemory(
    //            XGL_DEVICE                                  device,
    //            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    //            XGL_GPU_MEMORY*                             pMem);

//    typedef struct _XGL_MEMORY_ALLOC_INFO
//    {
//        XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO
//        XGL_VOID*                               pNext;                      // Pointer to next structure
//        XGL_GPU_SIZE                            allocationSize;             // Size of memory allocation
//        XGL_GPU_SIZE                            alignment;
//        XGL_FLAGS                               flags;                      // XGL_MEMORY_ALLOC_FLAGS
//        XGL_UINT                                heapCount;
//        XGL_UINT                                heaps[XGL_MAX_MEMORY_HEAPS];
//        XGL_MEMORY_PRIORITY                     memPriority;
//    } XGL_MEMORY_ALLOC_INFO;
    XGL_MEMORY_ALLOC_INFO mem_info = {
        XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        XGL_NULL_HANDLE,
        mem_req.size,                                   // allocationSize
        mem_req.alignment,                              // alignment
        XGL_MEMORY_ALLOC_SHAREABLE_BIT,                 // XGL_MEMORY_ALLOC_FLAGS
        mem_req.heapCount,                              // heapCount
        {0},                                  // heaps
        XGL_MEMORY_PRIORITY_NORMAL                      // XGL_MEMORY_PRIORITY
    };

    memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);

    err = xglAllocMemory(device(), &mem_info, &m_image_mem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(pipeline, m_image_mem, 0);
    ASSERT_XGL_SUCCESS(err);

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_XGL_SUCCESS(xglDestroyObject(pipeline));

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
