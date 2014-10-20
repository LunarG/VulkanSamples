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
#include "xglimage.h"
#include "icd-bil.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "xglrenderframework.h"

#undef ASSERT_NO_FATAL_FAILURE
#define ASSERT_NO_FATAL_FAILURE(x) x

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
struct Vertex
{
    XGL_FLOAT posX, posY, posZ, posW;    // Position data
    XGL_FLOAT r, g, b, a;                // Color
};

#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f

static const Vertex g_vbData[] =
{
    { XYZ1( -1, -1, -1 ), XYZ1( 0.f, 0.f, 0.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },

    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( 1,  1,  1 ), XYZ1( 1.f, 1.f, 1.f ) },

    { XYZ1( 1,  1,  1 ), XYZ1( 1.f, 1.f, 1.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },

    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( -1, -1, -1 ), XYZ1( 0.f, 0.f, 0.f ) },

    { XYZ1( 1,  1,  1 ), XYZ1( 1.f, 1.f, 1.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },

    { XYZ1( 1, -1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( -1, -1, -1 ), XYZ1( 0.f, 0.f, 0.f ) },
};

class XglRenderTest : public XglRenderFramework
{
public:
    void InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
    void InitTexture();
    void InitSampler();
    void DrawTriangleTest(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleTwoUniformsFS(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleWithVertexFetch(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleVSUniform(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleFSUniformBlock(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleVSUniformBlock(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleVSFSUniformBlock(const char *vertShaderText, const char *fragShaderText);
    void DrawTexturedTriangle(const char *vertShaderText, const char *fragShaderText);


    void CreatePipelineWithVertexFetch(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineVSUniform(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineVSFSUniformBlock(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineSingleTextureAndSampler(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void DrawRotatedTriangleTest();


protected:
    XGL_IMAGE m_texture;
    XGL_IMAGE_VIEW m_textureView;
    XGL_IMAGE_VIEW_ATTACH_INFO m_textureViewInfo;
    XGL_GPU_MEMORY m_textureMem;

    XGL_SAMPLER m_sampler;

//    XGL_APPLICATION_INFO app_info;
//    XGL_PHYSICAL_GPU objs[MAX_GPUS];
//    XGL_UINT gpu_count;
//    XGL_GPU_MEMORY      m_descriptor_set_mem;
//    XGL_GPU_MEMORY      m_pipe_mem;
//    XglDevice *m_device;
//    XGL_CMD_BUFFER m_cmdBuffer;
//    XGL_UINT32 m_numVertices;
//    XGL_MEMORY_VIEW_ATTACH_INFO m_vtxBufferView;
//    XGL_MEMORY_VIEW_ATTACH_INFO m_constantBufferView;
//    XGL_GPU_MEMORY m_vtxBufferMem;
//    XGL_GPU_MEMORY m_constantBufferMem;
//    XGL_UINT32                      m_numMemRefs;
//    XGL_MEMORY_REF                  m_memRefs[5];
//    XGL_RASTER_STATE_OBJECT         m_stateRaster;
//    XGL_COLOR_BLEND_STATE_OBJECT    m_colorBlend;
//    XGL_VIEWPORT_STATE_OBJECT       m_stateViewport;
//    XGL_DEPTH_STENCIL_STATE_OBJECT  m_stateDepthStencil;
//    XGL_MSAA_STATE_OBJECT           m_stateMsaa;
//    XGL_DESCRIPTOR_SET              m_rsrcDescSet;

    virtual void SetUp() {

        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = (const XGL_CHAR *) "compiler render_tests";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = (const XGL_CHAR *) "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

        memset(&m_textureViewInfo, 0, sizeof(m_textureViewInfo));
        m_textureViewInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;

        InitFramework();
    }

    virtual void TearDown() {
        // Clean up resources before we reset
        ShutdownFramework();
    }
};

// this function will create the vertex buffer and fill it with the mesh data
void XglRenderTest::InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride,
                              const void* vertices )
{
    XGL_RESULT err = XGL_SUCCESS;

    assert( numVertices * vbStride > 0 );
    m_numVertices = numVertices;

    XGL_MEMORY_ALLOC_INFO alloc_info = {};
    XGL_UINT8 *pData;

    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.allocationSize = numVertices * vbStride;
    alloc_info.alignment = 0;
    alloc_info.heapCount = 1;
    alloc_info.heaps[0] = 0; // TODO: Use known existing heap

    alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(device(), &alloc_info, &m_vtxBufferMem);
    ASSERT_XGL_SUCCESS(err);

    err = xglMapMemory(m_vtxBufferMem, 0, (XGL_VOID **) &pData);
    ASSERT_XGL_SUCCESS(err);

    memcpy(pData, vertices, alloc_info.allocationSize);

    err = xglUnmapMemory(m_vtxBufferMem);
    ASSERT_XGL_SUCCESS(err);

    // set up the memory view for the vertex buffer
    this->m_vtxBufferView.stride = vbStride;
    this->m_vtxBufferView.range  = numVertices * vbStride;
    this->m_vtxBufferView.offset = 0;
    this->m_vtxBufferView.mem    = m_vtxBufferMem;
    this->m_vtxBufferView.format.channelFormat = XGL_CH_FMT_UNDEFINED;
    this->m_vtxBufferView.format.numericFormat = XGL_NUM_FMT_UNDEFINED;

    // open the command buffer
    err = xglBeginCommandBuffer( m_cmdBuffer, 0 );
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_STATE_TRANSITION transition = {};
    transition.mem = m_vtxBufferMem;
    transition.oldState = XGL_MEMORY_STATE_DATA_TRANSFER;
    transition.newState = XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY;
    transition.offset = 0;
    transition.regionSize = numVertices * vbStride;

    // write transition to the command buffer
    xglCmdPrepareMemoryRegions( m_cmdBuffer, 1, &transition );
    this->m_vtxBufferView.state = XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY;

    // finish recording the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS(err);

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 1;
    m_memRefs[0].flags = 0;
    m_memRefs[0].mem = m_vtxBufferMem;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::InitTexture()
{
#define DEMO_TEXTURE_COUNT 1

    const XGL_FORMAT tex_format = { XGL_CH_FMT_B8G8R8A8, XGL_NUM_FMT_UNORM };
    const XGL_INT tex_width = 16;
    const XGL_INT tex_height = 16;
    const uint32_t tex_colors[DEMO_TEXTURE_COUNT][2] = {
        { 0xffff0000, 0xff00ff00 },
    };
    XGL_RESULT err;
    XGL_UINT i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        const XGL_SAMPLER_CREATE_INFO sampler = {
            .sType = XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = XGL_TEX_FILTER_NEAREST,
            .minFilter = XGL_TEX_FILTER_NEAREST,
            .mipMode = XGL_TEX_MIPMAP_BASE,
            .addressU = XGL_TEX_ADDRESS_WRAP,
            .addressV = XGL_TEX_ADDRESS_WRAP,
            .addressW = XGL_TEX_ADDRESS_WRAP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 0,
            .compareFunc = XGL_COMPARE_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColorType = XGL_BORDER_COLOR_OPAQUE_WHITE,
        };
        const XGL_IMAGE_CREATE_INFO image = {
            .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .imageType = XGL_IMAGE_2D,
            .format = tex_format,
            .extent = { tex_width, tex_height, 1 },
            .mipLevels = 1,
            .arraySize = 1,
            .samples = 1,
            .tiling = XGL_LINEAR_TILING,
            .usage = XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT,
            .flags = 0,
        };
        XGL_MEMORY_ALLOC_INFO mem_alloc;
            mem_alloc.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
            mem_alloc.pNext = NULL;
            mem_alloc.allocationSize = 0;
            mem_alloc.alignment = 0;
            mem_alloc.flags = 0;
            mem_alloc.heapCount = 0;
            mem_alloc.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
        XGL_IMAGE_VIEW_CREATE_INFO view;
            view.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view.pNext = NULL;
            view.image = XGL_NULL_HANDLE;
            view.viewType = XGL_IMAGE_VIEW_2D;
            view.format = image.format;
            view.channels.r = XGL_CHANNEL_SWIZZLE_R;
            view.channels.g = XGL_CHANNEL_SWIZZLE_G;
            view.channels.b = XGL_CHANNEL_SWIZZLE_B;
            view.channels.a = XGL_CHANNEL_SWIZZLE_A;
            view.subresourceRange.aspect = XGL_IMAGE_ASPECT_COLOR;
            view.subresourceRange.baseMipLevel = 0;
            view.subresourceRange.mipLevels = 1;
            view.subresourceRange.baseArraySlice = 0;
            view.subresourceRange.arraySize = 1;
            view.minLod = 0.0f;

        XGL_MEMORY_REQUIREMENTS mem_reqs;
        XGL_SIZE mem_reqs_size;

        /* create sampler */
        err = xglCreateSampler(device(), &sampler, &m_sampler);
        assert(!err);

        /* create image */
        err = xglCreateImage(device(), &image, &m_texture);
        assert(!err);

        err = xglGetObjectInfo(m_texture,
                XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                &mem_reqs_size, &mem_reqs);
        assert(!err && mem_reqs_size == sizeof(mem_reqs));

        mem_alloc.allocationSize = mem_reqs.size;
        mem_alloc.alignment = mem_reqs.alignment;
        mem_alloc.heapCount = mem_reqs.heapCount;
        memcpy(mem_alloc.heaps, mem_reqs.heaps,
                sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

        /* allocate memory */
        err = xglAllocMemory(device(), &mem_alloc, &m_textureMem);
        assert(!err);

        /* bind memory */
        err = xglBindObjectMemory(m_texture, m_textureMem, 0);
        assert(!err);

        /* create image view */
        view.image = m_texture;
        err = xglCreateImageView(device(), &view, &m_textureView);
        assert(!err);
    }

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        const XGL_IMAGE_SUBRESOURCE subres = {
            .aspect = XGL_IMAGE_ASPECT_COLOR,
            .mipLevel = 0,
            .arraySlice = 0,
        };
        XGL_SUBRESOURCE_LAYOUT layout;
        XGL_SIZE layout_size;
        XGL_VOID *data;
        XGL_INT x, y;

        err = xglGetImageSubresourceInfo(m_texture, &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));

        err = xglMapMemory(m_textureMem, 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[i][(x & 1) ^ (y & 1)];
        }

        err = xglUnmapMemory(m_textureMem);
        assert(!err);
    }

    m_textureViewInfo.view = m_textureView;
}

void XglRenderTest::InitSampler()
{
    XGL_RESULT err;

    XGL_SAMPLER_CREATE_INFO samplerCreateInfo = {};
    samplerCreateInfo.sType = XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = XGL_TEX_FILTER_NEAREST;
    samplerCreateInfo.minFilter = XGL_TEX_FILTER_NEAREST;
    samplerCreateInfo.mipMode = XGL_TEX_MIPMAP_BASE;
    samplerCreateInfo.addressU = XGL_TEX_ADDRESS_WRAP;
    samplerCreateInfo.addressV = XGL_TEX_ADDRESS_WRAP;
    samplerCreateInfo.addressW = XGL_TEX_ADDRESS_WRAP;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.maxAnisotropy = 0.0;
    samplerCreateInfo.compareFunc = XGL_COMPARE_NEVER;
    samplerCreateInfo.minLod = 0.0;
    samplerCreateInfo.maxLod = 0.0;
    samplerCreateInfo.borderColorType = XGL_BORDER_COLOR_OPAQUE_WHITE;

    err = xglCreateSampler(device(),&samplerCreateInfo, &m_sampler);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::DrawRotatedTriangleTest()
{
    // TODO : This test will pass a matrix into VS to affect triangle orientation.
}

void XglRenderTest::DrawTriangleTest(const char *vertShaderText, const char *fragShaderText)
{
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreateDefaultPipeline(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const int constantCount = 4;
    const float constants[constantCount] = { 0.5, 0.5, 0.5, 1.0 };
    InitConstantBuffer(constantCount, sizeof(constants[0]), (const void*) constants);

    // Create descriptor set for a uniform resource
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = 1;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the constant buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_constantBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

void XglRenderTest::DrawTriangleTwoUniformsFS(const char *vertShaderText, const char *fragShaderText)
{
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreateDefaultPipeline(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const int constantCount = 8;
    const float constants[constantCount] =  { 1.0, 0.0, 0.0, 1.0,
                                              0.0, 0.0, 1.0, 1.0 };

    InitConstantBuffer(constantCount, sizeof(constants[0]), (const void*) constants);

    // Create descriptor set for a uniform resource
    const int slotCount = 1;
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = slotCount;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the constant buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_constantBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

void XglRenderTest::DrawTexturedTriangle(const char *vertShaderText, const char *fragShaderText)
{

    // based on DrawTriangleTwoUniformsFS

    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreatePipelineSingleTextureAndSampler(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());


    // Enable our single sampler
    ASSERT_NO_FATAL_FAILURE(InitSampler());

    // Enable our single texture
    ASSERT_NO_FATAL_FAILURE(InitTexture());

    // Create descriptor set for a texture and sampler resources
    const int slotCount = 3;
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = slotCount;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the sampler and image views to the descriptor set
    // ensure this matches order set in CreatePipelineSingleTextureAndSampler
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachImageViewDescriptors( m_rsrcDescSet, 0, 1, &m_textureViewInfo );
    xglAttachSamplerDescriptors(m_rsrcDescSet, 1, 1, &m_sampler);
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

void XglRenderTest::DrawTriangleVSUniform(const char *vertShaderText, const char *fragShaderText)
{
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;
    glm::mat4 MVP;
    int i;

    // Create identity matrix
    glm::mat4 Model      = glm::mat4(1.0f);

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreatePipelineVSUniform(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const int matrixSize = 16;
    MVP = Model;

    InitConstantBuffer(matrixSize, sizeof(MVP[0]), (const void*) &MVP[0][0]);

    // Create descriptor set for a uniform resource
    const int slotCount = 1;
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = slotCount;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the constant buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_constantBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

    for (i = 0; i < 8; i++) {
        XGL_UINT8 *pData;
        err = xglMapMemory(m_constantBufferMem, 0, (XGL_VOID **) &pData);
        ASSERT_XGL_SUCCESS(err);

        MVP = glm::rotate(MVP, glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
        memcpy(pData, (const void*) &MVP[0][0], matrixSize);

        err = xglUnmapMemory(m_constantBufferMem);
        ASSERT_XGL_SUCCESS(err);

        // submit the command buffer to the universal queue
        err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
        ASSERT_XGL_SUCCESS( err );

        err = xglQueueWaitIdle( m_device->m_queue );
        ASSERT_XGL_SUCCESS( err );

        // Wait for work to finish before cleaning up.
        xglDeviceWaitIdle(m_device->device());

        RecordImage(m_renderTarget);
    }
}

void XglRenderTest::DrawTriangleVSUniformBlock(const char *vertShaderText, const char *fragShaderText)
{
    // sourced from DrawTriangleVSUniform
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreatePipelineVSUniform(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Let's populate our buffer with the following:
    //     vec4 red;
    //     vec4 green;
    //     vec4 blue;
    //     vec4 white;
    const int valCount = 4 * 4;
    const float bufferVals[valCount] = { 1.0, 0.0, 0.0, 1.0,
                                         0.0, 1.0, 0.0, 1.0,
                                         0.0, 0.0, 1.0, 1.0,
                                         1.0, 1.0, 1.0, 1.0 };

    InitConstantBuffer(valCount, sizeof(bufferVals[0]), (const void*) bufferVals);

    // Create descriptor set for a uniform resource
    const int slotCount = 1;
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = slotCount;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the constant buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_constantBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);
}

void XglRenderTest::CreatePipelineWithVertexFetch(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps)
{
    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;


    // Create descriptor set for our one resource
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = 1; // Vertex buffer only

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the vertex buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_vtxBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    const int slots = 1;
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( slots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    slotInfo[0].shaderEntityIndex = 0;
    slotInfo[0].slotObjectType = XGL_SLOT_VERTEX_INPUT;

    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = vs;
    vs_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = slots;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = ps;
    ps_stage.shader.descriptorSetMapping[0].descriptorCount = 0;
    ps_stage.shader.linkConstBufferCount = 0;
    ps_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    ps_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    ps_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    XGL_VERTEX_INPUT_BINDING_DESCRIPTION vi_binding = {
        sizeof(g_vbData[0]),              // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        XGL_VERTEX_INPUT_STEP_RATE_VERTEX // stepRate;       // Rate at which binding is incremented
    };

    // this is the current description of g_vbData
    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION vi_attribs[2];
    vi_attribs[0].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[0].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[0].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[0].offsetInBytes = 0;                 // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[1].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[1].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[1].offsetInBytes = 16;                 // Offset of first element in bytes from base of vertex

    XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO vi_state = {
        XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO, // sType;
        &ps_stage,                                            // pNext;
        1,                                                    // bindingCount
        &vi_binding,                                          // pVertexBindingDescriptions;
        2,                                                    // attributeCount; // number of attributes
        vi_attribs                                            // pVertexAttributeDescriptions;
    };

    XGL_PIPELINE_IA_STATE_CREATE_INFO ia_state = {
        XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO,  // sType
        &vi_state,                                         // pNext
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
                m_render_target_fmt,                        // XGL_FORMAT
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
    err = xglCreateGraphicsPipeline(device(), &info, pipeline);
    ASSERT_XGL_SUCCESS(err);

    err = m_device->AllocAndBindGpuMemory(*pipeline, "Pipeline", &m_pipe_mem);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::CreatePipelineVSUniform(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps)
{
    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;


    const int vsSlots = 1; // Uniform buffer only

    // Create descriptor set for our one resource
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = vsSlots;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);


    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( vsSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    slotInfo[0].shaderEntityIndex = 0;
    slotInfo[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;

    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = vs;
    vs_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = vsSlots;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = ps;
    ps_stage.shader.descriptorSetMapping[0].descriptorCount = 0;
    ps_stage.shader.linkConstBufferCount = 0;
    ps_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    ps_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
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
                m_render_target_fmt,                        // XGL_FORMAT
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
    err = xglCreateGraphicsPipeline(device(), &info, pipeline);
    ASSERT_XGL_SUCCESS(err);

    err = m_device->AllocAndBindGpuMemory(*pipeline, "Pipeline", &m_pipe_mem);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::CreatePipelineVSFSUniformBlock(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps)
{
    // this is based on CreatePipelineVSUniform

    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;


    const int slots = 1; // Uniform buffer only

    // Create descriptor set for our one resource
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = slots;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);


    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( slots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    slotInfo[0].shaderEntityIndex = 0;
    slotInfo[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;

    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = vs;
    vs_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = slots;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = ps;
    ps_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    ps_stage.shader.descriptorSetMapping[0].descriptorCount = slots;
    ps_stage.shader.linkConstBufferCount = 0;
    ps_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    ps_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
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
                m_render_target_fmt,                        // XGL_FORMAT
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
    err = xglCreateGraphicsPipeline(device(), &info, pipeline);
    ASSERT_XGL_SUCCESS(err);

    err = m_device->AllocAndBindGpuMemory(*pipeline, "Pipeline", &m_pipe_mem);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::CreatePipelineSingleTextureAndSampler(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps)
{
    // based on CreatePipelineVSUniform

    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;

    const int psSlots = 2; // One texture, one sampler

    // Create descriptor set for single texture and sampler
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = psSlots;
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

//XXXXXX
    // Texture must be first, followed by sampler (VALIDATED THIS BEFORE SUBMITTING COMMENT)
//XXXXXX

    // Assign the slots, note that only t0 and s0 will work as of writing this test
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( psSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    slotInfo[0].shaderEntityIndex = 0;
    slotInfo[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    slotInfo[1].shaderEntityIndex = 0;
    slotInfo[1].slotObjectType = XGL_SLOT_SHADER_SAMPLER;

    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = vs;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = 0;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = ps;
    ps_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    ps_stage.shader.descriptorSetMapping[0].descriptorCount = psSlots;
    ps_stage.shader.linkConstBufferCount = 0;
    ps_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    ps_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
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
                m_render_target_fmt,                        // XGL_FORMAT
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
    err = xglCreateGraphicsPipeline(device(), &info, pipeline);
    ASSERT_XGL_SUCCESS(err);

    err = m_device->AllocAndBindGpuMemory(*pipeline, "Pipeline", &m_pipe_mem);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::DrawTriangleWithVertexFetch(const char *vertShaderText, const char *fragShaderText)
{
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitMesh(sizeof(g_vbData)/sizeof(g_vbData[0]), sizeof(g_vbData[0]), g_vbData));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreatePipelineWithVertexFetch(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 6, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

void XglRenderTest::DrawTriangleFSUniformBlock(const char *vertShaderText, const char *fragShaderText)
{
    // probably sourced from DrawTriangleTwoUniformsFS

    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreateDefaultPipeline(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());


    // Let's populate our buffer with the following:
    //     vec4 red;
    //     vec4 green;
    //     vec4 blue;
    //     vec4 white;
    const int valCount = 4 * 4;
    const float bufferVals[valCount] = { 1.0, 0.0, 0.0, 1.0,
                                         0.0, 1.0, 0.0, 1.0,
                                         0.0, 0.0, 1.0, 1.0,
                                         1.0, 1.0, 1.0, 1.0 };

    InitConstantBuffer(valCount, sizeof(bufferVals[0]), (const void*) bufferVals);

    // Create descriptor set for a uniform resource
    const int slotCount = 1;
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = slotCount;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the constant buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_constantBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

void XglRenderTest::DrawTriangleVSFSUniformBlock(const char *vertShaderText, const char *fragShaderText)
{
    // this is sourced from DrawTriangleFSUniformBlock

    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreatePipelineVSFSUniformBlock(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());


    // Let's populate our buffer with the following:
    //     vec4 red;
    //     vec4 green;
    //     vec4 blue;
    //     vec4 white;
    const int valCount = 4 * 4;
    const float bufferVals[valCount] = { 1.0, 0.0, 0.0, 1.0,
                                         0.0, 1.0, 0.0, 1.0,
                                         0.0, 0.0, 1.0, 1.0,
                                         1.0, 1.0, 1.0, 1.0 };

    InitConstantBuffer(valCount, sizeof(bufferVals[0]), (const void*) bufferVals);

    // Create descriptor set for a uniform resource
    const int slotCount = 1;
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = slotCount;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the constant buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_constantBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // prepare the back buffer for present
//    XGL_IMAGE_STATE_TRANSITION transitionToPresent = {};
//    transitionToPresent.image = m_image;
//    transitionToPresent.oldState = m_image_state;
//    transitionToPresent.newState = m_display.fullscreen ? XGL_WSI_WIN_PRESENT_SOURCE_FLIP : XGL_WSI_WIN_PRESENT_SOURCE_BLT;
//    transitionToPresent.subresourceRange = srRange;
//    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToPresent );
//    m_image_state = ( XGL_IMAGE_STATE ) transitionToPresent.newState;

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;
//    m_memRefs[0].flags = 0;
//    m_memRefs[0].mem = m_vtxBufferMemory;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

TEST_F(XglRenderTest, GreenTriangle)
{
    static const char *vertShaderText =
            "#version 130\n"
            "vec2 vertices[3];\n"
            "void main() {\n"
            "      vertices[0] = vec2(-1.0, -1.0);\n"
            "      vertices[1] = vec2( 1.0, -1.0);\n"
            "      vertices[2] = vec2( 0.0,  1.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
       "#version 130\n"
       "void main() {\n"
       "   gl_FragColor = vec4(0,1,0,1);\n"
       "}\n";
    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, BIL_GreenTriangle)
{
    bool saved_use_bil = XglTestFramework::m_use_bil;

    static const char *vertShaderText =
            "#version 130\n"
            "vec2 vertices[3];\n"
            "void main() {\n"
            "      vertices[0] = vec2(-1.0, -1.0);\n"
            "      vertices[1] = vec2( 1.0, -1.0);\n"
            "      vertices[2] = vec2( 0.0,  1.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
       "#version 130\n"
       "void main() {\n"
       "   gl_FragColor = vec4(0,1,0,1);\n"
       "}\n";
    XglTestFramework::m_use_bil = true;
    DrawTriangleTest(vertShaderText, fragShaderText);
    XglTestFramework::m_use_bil = saved_use_bil;
}

TEST_F(XglRenderTest, TriangleFragUniform)
{

    static const char *vertShaderText =
            "#version 130\n"
            "out vec4 color;\n"
            "out vec4 scale;\n"
            "vec2 vertices[3];\n"
            "void main() {\n"
            "vec2 vertices[3];\n"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "vec4 colors[3];\n"
            "      colors[0] = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "      colors[1] = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "      colors[2] = vec4(0.0, 0.0, 1.0, 1.0);\n"
            "   color = colors[gl_VertexID % 3];\n"
            "   scale = vec4(1.0, 1.0, 1.0, 1.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 130\n"
            "in vec4 color;\n"
            "in vec4 scale;\n"
            "uniform vec4 foo;\n"
            "void main() {\n"
            "   gl_FragColor = color * scale + foo;\n"
            "}\n";

    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, YellowTriangle)
{
    static const char *vertShaderText =
            "#version 130\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   vec4 colors[3];\n"
            "      colors[0] = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "      colors[1] = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "      colors[2] = vec4(0.0, 0.0, 1.0, 1.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 130\n"
            "void main() {\n"
            "  gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
            "}\n";

    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, RotatedTriangle) {
    DrawRotatedTriangleTest();
}

TEST_F(XglRenderTest, TriangleTwoFSUniforms)
{
    static const char *vertShaderText =
            "#version 130\n"
            "out vec4 color;\n"
            "out vec4 scale;\n"
            "out vec2 samplePos;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   vec4 colors[3];\n"
            "      colors[0] = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "      colors[1] = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "      colors[2] = vec4(0.0, 0.0, 1.0, 1.0);\n"
            "   color = colors[gl_VertexID % 3];\n"
            "   vec2 positions[3];"
            "      positions[0] = vec2( 0.0, 0.0);\n"
            "      positions[1] = vec2( 1.0, 0.0);\n"
            "      positions[2] = vec2( 1.0, 1.0);\n"
            "   scale = vec4(0.0, 0.0, 0.0, 0.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";


    static const char *fragShaderText =
            "#version 430\n"
            "in vec4 color;\n"
            "in vec4 scale;\n"
            "uniform vec4 foo;\n"
            "uniform vec4 bar;\n"
            "void main() {\n"
            // by default, with no location or blocks
            // the compiler will read them from buffer
            // in reverse order of first use in shader
            // The buffer contains red, followed by blue,
            // so foo should be blue, bar should be red
            "   gl_FragColor = color * scale * foo * bar + foo;\n"
            "}\n";

    DrawTriangleTwoUniformsFS(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleWithVertexFetch)
{
    static const char *vertShaderText =
            "#version 130\n"
            //XYZ1( -1, -1, -1 )
            "in vec4 pos;\n"
            //XYZ1( 0.f, 0.f, 0.f )
            "in vec4 inColor;\n"
            "out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   gl_Position = pos;\n"
            "}\n";


    static const char *fragShaderText =
            "#version 430\n"
            "in vec4 color;\n"
            "void main() {\n"
            "   gl_FragColor = color;\n"
            "}\n";

    DrawTriangleWithVertexFetch(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleVSUniform)
{
    static const char *vertShaderText =
            "#version 130\n"
            "uniform mat4 mvp;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0) * mvp;\n"
            "}\n";

    static const char *fragShaderText =
            "#version 430\n"
            "void main() {\n"
            "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "}\n";

    // Create identity matrix
    glm::mat4 Model      = glm::mat4(1.0f);
    DrawTriangleVSUniform(vertShaderText, fragShaderText);

//    Model = glm::rotate(Model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//    DrawTriangleVSUniform(vertShaderText, fragShaderText, Model);
}

TEST_F(XglRenderTest, TriangleFSUniformBlock)
{
    // The expected result from this test is a blue triangle

    static const char *vertShaderText =
            "#version 130\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 430\n"
            "layout (std140) uniform bufferVals {\n"
            "    vec4 red;\n"
            "    vec4 green;\n"
            "    vec4 blue;\n"
            "    vec4 white;\n"
            "} myBufferVals;\n"
            "void main() {\n"
            "   gl_FragColor = myBufferVals.blue;\n"
            "}\n";

    DrawTriangleFSUniformBlock(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleVSUniformBlock)
{
    // The expected result from this test is a blue triangle

    static const char *vertShaderText =
            "#version 140\n"
            "out vec4 outColor;\n"
            "layout (std140) uniform bufferVals {\n"
            "    vec4 red;\n"
            "    vec4 green;\n"
            "    vec4 blue;\n"
            "    vec4 white;\n"
            "} myBufferVals;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   outColor = myBufferVals.blue;\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 430\n"
            "in vec4 inColor;\n"
            "void main() {\n"
            "   gl_FragColor = inColor;\n"
            "}\n";

    DrawTriangleVSUniformBlock(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleVSFSUniformBlock)
{
    // The expected result from this test is a green triangle
    // Note the buffer is shared between stages, idenitical layout

    static const char *vertShaderText =
            "#version 140\n"
            "out vec4 outRed;\n"
            "out vec4 outGreen;\n"
            "out vec4 outBlue;\n"
            "out vec4 outWhite;\n"
            "layout (std140) uniform bufferVals {\n"
            "    vec4 red;\n"
            "    vec4 green;\n"
            "    vec4 blue;\n"
            "    vec4 white;\n"
            "} myBufferVals;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   outRed   = myBufferVals.red;\n"
            "   outGreen = myBufferVals.green;\n"
            "   outBlue  = myBufferVals.blue;\n"
            "   outWhite = myBufferVals.white;\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 430\n"
            "in vec4 inRed;\n"
            "in vec4 inGreen;\n"
            "in vec4 inBlue;\n"
            "in vec4 inWhite;\n"
            "layout (std140) uniform bufferVals {\n"
            "    vec4 red;\n"
            "    vec4 green;\n"
            "    vec4 blue;\n"
            "    vec4 white;\n"
            "} myBufferVals;\n"
            "void main() {\n"
            "   if (inRed   == myBufferVals.red   && \n"
            "       inGreen == myBufferVals.green && \n"
            "       inBlue  == myBufferVals.blue  && \n"
            "       inWhite == myBufferVals.white)   \n"
            "       gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "   else\n"
            "       gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "}\n";

    DrawTriangleVSFSUniformBlock(vertShaderText, fragShaderText);
 }


TEST_F(XglRenderTest, TexturedTriangle)
{
    static const char *vertShaderText =
            "#version 130\n"
            "out vec2 samplePos;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   vec2 positions[3];"
            "      positions[0] = vec2( 0.0, 0.0);\n"
            "      positions[1] = vec2( 1.0, 0.0);\n"
            "      positions[2] = vec2( 1.0, 1.0);\n"
            "   samplePos = positions[gl_VertexID % 3];\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 130\n"
            "in vec2 samplePos;\n"
            "uniform sampler2D surface;\n"
            "void main() {\n"
            "   vec4 texColor = textureLod(surface, samplePos, 0.0);\n"
            "   gl_FragColor = texColor;\n"
            "}\n";
    DrawTexturedTriangle(vertShaderText, fragShaderText);
}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    XglTestFramework::InitArgs(&argc, argv);

    ::testing::Environment* const xgl_test_env = ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    XglTestFramework::Finish();
    return result;
}
