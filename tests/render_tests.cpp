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
#ifdef PRINT_OBJECTS
#include "../layers/object_track.h"
#endif
#ifdef DEBUG_CALLBACK
#include <xglDbg.h>
#endif
#include "gtest-1.7.0/include/gtest/gtest.h"

#include "xgldevice.h"
#include "xglimage.h"
#include "icd-bil.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "xglrenderframework.h"
#ifdef DEBUG_CALLBACK
XGL_VOID XGLAPI myDbgFunc(
    XGL_DBG_MSG_TYPE     msgType,
    XGL_VALIDATION_LEVEL validationLevel,
    XGL_BASE_OBJECT      srcObject,
    XGL_SIZE             location,
    XGL_INT              msgCode,
    const XGL_CHAR*      pMsg,
    XGL_VOID*            pUserData)
{
    switch (msgType)
    {
        case XGL_DBG_MSG_WARNING:
            printf("CALLBACK WARNING : %s\n", pMsg);
            break;
        case XGL_DBG_MSG_ERROR:
            printf("CALLBACK ERROR : %s\n", pMsg);
            break;
        default:
            printf("EATING Msg of type %u\n", msgType);
            break;
    }
}
#endif
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

static const Vertex g_vb_solid_face_colors_Data[] =
{
    { XYZ1( -1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },

    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( 1,  1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },

    { XYZ1( 1,  1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( 1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 0.f, 0.f, 1.f ) },

    { XYZ1( -1,  1,  1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1, -1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },

    { XYZ1( 1,  1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 0.f, 1.f ) },

    { XYZ1( 1, -1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1, -1, -1 ), XYZ1( 0.f, 1.f, 1.f ) },
};

class XglRenderTest : public XglRenderFramework
{
public:
    void InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
    void UploadMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
    void InitTexture();
    void InitSampler();
    void DrawTriangleTest(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleTwoUniformsFS(const char *vertShaderText, const char *fragShaderText);
    void DrawTriangleWithVertexFetch(const char *vertShaderText, const char *fragShaderText);
    void RotateTriangleVSUniform(glm::mat4 Projection, glm::mat4 View, glm::mat4 Model);
    void DrawTriangleVSUniform(const char *vertShaderText, const char *fragShaderText, int numTris);
    void DrawTriangleWithVertexFetchAndMVP(const char *vertShaderText, const char *fragShaderText);

    void CreatePipelineWithVertexFetch(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineWithVertexFetchAndMVP(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void CreatePipelineVSUniform(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void ClearDepthStencil(XGL_FLOAT value);
    void ClearRenderBuffer(XGL_UINT32 clear_color);
    void InitDepthStencil();
    void DrawRotatedTriangleTest();
    void GenerateClearAndPrepareBufferCmds();
    void XGLTriangleTest(const char *vertShaderText, const char *fragShaderText);


protected:
    XGL_IMAGE m_texture;
    XGL_IMAGE_VIEW m_textureView;
    XGL_IMAGE_VIEW_ATTACH_INFO m_textureViewInfo;
    XGL_GPU_MEMORY m_textureMem;

    XGL_SAMPLER m_sampler;

    XGL_FORMAT                  m_depth_stencil_fmt;
    XGL_IMAGE                   m_depthStencilImage;
    XGL_GPU_MEMORY              m_depthStencilMem;
    XGL_DEPTH_STENCIL_VIEW      m_depthStencilView;

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
        this->app_info.pAppName = (const XGL_CHAR *) "render_tests";
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
}

// this function will create the vertex buffer and fill it with the mesh data
void XglRenderTest::UploadMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride,
                              const void* vertices )
{
    XGL_UINT8 *pData;
    XGL_RESULT err = XGL_SUCCESS;

    assert( numVertices * vbStride > 0 );
    m_numVertices = numVertices;

    err = xglMapMemory(m_vtxBufferMem, 0, (XGL_VOID **) &pData);
    ASSERT_XGL_SUCCESS(err);

    memcpy(pData, vertices, numVertices * vbStride);

    err = xglUnmapMemory(m_vtxBufferMem);
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
    m_numMemRefs = 1;
    m_memRefs[0].flags = 0;
    m_memRefs[0].mem = m_renderTarget->memory();
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


void XglRenderTest::DrawTriangleVSUniform(const char *vertShaderText, const char *fragShaderText, int numTris)
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

    ASSERT_NO_FATAL_FAILURE(CreatePipelineVSUniform(&pipeline, vs, ps));

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

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
    xglCmdDraw( m_cmdBuffer, 0, numTris*3, 0, 1 );

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

    // this command buffer only uses a data buffer for the MVP
    m_numMemRefs = 1;
    m_memRefs[0].flags = 0;
    m_memRefs[0].mem = m_constantBufferMem;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);
}


void XglRenderTest::RotateTriangleVSUniform(glm::mat4 Projection, glm::mat4 View, glm::mat4 Model)
{
    int i;
    glm::mat4 MVP;
    int matrixSize = sizeof(MVP);
    XGL_RESULT err;

    for (i = 0; i < 8; i++) {
        XGL_UINT8 *pData;
        err = xglMapMemory(m_constantBufferMem, 0, (XGL_VOID **) &pData);
        ASSERT_XGL_SUCCESS(err);

        Model = glm::rotate(Model, glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
        MVP = Projection * View * Model;
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

void dumpMatrix(const char *note, glm::mat4 MVP)
{
    int i,j;

    printf("%s: \n", note);
    for (i=0; i<4; i++) {
        printf("%f, %f, %f, %f\n", MVP[i][0], MVP[i][1], MVP[i][2], MVP[i][3]);
    }
    printf("\n");
    fflush(stdout);
}

void dumpVec4(const char *note, glm::vec4 vector)
{
    printf("%s: \n", note);
        printf("%f, %f, %f, %f\n", vector[0], vector[1], vector[2], vector[3]);
    printf("\n");
    fflush(stdout);
}

void XglRenderTest::GenerateClearAndPrepareBufferCmds()
{
    XglRenderFramework::GenerateClearAndPrepareBufferCmds();

    if (0) {
//    if (m_depthStencilImage) {
        XGL_IMAGE_SUBRESOURCE_RANGE dsRange = {};
        dsRange.aspect = XGL_IMAGE_ASPECT_DEPTH;
        dsRange.baseMipLevel = 0;
        dsRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
        dsRange.baseArraySlice = 0;
        dsRange.arraySize = XGL_LAST_MIP_OR_SLICE;

        // prepare the depth buffer for clear
        XGL_IMAGE_STATE_TRANSITION transitionToClear = {};
        transitionToClear.image = m_depthStencilImage;
        transitionToClear.oldState = m_depthStencilBinding.depthState;
        transitionToClear.newState = XGL_IMAGE_STATE_CLEAR;
        transitionToClear.subresourceRange = dsRange;
        xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToClear );
        m_renderTarget->state(( XGL_IMAGE_STATE ) transitionToClear.newState);

        xglCmdClearDepthStencil(m_cmdBuffer, m_depthStencilImage, 1.0f, 0, 1, &dsRange);

        // prepare depth buffer for rendering
        XGL_IMAGE_STATE_TRANSITION transitionToRender = {};
        transitionToRender.image = m_renderTarget->image();
        transitionToRender.oldState = XGL_IMAGE_STATE_CLEAR;
        transitionToRender.newState = m_depthStencilBinding.depthState;
        transitionToRender.subresourceRange = dsRange;
        xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToRender );
        m_renderTarget->state(( XGL_IMAGE_STATE ) transitionToClear.newState);
    }
}

void XglRenderTest::DrawTriangleWithVertexFetchAndMVP(const char *vertShaderText, const char *fragShaderText)
{
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;
    int i, loop;

    // Projection matrix : 45° Field of View, 1:1 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
//    dumpMatrix("Projection", Projection);

    // Camera matrix
    glm::mat4 View       = glm::lookAt(
                               glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                               glm::vec3(0,0,0), // and looks at the origin
                               glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                               );
//    dumpMatrix("View", View);

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
//    dumpMatrix("Model", Model);

    // Our ModelViewProjection : multiplication of our 3 matrices
//    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 4.0f));
//    Model = glm::rotate(Model, glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 MVP = Projection * View * Model;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitDepthStencil());
    ASSERT_NO_FATAL_FAILURE(InitMesh(sizeof(g_vb_solid_face_colors_Data)/sizeof(g_vb_solid_face_colors_Data[0]),
                            sizeof(g_vb_solid_face_colors_Data[0]), g_vb_solid_face_colors_Data));

    const int buf_size = sizeof(MVP) / sizeof(XGL_FLOAT);

    InitConstantBuffer(buf_size, sizeof(MVP[0][0]), (const void*) &MVP[0][0]);

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX,
                                         vertShaderText, &vs));

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT,
                                         fragShaderText, &ps));

    ASSERT_NO_FATAL_FAILURE(CreatePipelineWithVertexFetchAndMVP(&pipeline, vs, ps));

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
    ClearDepthStencil(1.0f); // HACK for now
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 12*3, 0, 1 );

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

    // this command buffer uses the vertex buffer memory and a data buffer (for MVP)
    m_numMemRefs = 2;
    m_memRefs[0].flags = 0;
    m_memRefs[0].mem = m_vtxBufferMem;
    m_memRefs[1].flags = 0;
    m_memRefs[1].mem = m_constantBufferMem;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

    for (loop = 0; loop < 16; loop++) {
//        ClearRenderBuffer(0x80); // HACK
        ClearDepthStencil(1.0f); // HACK for now

        // TODO: Do we need to transition the constant buffer?
        XGL_UINT8 *pData;
        err = xglMapMemory(m_constantBufferMem, 0, (XGL_VOID **) &pData);
        ASSERT_XGL_SUCCESS(err);

        Model = glm::rotate(Model, glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
//        dumpMatrix("Model", Model);
        glm::mat4 MVP = Projection * View * Model;
//        dumpMatrix("MVP", MVP);

        memcpy(pData, (const void*) &MVP[0][0], buf_size * sizeof(XGL_FLOAT));

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

    memset(&vs_stage, 0, sizeof(vs_stage));
    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = vs;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = 0;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    memset(&ps_stage, 0, sizeof(ps_stage));
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

/*
 * Based on CreatePipelineWithVertexFetch and CreatePipelineVSUniform
 */
void XglRenderTest::CreatePipelineWithVertexFetchAndMVP(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps)
{
    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;

    // Create descriptor set for our two resources
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = 2; // Vertex buffer and Model View Matrix

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // write the vertex buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_constantBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    const int slots = 1;
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( slots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    slotInfo[0].shaderEntityIndex = 0;
    slotInfo[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;

    memset(&vs_stage, 0, sizeof(vs_stage));
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

    memset(&ps_stage, 0, sizeof(ps_stage));
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
        m_depth_stencil_fmt                    // XGL_FORMAT
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

    memset(&vs_stage, 0, sizeof(vs_stage));
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

    memset(&ps_stage, 0, sizeof(ps_stage));
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

void XglRenderTest::ClearDepthStencil(XGL_FLOAT value)
/* clear the buffer */
{
    XGL_RESULT err;
    const uint16_t depth_value = (uint16_t) (value * 65535);
    const XGL_INT tw = 128 / sizeof(uint16_t);
    const XGL_INT th = 32;
    XGL_INT i, j, w, h;
    XGL_VOID *data;

    w = (m_width + tw - 1) / tw;
    h = (m_height + th - 1) / th;

    err = xglMapMemory(m_depthStencilMem, 0, &data);
    ASSERT_XGL_SUCCESS(err);

    for (i = 0; i < w * h; i++) {
        uint16_t *tile = (uint16_t *) ((char *) data + 4096 * i);

        for (j = 0; j < 2048; j++)
            tile[j] = depth_value;
    }

    err = xglUnmapMemory(m_depthStencilMem);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::ClearRenderBuffer(XGL_UINT32 clear_color)
/* clear the buffer */
{
    XGL_RESULT err;
    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    XGL_UINT data_size = sizeof(sr_layout);
    XGL_VOID    *ptr;

    err = xglGetImageSubresourceInfo( m_renderTarget->image(),
                                      &sr, XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    err = m_renderTarget->MapMemory( &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr = (void *) ((char *) ptr + sr_layout.offset);

    memset(ptr, clear_color, m_width * m_height *sizeof(XGL_UINT32));

    err = m_renderTarget->UnmapMemory();
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::InitDepthStencil()
{
    XGL_RESULT err;
    XGL_IMAGE_CREATE_INFO image;
    XGL_MEMORY_ALLOC_INFO mem_alloc;
    XGL_DEPTH_STENCIL_VIEW_CREATE_INFO view;
    XGL_MEMORY_REQUIREMENTS mem_reqs;
    XGL_SIZE mem_reqs_size;

    // Clean up default state created by framework
    if (m_stateDepthStencil) xglDestroyObject(m_stateDepthStencil);

    m_depth_stencil_fmt.channelFormat = XGL_CH_FMT_R16;
    m_depth_stencil_fmt.numericFormat = XGL_NUM_FMT_DS;

    image.sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.pNext = NULL;
    image.imageType = XGL_IMAGE_2D;
    image.format = m_depth_stencil_fmt;
    image.extent.width = m_width;
    image.extent.height = m_height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arraySize = 1;
    image.samples = 1;
    image.tiling = XGL_OPTIMAL_TILING;
    image.usage = XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT;
    image.flags = 0;

    mem_alloc.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.alignment = 0;
    mem_alloc.flags = 0;
    mem_alloc.heapCount = 0;
    mem_alloc.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    /* create image */
    err = xglCreateImage(device(), &image,
                         &m_depthStencilImage);
    ASSERT_XGL_SUCCESS(err);

    err = xglGetObjectInfo(m_depthStencilImage,
                           XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &mem_reqs_size, &mem_reqs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(mem_reqs_size, sizeof(mem_reqs));

    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.alignment = mem_reqs.alignment;
    mem_alloc.heapCount = mem_reqs.heapCount;
    memcpy(mem_alloc.heaps, mem_reqs.heaps,
           sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

    /* allocate memory */
    err = xglAllocMemory(device(), &mem_alloc, &m_depthStencilMem);
    ASSERT_XGL_SUCCESS(err);

    /* bind memory */
    err = xglBindObjectMemory(m_depthStencilImage, m_depthStencilMem, 0);
    ASSERT_XGL_SUCCESS(err);

    XGL_DEPTH_STENCIL_STATE_CREATE_INFO depthStencil = {};
    depthStencil.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable        = XGL_TRUE;
    depthStencil.depthWriteEnable       = XGL_TRUE;
    depthStencil.depthFunc = XGL_COMPARE_LESS_EQUAL;
    depthStencil.depthBoundsEnable = XGL_FALSE;
    depthStencil.minDepth = 0.f;
    depthStencil.maxDepth = 1.f;
    depthStencil.back.stencilDepthFailOp = XGL_STENCIL_OP_KEEP;
    depthStencil.back.stencilFailOp = XGL_STENCIL_OP_KEEP;
    depthStencil.back.stencilPassOp = XGL_STENCIL_OP_KEEP;
    depthStencil.back.stencilRef = 0x00;
    depthStencil.back.stencilFunc = XGL_COMPARE_ALWAYS;
    depthStencil.front = depthStencil.back;

    err = xglCreateDepthStencilState( device(), &depthStencil, &m_stateDepthStencil );
    ASSERT_XGL_SUCCESS( err );

    /* create image view */
    view.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO;
    view.pNext = NULL;
    view.image = XGL_NULL_HANDLE;
    view.mipLevel = 0;
    view.baseArraySlice = 0;
    view.arraySize = 1;
    view.flags = 0;
    view.image = m_depthStencilImage;
    err = xglCreateDepthStencilView(device(), &view, &m_depthStencilView);
    ASSERT_XGL_SUCCESS(err);

    m_depthStencilBinding.view = m_depthStencilView;
    m_depthStencilBinding.depthState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
    m_depthStencilBinding.stencilState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
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
    xglCmdDraw( m_cmdBuffer, 0, 12*3, 0, 1 );

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

struct xgltriangle_vs_uniform {
    // Must start with MVP
    XGL_FLOAT   mvp[4][4];
    XGL_FLOAT   position[3][4];
    XGL_FLOAT   color[3][4];
};

void XglRenderTest::XGLTriangleTest(const char *vertShaderText, const char *fragShaderText)
{
#ifdef DEBUG_CALLBACK
    xglDbgRegisterMsgCallback(myDbgFunc, NULL);
#endif
    // Create identity matrix
    int i;
    struct xgltriangle_vs_uniform data;

    glm::mat4 Projection      = glm::mat4(1.0f);
    glm::mat4 View      = glm::mat4(1.0f);
    glm::mat4 Model      = glm::mat4(1.0f);
    glm::mat4 MVP = Projection * View * Model;
    const int matrixSize = sizeof(MVP);
    const int bufSize = sizeof(xgltriangle_vs_uniform) / sizeof(XGL_FLOAT);
    memcpy(&data.mvp, &MVP[0][0], matrixSize);

    static const Vertex tri_data[] =
    {
        { XYZ1( -1, -1, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
        { XYZ1( 1, -1, 0 ), XYZ1( 0.f, 1.f, 0.f ) },
        { XYZ1( 0,  1, 0 ), XYZ1( 0.f, 0.f, 1.f ) },
    };

    for (i=0; i<3; i++) {
        data.position[i][0] = tri_data[i].posX;
        data.position[i][1] = tri_data[i].posY;
        data.position[i][2] = tri_data[i].posZ;
        data.position[i][3] = tri_data[i].posW;
        data.color[i][0] = tri_data[i].r;
        data.color[i][1] = tri_data[i].g;
        data.color[i][2] = tri_data[i].b;
        data.color[i][3] = tri_data[i].a;
    }

    InitConstantBuffer(bufSize, sizeof(XGL_FLOAT), (const void*) &data);
    DrawTriangleVSUniform(vertShaderText, fragShaderText, 1);
    RotateTriangleVSUniform(Projection, View, Model);
#ifdef PRINT_OBJECTS
    //XGL_UINT64 objTrackGetObjectCount(XGL_OBJECT_TYPE type)
    OBJ_TRACK_GET_OBJECT_COUNT pObjTrackGetObjectCount = (OBJ_TRACK_GET_OBJECT_COUNT)xglGetProcAddr(gpu(), (XGL_CHAR*)"objTrackGetObjectCount");
    XGL_UINT64 numObjects = pObjTrackGetObjectCount(XGL_OBJECT_TYPE_ANY);
    //OBJ_TRACK_GET_OBJECTS pGetObjsFunc = xglGetProcAddr(gpu(), (XGL_CHAR*)"objTrackGetObjects");
    printf("DEBUG : Number of Objects : %lu\n", numObjects);
    OBJ_TRACK_GET_OBJECTS pObjTrackGetObjs = (OBJ_TRACK_GET_OBJECTS)xglGetProcAddr(gpu(), (XGL_CHAR*)"objTrackGetObjects");
    OBJTRACK_NODE* pObjNodeArray = (OBJTRACK_NODE*)malloc(sizeof(OBJTRACK_NODE)*numObjects);
    pObjTrackGetObjs(XGL_OBJECT_TYPE_ANY, numObjects, pObjNodeArray);
    for (i=0; i < numObjects; i++) {
        printf("Object %i of type %s has objID (%p) and %lu uses\n", i, string_XGL_OBJECT_TYPE(pObjNodeArray[i].objType), pObjNodeArray[i].pObj, pObjNodeArray[i].numUses);
    }
    free(pObjNodeArray);
#endif
}

TEST_F(XglRenderTest, XGLTriangle_FragColor)
{
    static const char *vertShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "\n"
        "layout(binding = 0) uniform buf {\n"
        "        mat4 MVP;\n"
        "        vec4 position[3];\n"
        "        vec4 color[3];\n"
        "} ubuf;\n"
        "\n"
        "layout (location = 0) out vec4 outColor;\n"
        "\n"
        "void main() \n"
        "{\n"
        "   outColor = ubuf.color[gl_VertexID];\n"
        "   gl_Position = ubuf.MVP * ubuf.position[gl_VertexID];\n"
        "}\n";

    static const char *fragShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "\n"
        "layout (location = 0) in vec4 inColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_FragColor = inColor;\n"
        "}\n";

    TEST_DESCRIPTION("XGL-style shaders where fragment shader outputs to GLSL built-in gl_FragColor");
    XGLTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, XGLTriangle_OutputLocation)
{
    static const char *vertShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "\n"
        "layout(binding = 0) uniform buf {\n"
        "        mat4 MVP;\n"
        "        vec4 position[3];\n"
        "        vec4 color[3];\n"
        "} ubuf;\n"
        "\n"
        "layout (location = 0) out vec4 outColor;\n"
        "\n"
        "void main() \n"
        "{\n"
        "   outColor = ubuf.color[gl_VertexID];\n"
        "   gl_Position = ubuf.MVP * ubuf.position[gl_VertexID];\n"
        "}\n";

    static const char *fragShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "\n"
        "layout (location = 0) in vec4 inColor;\n"
        "layout (location = 0) out vec4 outColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   outColor = inColor;\n"
        "}\n";

    TEST_DESCRIPTION("XGL-style shaders where fragment shader outputs to output location 0, which should be the same as gl_FragColor");

    XGLTriangleTest(vertShaderText, fragShaderText);
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

    TEST_DESCRIPTION("Basic shader that renders a fixed Green triangle coded as part of the vertex shader.");

    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleWithVertexFetch)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout(location = 0) in vec4 pos;\n"
            "layout(location = 1) in vec4 inColor;\n"
            "layout(location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   gl_Position = pos;\n"
            "}\n";


    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout(location = 0) in vec4 color;\n"
            "void main() {\n"
            "   gl_FragColor = color;\n"
            "}\n";

    DrawTriangleWithVertexFetch(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TriangleVSUniform)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "\n"
            "layout(binding = 0) uniform buf {\n"
            "        mat4 MVP;\n"
            "} ubuf;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   gl_Position = ubuf.MVP * vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 130\n"
            "void main() {\n"
            "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "}\n";

    // Create identity matrix
    glm::mat4 Projection    = glm::mat4(1.0f);
    glm::mat4 View          = glm::mat4(1.0f);
    glm::mat4 Model         = glm::mat4(1.0f);
    glm::mat4 MVP = Projection * View * Model;
    const int matrixSize = sizeof(MVP) / sizeof(MVP[0]);

    InitConstantBuffer(matrixSize, sizeof(MVP[0]), (const void*) &MVP[0][0]);

    DrawTriangleVSUniform(vertShaderText, fragShaderText, 1);
    RotateTriangleVSUniform(Projection, View, Model);
}

TEST_F(XglRenderTest, TriangleWithVertexFetchAndMVP)
{
    static const char *vertShaderText =
            "#version 140\n"
            "layout (std140) uniform bufferVals {\n"
            "    mat4 mvp;\n"
            "} myBufferVals;\n"
            "in vec4 pos;\n"
            "in vec4 inColor;\n"
            "out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   gl_Position = myBufferVals.mvp * pos;\n"
            "}\n";

    static const char *fragShaderText =
            "#version 130\n"
            "in vec4 color;\n"
            "void main() {\n"
            "   gl_FragColor = color;\n"
            "}\n";

    DrawTriangleWithVertexFetchAndMVP(vertShaderText, fragShaderText);
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
