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
#include <xglIntelExt.h>
#include "gtest-1.7.0/include/gtest/gtest.h"

#include "xgldevice.h"
#include "xglimage.h"
#include "icd-bil.h"

#include "xglrenderframework.h"

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

static const uint32_t gen6_fs[] = {
    0x00600001, 0x202003fe, 0x00000000, 0x3f800000, // mov(8)          m1<1>F          1F                              { align1 1Q };
    0x00600001, 0x204003fe, 0x00000000, 0x00000000, // mov(8)          m2<1>F          0F                              { align1 1Q };
    0x00600001, 0x206003fe, 0x00000000, 0x00000000, // mov(8)          m3<1>F          0F                              { align1 1Q };
    0x00600001, 0x208003fe, 0x00000000, 0x3f800000, // mov(8)          m4<1>F          1F                              { align1 1Q };
    0x05600032, 0x20001fc8, 0x008d0020, 0x88019400, // sendc(8)        null            m1<8,8,1>F
                                                    // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
};

static const uint32_t gen6_vs[] = {
    0x01600110, 0x200f1ca4, 0x00600020, 0x00000000, // cmp.z.f0(8)     null            g1<4,4,1>.xD    0D              { align16 1Q };
    0x00670122, 0x000a108f, 0x000e0004, 0x000e0004, // (+f0.all4h) if(8) JIP: 10                                       { align16 1Q };
    0x00600501, 0x204303fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.xyF       -1F                             { align16 NoDDClr 1Q };
    0x00600d01, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDClr,NoDDChk 1Q };
    0x00600901, 0x204803fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.wF        1F                              { align16 NoDDChk 1Q };
    0x00600124, 0x0014108f, 0x006e0004, 0x006e0004, // else(8)         JIP: 20                                         { align16 1Q };
    0x01600110, 0x200f1ca4, 0x00600020, 0x00000001, // cmp.z.f0(8)     null            g1<4,4,1>.xD    1D              { align16 1Q };
    0x00670122, 0x000a108f, 0x000e0004, 0x000e0004, // (+f0.all4h) if(8) JIP: 10                                       { align16 1Q };
    0x00600501, 0x204903fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.xwF       1F                              { align16 NoDDClr 1Q };
    0x00600d01, 0x204203fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.yF        -1F                             { align16 NoDDClr,NoDDChk 1Q };
    0x00600901, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDChk 1Q };
    0x00600124, 0x0006108f, 0x006e0004, 0x006e0004, // else(8)         JIP: 6                                          { align16 1Q };
    0x00600501, 0x204503fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.xzF       0F                              { align16 NoDDClr 1Q };
    0x00600901, 0x204a03fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.ywF       1F                              { align16 NoDDChk 1Q };
    0x00600125, 0x0002108f, 0x006e0004, 0x006e0002, // endif(8)        JIP: 2                                          { align16 1Q };
    0x00600125, 0x0002108f, 0x006e0004, 0x006e0002, // endif(8)        JIP: 2                                          { align16 1Q };
    0x00600101, 0x204f0062, 0x00000000, 0x00000000, // mov(8)          m2<1>UD         0x00000000UD                    { align16 1Q };
    0x00600101, 0x206f03be, 0x006e0044, 0x00000000, // mov(8)          m3<1>F          g2<4,4,1>F                      { align16 1Q };
    0x00600301, 0x202f0022, 0x006e0004, 0x00000000, // mov(8)          m1<1>UD         g0<4,4,1>UD                     { align16 WE_all 1Q };
    0x06600131, 0x200f1fdc, 0x006e0024, 0x8608c400, // send(8)         null            m1<4,4,1>F
                                                    // urb 0 urb_write interleave used complete mlen 3 rlen 0 { align16 1Q EOT };
};

static const uint32_t gen7_fs[] = {
    0x00600001, 0x2e2003fd, 0x00000000, 0x3f800000, // mov(8)          g113<1>F        1F                              { align1 1Q };
    0x00600001, 0x2e4003fd, 0x00000000, 0x00000000, // mov(8)          g114<1>F        0F                              { align1 1Q };
    0x00600001, 0x2e6003fd, 0x00000000, 0x00000000, // mov(8)          g115<1>F        0F                              { align1 1Q };
    0x00600001, 0x2e8003fd, 0x00000000, 0x3f800000, // mov(8)          g116<1>F        1F                              { align1 1Q };
    0x05600032, 0x20001fa8, 0x008d0e20, 0x88031400, // sendc(8)        null            g113<8,8,1>F
                                                    // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
};

