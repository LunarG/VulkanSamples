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

// These values are used in a few places
static const int g_UniformBufferCount = 16;
static const int g_SamplerCount = 3;
static const int g_TextureCount = 3;

class XglRenderTest : public XglRenderFramework
{
public:
    void InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
    void InitTexture(int textureSlot = 0, int* color = 0);
    void InitMultipleTextures(int textureCount, int* colors);
    void InitSampler(int samplerSlot = 0);
    void InitMultipleSamplers(int samplerCount);
    void InitUniformBuffer(int constantCount, int constantSize, int constantIndex, const void* data);
    void DrawTriangleTest(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleWithVertexFetch(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleFSUniformBlockBinding(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleVSUniformBlock(const char *vertShaderText, const char *fragShaderText);
    void DrawTexturedTriangle(const char *vertShaderText, const char *fragShaderText);
    void DrawVSTexture(const char *vertShaderText, const char *fragShaderText);
    void DrawSamplerBindingsTriangle(const char *vertShaderText, const char *fragShaderText, int textureCount, int samplerCount);


    void CreatePipelineWithVertexFetch(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineFSUniformBlockBinding(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps, int bufferCount);
    void CreatePipelineVSUniform(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineSingleTextureAndSampler(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineMultipleTexturesAndSamplers(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps, int textureCount, int samplerCount);

protected:
    XGL_IMAGE m_texture[g_TextureCount];
    XGL_IMAGE_VIEW m_textureView[g_TextureCount];
    XGL_IMAGE_VIEW_ATTACH_INFO m_textureViewInfo[g_TextureCount];
    XGL_GPU_MEMORY m_textureMem[g_TextureCount];


    XGL_SAMPLER m_sampler[g_SamplerCount];

    XGL_GPU_MEMORY m_uniformBufferMem[g_UniformBufferCount];
    XGL_MEMORY_VIEW_ATTACH_INFO     m_uniformBufferView[g_UniformBufferCount];



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

        for (int i = 0; i < g_TextureCount; ++i) {
            m_textureViewInfo[i].sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
        }

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

void XglRenderTest::InitTexture(int textureSlot, int* color)
{
#define DEMO_TEXTURE_COUNT 1

    const XGL_FORMAT tex_format = { XGL_CH_FMT_R8G8B8A8, XGL_NUM_FMT_UNORM };
    const XGL_INT tex_width = 16;
    const XGL_INT tex_height = 16;
    uint32_t tex_colors[DEMO_TEXTURE_COUNT][2];

    // assign the texture color with parameter
    assert(1 == DEMO_TEXTURE_COUNT);
    if (color != NULL) {
        tex_colors[0][0] = *color;
        tex_colors[0][1] = *color;
    } else {
        tex_colors[0][0] = 0xff0000ff;
        tex_colors[0][1] = 0xff00ff00;
    }

    XGL_RESULT err;
    XGL_UINT i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
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

        /* create image */
        err = xglCreateImage(device(), &image, &m_texture[textureSlot]);
        assert(!err);

        err = xglGetObjectInfo(m_texture[textureSlot],
                XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                &mem_reqs_size, &mem_reqs);
        assert(!err && mem_reqs_size == sizeof(mem_reqs));

        mem_alloc.allocationSize = mem_reqs.size;
        mem_alloc.alignment = mem_reqs.alignment;
        mem_alloc.heapCount = mem_reqs.heapCount;
        memcpy(mem_alloc.heaps, mem_reqs.heaps,
                sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

        /* allocate memory */
        err = xglAllocMemory(device(), &mem_alloc, &m_textureMem[textureSlot]);
        assert(!err);

        /* bind memory */
        err = xglBindObjectMemory(m_texture[textureSlot], m_textureMem[textureSlot], 0);
        assert(!err);

        /* create image view */
        view.image = m_texture[textureSlot];
        err = xglCreateImageView(device(), &view, &m_textureView[textureSlot]);
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

        err = xglGetImageSubresourceInfo(m_texture[textureSlot], &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));

        err = xglMapMemory(m_textureMem[textureSlot], 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[i][(x & 1) ^ (y & 1)];
        }

        err = xglUnmapMemory(m_textureMem[textureSlot]);
        assert(!err);
    }

    m_textureViewInfo[textureSlot].view = m_textureView[textureSlot];
}


void XglRenderTest::InitMultipleTextures(int textureCount, int* colors)
{

    for (int i = 0; i < textureCount; ++i)
        InitTexture(i, &colors[i]);
}


void XglRenderTest::InitMultipleSamplers(const int samplerCount)
{
    XGL_RESULT err;

    for (int i = 0; i < samplerCount; ++i) {

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

        err = xglCreateSampler(device(),&samplerCreateInfo, &m_sampler[i]);
        ASSERT_XGL_SUCCESS(err);
    }
}

void XglRenderTest::InitSampler(int samplerSlot)
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

    err = xglCreateSampler(device(),&samplerCreateInfo, &m_sampler[samplerSlot]);
    ASSERT_XGL_SUCCESS(err);
}


void XglRenderTest::InitUniformBuffer(int constantCount, int constantSize,
                                      int constantIndex, const void* data)
{
    // based on XglRenderFramework::InitConstantBuffer
    // mainly add an index when selecting which buffer you are creating

    XGL_RESULT err = XGL_SUCCESS;

    XGL_MEMORY_ALLOC_INFO alloc_info = {};
    XGL_UINT8 *pData;

    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.allocationSize = constantCount * constantSize;
    alloc_info.alignment = 0;
    alloc_info.heapCount = 1;
    alloc_info.heaps[0] = 0; // TODO: Use known existing heap

    alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(device(), &alloc_info, &m_uniformBufferMem[constantIndex]);
    ASSERT_XGL_SUCCESS(err);

    err = xglMapMemory(m_uniformBufferMem[constantIndex], 0, (XGL_VOID **) &pData);
    ASSERT_XGL_SUCCESS(err);

    memcpy(pData, data, alloc_info.allocationSize);

    err = xglUnmapMemory(m_uniformBufferMem[constantIndex]);
    ASSERT_XGL_SUCCESS(err);

    // set up the memory view for the constant buffer
    this->m_uniformBufferView[constantIndex].stride = 16;
    this->m_uniformBufferView[constantIndex].range  = alloc_info.allocationSize;
    this->m_uniformBufferView[constantIndex].offset = 0;
    this->m_uniformBufferView[constantIndex].mem    = m_uniformBufferMem[constantIndex];
    this->m_uniformBufferView[constantIndex].format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    this->m_uniformBufferView[constantIndex].format.numericFormat = XGL_NUM_FMT_FLOAT;
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
    const int slotCount = 2;
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
    xglAttachImageViewDescriptors( m_rsrcDescSet, 0, 1, &m_textureViewInfo[0] );
    xglAttachSamplerDescriptors(m_rsrcDescSet, 1, 1, &m_sampler[0]);
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

void XglRenderTest::CreatePipelineFSUniformBlockBinding(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps, const int bufferCount)
{
    // based on CreateDefaultPipeline
    // only difference is number of constant buffers

    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;

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

//    const int slots = 4;
//    assert (slots == bufferCount); // update as needed

    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( bufferCount * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    for (int i = 0; i < bufferCount; ++i) {
        // Note:  These are all constant buffers
        slotInfo[i].shaderEntityIndex = i;
        slotInfo[i].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    }

    ps_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    ps_stage.shader.descriptorSetMapping[0].descriptorCount = bufferCount;

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

    const int vsSlots = 2; // One texture, one sampler

    // Create descriptor set for single texture and sampler
    XGL_DESCRIPTOR_SET_CREATE_INFO vsDescriptorInfo = {};
    vsDescriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    vsDescriptorInfo.slots = vsSlots;
    err = xglCreateDescriptorSet( device(), &vsDescriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // Assign the slots, note that only t0 and s0 will work as of writing this test
    XGL_DESCRIPTOR_SLOT_INFO *vsSlotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( vsSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    vsSlotInfo[0].shaderEntityIndex = 0;
    vsSlotInfo[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    vsSlotInfo[1].shaderEntityIndex = 0;
    vsSlotInfo[1].slotObjectType = XGL_SLOT_SHADER_SAMPLER;

    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = vs;
    vs_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) vsSlotInfo;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = vsSlots;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    const int psSlots = 2; // One texture, one sampler

    // Create descriptor set for single texture and sampler
    XGL_DESCRIPTOR_SET_CREATE_INFO psDescriptorInfo = {};
    psDescriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    psDescriptorInfo.slots = psSlots;
    err = xglCreateDescriptorSet( device(), &psDescriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // Assign the slots, note that only t0 and s0 will work as of writing this test
    XGL_DESCRIPTOR_SLOT_INFO *psSlotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( psSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    psSlotInfo[0].shaderEntityIndex = 0;
    psSlotInfo[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    psSlotInfo[1].shaderEntityIndex = 0;
    psSlotInfo[1].slotObjectType = XGL_SLOT_SHADER_SAMPLER;

    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = ps;
    ps_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) psSlotInfo;
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

void XglRenderTest::CreatePipelineMultipleTexturesAndSamplers(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps,
                                                         int textureCount, int samplerCount)
{
    // based on CreatePipelineSingleTextureAndSampler

    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;

    const int psSlots = textureCount + samplerCount;

    // Create descriptor set for single texture and sampler
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = psSlots;
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // Assign the slots, note that only t0 and s0 will work as of writing this test
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( psSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    int slotIndex = 0;
    for (int i = 0; i < textureCount; ++i) {
        slotInfo[slotIndex].shaderEntityIndex = i;
        slotInfo[slotIndex++].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    }
    for (int i = 0; i < samplerCount; ++i) {
        slotInfo[slotIndex].shaderEntityIndex = i;
        slotInfo[slotIndex++].slotObjectType = XGL_SLOT_SHADER_SAMPLER;
    }

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

void XglRenderTest::DrawTriangleFSUniformBlockBinding(const char *vertShaderText, const char *fragShaderText)
{
    // sourced from DrawTriangleFSUniformBlock

    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    const int bufferCount = 4;
    ASSERT_NO_FATAL_FAILURE(CreatePipelineFSUniformBlockBinding(&pipeline, vs, ps, bufferCount));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());


    // We're going to create a number of uniform buffers, and then allow
    // the shader to select which it wants to read from with a binding

    // Let's populate the buffers with a single color each:
    //    layout (std140, binding = 0) uniform bufferVals { vec4 red;   } myRedVal;
    //    layout (std140, binding = 1) uniform bufferVals { vec4 green; } myGreenVal;
    //    layout (std140, binding = 2) uniform bufferVals { vec4 blue;  } myBlueVal;
    //    layout (std140, binding = 3) uniform bufferVals { vec4 white; } myWhiteVal;

    assert(4 == bufferCount);  // update the following code if you want more than 4

    const float redVals[4]   = { 1.0, 0.0, 0.0, 1.0 };
    const float greenVals[4] = { 0.0, 1.0, 0.0, 1.0 };
    const float blueVals[4]  = { 0.0, 0.0, 1.0, 1.0 };
    const float whiteVals[4] = { 1.0, 1.0, 1.0, 1.0 };

    const int redCount   = sizeof(redVals)   / sizeof(float);
    const int greenCount = sizeof(greenVals) / sizeof(float);
    const int blueCount  = sizeof(blueVals)  / sizeof(float);
    const int whiteCount = sizeof(whiteVals) / sizeof(float);

    int index = 0;
    InitUniformBuffer(redCount,   sizeof(redVals[0]),   index++, (const void *) redVals);
    InitUniformBuffer(greenCount, sizeof(greenVals[0]), index++, (const void *) greenVals);
    InitUniformBuffer(blueCount,  sizeof(blueVals[0]),  index++, (const void *) blueVals);
    InitUniformBuffer(whiteCount, sizeof(whiteVals[0]), index++, (const void *) whiteVals);


    // Create descriptor set for a uniform resource
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = bufferCount;

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the constant buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );

    for(int i = 0; i < bufferCount; ++i)
        xglAttachMemoryViewDescriptors( m_rsrcDescSet, i, 1, &m_uniformBufferView[i] );

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

void XglRenderTest::DrawSamplerBindingsTriangle(const char *vertShaderText, const char *fragShaderText,
                                               int textureCount, int samplerCount)
{
    // based on DrawTexturedTriangle

    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreatePipelineMultipleTexturesAndSamplers(&pipeline, vs, ps, textureCount, samplerCount));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());


    // Create a few texture/sampler pairs
    int textureColors[16];
    assert(textureCount < 16);
    textureColors[0] = 0xFF0000FF; //red
    textureColors[1] = 0xFF00FF00; //green
    textureColors[2] = 0xFFFF0000; //blue

    ASSERT_NO_FATAL_FAILURE(InitMultipleSamplers(samplerCount));
    ASSERT_NO_FATAL_FAILURE(InitMultipleTextures(textureCount, textureColors));

    // Create descriptor set for a texture and sampler resources
    const int slotCount = textureCount + samplerCount;
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
    int descSlot = 0;
    for (int i = 0; i < textureCount; ++i)
        xglAttachImageViewDescriptors( m_rsrcDescSet, descSlot++, 1, &m_textureViewInfo[i]);
    for (int i = 0; i < samplerCount; ++i)
        xglAttachSamplerDescriptors(m_rsrcDescSet, descSlot++, 1, &m_sampler[i]);
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

    XglTestFramework::m_use_bil = true;
    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, MixTriangle)
{
    // This tests location applied to varyings. Notice that we have switched foo
    // and bar in the FS. The triangle should be blended with red, green and blue
    // corners.
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location=0) out vec4 bar;\n"
            "layout (location=1) out vec4 foo;\n"
            "layout (location=2) out float scale;\n"
            "vec2 vertices[3];\n"
            "void main() {\n"
            "      vertices[0] = vec2(-1.0, -1.0);\n"
            "      vertices[1] = vec2( 1.0, -1.0);\n"
            "      vertices[2] = vec2( 0.0,  1.0);\n"
            "vec4 colors[3];\n"
            "      colors[0] = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "      colors[1] = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "      colors[2] = vec4(0.0, 0.0, 1.0, 1.0);\n"
            "   foo = colors[gl_VertexID % 3];\n"
            "   bar = vec4(1.0, 1.0, 1.0, 1.0);\n"
            "   scale = 1.0;\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
           "#version 140\n"
           "#extension GL_ARB_separate_shader_objects : enable\n"
           "#extension GL_ARB_shading_language_420pack : enable\n"
           "layout (location = 1) in vec4 bar;\n"
           "layout (location = 0) in vec4 foo;\n"
           "layout (location = 2) in float scale;\n"
           "void main() {\n"
           "   gl_FragColor = bar * scale + foo * (1.0-scale);\n"
           "}\n";

    XglTestFramework::m_use_bil = true;
    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleWithVertexFetch)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            //XYZ1( -1, -1, -1 )
            "layout (location = 0) in vec4 pos;\n"
            //XYZ1( 0.f, 0.f, 0.f )
            "layout (location = 1) in vec4 inColor;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   gl_Position = pos;\n"
            "}\n";


    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 color;\n"
            "void main() {\n"
            "   gl_FragColor = color;\n"
            "}\n";

    XglTestFramework::m_use_bil = true;
    DrawTriangleWithVertexFetch(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TexturedTriangle)
{
    // The expected result from this test is a red and green checkered triangle
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec2 samplePos;\n"
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
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec2 samplePos;\n"
            "layout (binding = 0) uniform sampler2D surface;\n"
            "void main() {\n"
            "   vec4 texColor = textureLod(surface, samplePos, 0.0);\n"
            "   gl_FragColor = texColor;\n"
            "}\n";

    XglTestFramework::m_use_bil = true;
    DrawTexturedTriangle(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, VSTexture)
{
    // The expected result from this test is a green and red triangle;
    // one red vertex on the left, two green vertices on the right.
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 texColor;\n"
            "layout (binding = 0) uniform sampler2D surface;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   vec2 positions[3];"
            "      positions[0] = vec2( 0.0, 0.0);\n"
            "      positions[1] = vec2( 0.25, 0.1);\n"
            "      positions[2] = vec2( 0.1, 0.25);\n"
            "   vec2 samplePos = positions[gl_VertexID % 3];\n"
            "   texColor = textureLod(surface, samplePos, 0.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 texColor;\n"
            "void main() {\n"
            "   gl_FragColor = texColor;\n"
            "}\n";

    XglTestFramework::m_use_bil = true;
    DrawTexturedTriangle(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, SamplerBindingsTriangle)
{
    // This test sets bindings on the samplers
    // For now we are asserting that sampler and texture pairs
    // march in lock step, and are set via GLSL binding.  This can
    // and will probably change.
    // The sampler bindings should match the sampler and texture slot
    // number set up by the application.
    // This test will result in a blue triangle
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 samplePos;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   vec2 positions[3];"
            "      positions[0] = vec2( 0.0, 0.0);\n"
            "      positions[1] = vec2( 1.0, 0.0);\n"
            "      positions[2] = vec2( 1.0, 1.0);\n"
            "   samplePos = vec4(positions[gl_VertexID % 3], 0.0, 0.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

   static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 samplePos;\n"
            "layout (binding = 0) uniform sampler2D surface0;\n"
            "layout (binding = 1) uniform sampler2D surface1;\n"
            "layout (binding = 2) uniform sampler2D surface2;\n"
            "void main() {\n"
            "   gl_FragColor = textureLod(surface2, samplePos.xy, 0.0);\n"
            "}\n";

   XglTestFramework::m_use_bil = true;
   int textureCount = g_TextureCount;
   int samplerCount = g_SamplerCount;
   DrawSamplerBindingsTriangle(vertShaderText, fragShaderText, textureCount, samplerCount);
}

TEST_F(XglRenderTest, TriangleVSUniformBlock)
{
    // The expected result from this test is a blue triangle

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 outColor;\n"
            "layout (std140, binding = 0) uniform bufferVals {\n"
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
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 inColor;\n"
            "void main() {\n"
            "   gl_FragColor = inColor;\n"
            "}\n";

    XglTestFramework::m_use_bil = true;
    DrawTriangleVSUniformBlock(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleFSUniformBlockBinding)
{
    // This test allows the shader to select which buffer it is
    // pulling from using layout binding qualifier.
    // There are corresponding changes in the compiler stack that
    // will select the buffer using binding directly.
    // The binding number should match the slot number set up by
    // the application.
    // The expected result from this test is a purple triangle

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform redVal   { vec4 color; } myRedVal\n;"
            "layout (std140, binding = 1) uniform greenVal { vec4 color; } myGreenVal\n;"
            "layout (std140, binding = 2) uniform blueVal  { vec4 color; } myBlueVal\n;"
            "layout (std140, binding = 3) uniform whiteVal { vec4 color; } myWhiteVal\n;"
            "void main() {\n"
            "   gl_FragColor = myBlueVal.color;\n"
            "   gl_FragColor += myRedVal.color;\n"
            "}\n";

    XglTestFramework::m_use_bil = true;
    DrawTriangleFSUniformBlockBinding(vertShaderText, fragShaderText);
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