static const uint32_t gen7_vs[] = {
    0x01608110, 0x200f1ca4, 0x00600020, 0x00000000, // cmp.z.f0(8)     null            g1<4,4,1>.xD    0D              { align16 1Q switch };
    0x00670122, 0x200f0c84, 0x000e0004, 0x001c000a, // (+f0.all4h) if(8) JIP: 10       UIP: 28                         { align16 1Q };
    0x00600501, 0x204303fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.xyF       -1F                             { align16 NoDDClr 1Q };
    0x00600d01, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDClr,NoDDChk 1Q };
    0x00600901, 0x204803fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.wF        1F                              { align16 NoDDChk 1Q };
    0x00600124, 0x200f0c84, 0x006e0004, 0x00000014, // else(8)         JIP: 20                                         { align16 1Q };
    0x01608110, 0x200f1ca4, 0x00600020, 0x00000001, // cmp.z.f0(8)     null            g1<4,4,1>.xD    1D              { align16 1Q switch };
    0x00670122, 0x200f0c84, 0x000e0004, 0x000e000a, // (+f0.all4h) if(8) JIP: 10       UIP: 14                         { align16 1Q };
    0x00600501, 0x204903fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.xwF       1F                              { align16 NoDDClr 1Q };
    0x00600d01, 0x204203fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.yF        -1F                             { align16 NoDDClr,NoDDChk 1Q };
    0x00600901, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDChk 1Q };
    0x00600124, 0x200f0c84, 0x006e0004, 0x00000006, // else(8)         JIP: 6                                          { align16 1Q };
    0x00600501, 0x204503fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.xzF       0F                              { align16 NoDDClr 1Q };
    0x00600901, 0x204a03fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.ywF       1F                              { align16 NoDDChk 1Q };
    0x00600125, 0x200f0c84, 0x006e0004, 0x00000002, // endif(8)        JIP: 2                                          { align16 1Q };
    0x00600125, 0x200f0c84, 0x006e0004, 0x00000002, // endif(8)        JIP: 2                                          { align16 1Q };
    0x00600101, 0x2e4f0061, 0x00000000, 0x00000000, // mov(8)          g114<1>UD       0x00000000UD                    { align16 1Q };
    0x00600101, 0x2e6f03bd, 0x006e0044, 0x00000000, // mov(8)          g115<1>F        g2<4,4,1>F                      { align16 1Q };
    0x00600301, 0x2e2f0021, 0x006e0004, 0x00000000, // mov(8)          g113<1>UD       g0<4,4,1>UD                     { align16 WE_all 1Q };
    0x00000206, 0x2e340c21, 0x00000014, 0x0000ff00, // or(1)           g113.5<1>UD     g0.5<0,1,0>UD   0x0000ff00UD    { align1 WE_all };
    0x06600131, 0x200f1fbc, 0x006e0e24, 0x8608c000, // send(8)         null            g113<4,4,1>F
                                                    // urb 0 write HWord interleave complete mlen 3 rlen 0 { align16 1Q EOT };
};

class XglRenderTest : public XglRenderFramework
{
public:
    void InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
    void InitTexture();
    void InitSampler();
    void DrawTriangleTest(const char *vertShaderText, const char *fragShaderText);
    void DrawRotatedTriangleTest();
    void NewGenerateClearAndPrepareBufferCmds(XglImage *renderTarget);
    void NewGenerateBindStateAndPipelineCmds(XGL_PIPELINE* pipeline);


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


TEST_F(XglRenderTest, TestDrawTriangle1) {
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
       "uniform vec4 foo;\n"
       "void main() {\n"
       "   gl_FragColor = foo;\n"
       "}\n";
    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TestDrawTriangle2) {
    static const char *vertShaderText =
            "#version 330\n"
            "out vec4 color;\n"
            "out vec4 scale;\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   vec4 colors[3];\n"
            "      colors[0] = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "      colors[1] = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "      colors[2] = vec4(0.0, 0.0, 1.0, 1.0);\n"
            "   color = colors[int(mod(gl_VertexID, 3))];\n"
            "   scale = vec4(1.0, 1.0, 1.0, 1.0);\n"
            "   gl_Position = vec4(vertices[int(mod(gl_VertexID, 3))], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 430\n"
            "in vec4 color;\n"
            "in vec4 scale;\n"
            "layout(location = 0) uniform vec4 foo;\n"
            "void main() {\n"
            "   gl_FragColor = color * scale + foo;\n"
            "}\n";
    DrawTriangleTest(vertShaderText, fragShaderText);
}

TEST_F(XglRenderTest, TestDrawRotatedTriangle) {
    DrawRotatedTriangleTest();
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
