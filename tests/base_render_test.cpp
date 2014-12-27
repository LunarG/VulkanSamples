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


// Base rendering test
// The intend of this test is to have all the necessary steps to render
// a textured triangle in straight-line code to simplify debugging

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

#include "xgltestframework.h"

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

class XglRenderTest : public XglTestFramework
{
public:
    void CreateQueryPool(XGL_QUERY_TYPE type, XGL_UINT slots,
                         XGL_QUERY_POOL *pPool, XGL_GPU_MEMORY *pMem);
    void DestroyQueryPool(XGL_QUERY_POOL pool, XGL_GPU_MEMORY mem);

    XGL_DEVICE device() {return m_device->device();}
    void InitPipeline();
    void InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
    void InitConstantBuffer( int constantCount, int constantSize, const void* data );
    void DrawTriangleTest();
    void DrawRotatedTriangleTest();

protected:
    XGL_APPLICATION_INFO app_info;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count;
    XGL_GPU_MEMORY      m_descriptor_set_mem;
    XGL_GPU_MEMORY      m_pipe_mem;
    XglDevice *m_device;
    XGL_CMD_BUFFER m_cmdBuffer;
    XGL_UINT32 m_numVertices;
    XGL_MEMORY_VIEW_ATTACH_INFO m_vtxBufferView;
    XGL_MEMORY_VIEW_ATTACH_INFO m_constantBufferView;
    XGL_GPU_MEMORY m_vtxBufferMem;
    XGL_GPU_MEMORY m_constantBufferMem;
    XGL_UINT32                      m_numMemRefs;
    XGL_MEMORY_REF                  m_memRefs[5];
    XGL_RASTER_STATE_OBJECT         m_stateRaster;
    XGL_COLOR_BLEND_STATE_OBJECT    m_colorBlend;
    XGL_VIEWPORT_STATE_OBJECT       m_stateViewport;
    XGL_DEPTH_STENCIL_STATE_OBJECT  m_stateDepthStencil;
    XGL_MSAA_STATE_OBJECT           m_stateMsaa;
    XGL_DESCRIPTOR_SET              m_rsrcDescSet;

    virtual void SetUp() {
        XGL_RESULT err;

        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

        memset(&m_vtxBufferView, 0, sizeof(m_vtxBufferView));
        m_vtxBufferView.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;

        memset(&m_constantBufferView, 0, sizeof(m_constantBufferView));
        m_constantBufferView.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;

        err = xglInitAndEnumerateGpus(&app_info, NULL,
                                      MAX_GPUS, &this->gpu_count, objs);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_GE(this->gpu_count, 1) << "No GPU available";

        m_device = new XglDevice(0, objs[0]);
        m_device->get_device_queue();
    }

    virtual void TearDown() {
        xglInitAndEnumerateGpus(&this->app_info, XGL_NULL_HANDLE, 0, &gpu_count, XGL_NULL_HANDLE);
    }
};


void XglRenderTest::CreateQueryPool(XGL_QUERY_TYPE type, XGL_UINT slots,
                                    XGL_QUERY_POOL *pPool, XGL_GPU_MEMORY *pMem)
{
    XGL_RESULT err;

    XGL_QUERY_POOL_CREATE_INFO poolCreateInfo = {};
    poolCreateInfo.sType = XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    poolCreateInfo.pNext = NULL;
    poolCreateInfo.queryType = type;
    poolCreateInfo.slots = slots;

    err = xglCreateQueryPool(device(), &poolCreateInfo, pPool);
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_SIZE data_size = sizeof(mem_req);
    err = xglGetObjectInfo(*pPool, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_EQ(data_size, sizeof(mem_req));

    if (!mem_req.size) {
        *pMem = XGL_NULL_HANDLE;
        return;
    }

    XGL_MEMORY_ALLOC_INFO mem_info;

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = mem_req.size;
    mem_info.alignment = mem_req.alignment;
    mem_info.heapCount = mem_req.heapCount;
    memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);
    mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    mem_info.flags = XGL_MEMORY_ALLOC_SHAREABLE_BIT;
    err = xglAllocMemory(device(), &mem_info, pMem);
    ASSERT_XGL_SUCCESS(err);

    err = xglBindObjectMemory(*pPool, *pMem, 0);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::DestroyQueryPool(XGL_QUERY_POOL pool, XGL_GPU_MEMORY mem)
{
    ASSERT_XGL_SUCCESS(xglBindObjectMemory(pool, XGL_NULL_HANDLE, 0));
    ASSERT_XGL_SUCCESS(xglFreeMemory(mem));
    ASSERT_XGL_SUCCESS(xglDestroyObject(pool));
}


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

void XglRenderTest::InitConstantBuffer(int constantCount, int constantSize, const void* data)
{
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

    err = xglAllocMemory(device(), &alloc_info, &m_constantBufferMem);
    ASSERT_XGL_SUCCESS(err);

    err = xglMapMemory(m_constantBufferMem, 0, (XGL_VOID **) &pData);
    ASSERT_XGL_SUCCESS(err);

    memcpy(pData, data, alloc_info.allocationSize);

    err = xglUnmapMemory(m_constantBufferMem);
    ASSERT_XGL_SUCCESS(err);

    // set up the memory view for the constant buffer
    this->m_constantBufferView.stride = 16;
    this->m_constantBufferView.range  = alloc_info.allocationSize;
    this->m_constantBufferView.offset = 0;
    this->m_constantBufferView.mem    = m_constantBufferMem;
    this->m_constantBufferView.format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    this->m_constantBufferView.format.numericFormat = XGL_NUM_FMT_FLOAT;
}

void XglRenderTest::DrawRotatedTriangleTest()
{
    // TODO : This test will pass a matrix into VS to affect triangle orientation.
}

/**
 * DrawTriangleTest deliberately calls each necessary xgl call to
 * make it clear what is necessary to render and simplify debugging.
 */
void XglRenderTest::DrawTriangleTest()
{
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;
    std::vector<unsigned int> bil;
    XGL_SHADER_CREATE_INFO createInfo;
    size_t shader_len;
    XGL_IMAGE                    m_image;
    XGL_COLOR_ATTACHMENT_VIEW    m_targetView;
    XGL_GPU_MEMORY               m_memory;

    int width = 256, height = 256;

    // create a raster state (solid, back-face culling)
    XGL_RASTER_STATE_CREATE_INFO raster = {};
    raster.sType = XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO;
    raster.fillMode = XGL_FILL_SOLID;
    raster.cullMode = XGL_CULL_NONE;
    raster.frontFace = XGL_FRONT_FACE_CCW;
    err = xglCreateRasterState( device(), &raster, &m_stateRaster );
    ASSERT_XGL_SUCCESS(err);

    XGL_VIEWPORT_STATE_CREATE_INFO viewport = {};
    viewport.viewportCount         = 1;
    viewport.scissorEnable         = XGL_FALSE;
    viewport.viewports[0].originX  = 0;
    viewport.viewports[0].originY  = 0;
    viewport.viewports[0].width    = 1.f * width;
    viewport.viewports[0].height   = 1.f * height;
    viewport.viewports[0].minDepth = 0.f;
    viewport.viewports[0].maxDepth = 1.f;

    err = xglCreateViewportState( device(), &viewport, &m_stateViewport );
    ASSERT_XGL_SUCCESS( err );

    XGL_COLOR_BLEND_STATE_CREATE_INFO blend = {};
    blend.sType = XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO;
    err = xglCreateColorBlendState(device(), &blend, &m_colorBlend);
    ASSERT_XGL_SUCCESS( err );

    XGL_DEPTH_STENCIL_STATE_CREATE_INFO depthStencil = {};
    depthStencil.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable      = XGL_FALSE;
    depthStencil.depthWriteEnable = XGL_FALSE;
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

    XGL_MSAA_STATE_CREATE_INFO msaa = {};
    msaa.sType = XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO;
    msaa.sampleMask = 1;
    msaa.samples = 1;

    err = xglCreateMsaaState( device(), &msaa, &m_stateMsaa );
    ASSERT_XGL_SUCCESS( err );

    XGL_CMD_BUFFER_CREATE_INFO cmdInfo = {};

    cmdInfo.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmdInfo.queueType = XGL_QUEUE_TYPE_GRAPHICS;
    err = xglCreateCommandBuffer(device(), &cmdInfo, &m_cmdBuffer);
    ASSERT_XGL_SUCCESS(err) << "xglCreateCommandBuffer failed";

#if 0
    // Create descriptor set for our one resource
    XGL_DESCRIPTOR_SET_CREATE_INFO descriptorInfo = {};
    descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    descriptorInfo.slots = 1; // Vertex buffer only

    // create a descriptor set with a single slot
    err = xglCreateDescriptorSet( device(), &descriptorInfo, &m_rsrcDescSet );
    ASSERT_XGL_SUCCESS(err) << "xglCreateDescriptorSet failed";

    // bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    // set up the memory view for the vertex buffer
    this->m_vtxBufferView.stride = vbStride;
    this->m_vtxBufferView.range  = numVertices * vbStride;
    this->m_vtxBufferView.offset = 0;
    this->m_vtxBufferView.mem    = m_vtxBufferMem;
    this->m_vtxBufferView.format.channelFormat = XGL_CH_FMT_UNDEFINED;
    this->m_vtxBufferView.format.numericFormat = XGL_NUM_FMT_UNDEFINED;
    // write the vertex buffer view to the descriptor set
    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglAttachMemoryViewDescriptors( m_rsrcDescSet, 0, 1, &m_vtxBufferView );
    xglEndDescriptorSetUpdate( m_rsrcDescSet );
#endif

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

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    shader_len = strlen(vertShaderText);
    createInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
    createInfo.pCode = malloc(createInfo.codeSize);
    createInfo.flags = 0;

    /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
    ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
    ((uint32_t *) createInfo.pCode)[1] = 0;
    ((uint32_t *) createInfo.pCode)[2] = XGL_SHADER_STAGE_VERTEX;
    memcpy(((uint32_t *) createInfo.pCode + 3), vertShaderText, shader_len + 1);

    err = xglCreateShader(device(), &createInfo, &vs);
    if (err) {
        free((void *) createInfo.pCode);

        // Use Reference GLSL to BIL compiler
        GLSLtoBIL(XGL_SHADER_STAGE_VERTEX, vertShaderText, bil);
        createInfo.pCode = bil.data();
        createInfo.codeSize = bil.size() * sizeof(unsigned int);
        createInfo.flags = 0;
        err = xglCreateShader(device(), &createInfo, &vs);
    }
    ASSERT_XGL_SUCCESS(err);

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

    static const char *fragShaderText =
            "#version 130\n"
            "void main() {\n"
            "  gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
            "}\n";

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    shader_len = strlen(fragShaderText);
    createInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
    createInfo.pCode = malloc(createInfo.codeSize);
    createInfo.flags = 0;

    /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
    ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
    ((uint32_t *) createInfo.pCode)[1] = 0;
    ((uint32_t *) createInfo.pCode)[2] = XGL_SHADER_STAGE_FRAGMENT;
    memcpy(((uint32_t *) createInfo.pCode + 3), fragShaderText, shader_len + 1);

    err = xglCreateShader(device(), &createInfo, &ps);
    if (err) {
        free((void *) createInfo.pCode);

        // Use Reference GLSL to BIL compiler
        GLSLtoBIL(XGL_SHADER_STAGE_FRAGMENT, fragShaderText, bil);
        createInfo.pCode = bil.data();
        createInfo.codeSize = bil.size() * sizeof(unsigned int);
        createInfo.flags = 0;
        err = xglCreateShader(device(), &createInfo, &ps);
    }
    ASSERT_XGL_SUCCESS(err);

    memset(&ps_stage, 0, sizeof(ps_stage));
    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = ps;

    const int slots = 1;
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( slots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    slotInfo[0].shaderEntityIndex = 0;
    slotInfo[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;

    ps_stage.shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    ps_stage.shader.descriptorSetMapping[0].descriptorCount = 1;

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
                {XGL_CH_FMT_R8G8B8A8, XGL_NUM_FMT_UNORM},   // XGL_FORMAT
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

    err = m_device->AllocAndBindGpuMemory(pipeline, "Pipeline", &m_pipe_mem);
    ASSERT_XGL_SUCCESS(err);

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    XglImage *renderTarget;
    XGL_FORMAT fmt = {
        XGL_CH_FMT_R8G8B8A8,
        XGL_NUM_FMT_UNORM
    };
    renderTarget = new XglImage(m_device);
    renderTarget->init(width, height, fmt, XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
                       XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
//    ASSERT_NO_FATAL_FAILURE(m_device->CreateImage(width, height, fmt,
//                                                  XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
//                                                  XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
//                                                  &renderTarget));
    {
        XGL_UINT mipCount;
        XGL_SIZE size = sizeof(XGL_FORMAT_PROPERTIES);
        XGL_FORMAT_PROPERTIES image_fmt;

        mipCount = 0;

        XGL_UINT _w = width;
        XGL_UINT _h = height;
        while( ( _w > 0 ) || ( _h > 0 ) )
        {
            _w >>= 1;
            _h >>= 1;
            mipCount++;
        }

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
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
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
    //    imageCreateInfo.usage = XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT | XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imageCreateInfo.usage = XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
                                XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //    XGL_RESULT XGLAPI xglCreateImage(
        //        XGL_DEVICE                                  device,
        //        const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
        //        XGL_IMAGE*                                  pImage);
        err = xglCreateImage(device(), &imageCreateInfo, &m_image);
        ASSERT_XGL_SUCCESS(err);

        XGL_MEMORY_REQUIREMENTS mem_req;
        XGL_SIZE data_size = sizeof(XGL_MEMORY_REQUIREMENTS);
        err = xglGetObjectInfo(m_image, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
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
        err = xglAllocMemory(device(), &mem_info, &m_memory);
        ASSERT_XGL_SUCCESS(err);

        err = xglBindObjectMemory(m_image, m_memory, 0);
        ASSERT_XGL_SUCCESS(err);

        XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO createView = {
            XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            XGL_NULL_HANDLE,
            m_image,
            fmt,
            0,
            0,
            1
        };

        err = xglCreateColorAttachmentView(device(), &createView, &m_targetView);
        ASSERT_XGL_SUCCESS(err);
    }

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

    XGL_QUERY_POOL query;
    XGL_GPU_MEMORY query_mem;
    ASSERT_NO_FATAL_FAILURE(CreateQueryPool(XGL_QUERY_PIPELINE_STATISTICS, 1, &query, &query_mem));

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

//    GenerateClearAndPrepareBufferCmds(renderTarget);
    {
        // whatever we want to do, we do it to the whole buffer
        XGL_IMAGE_SUBRESOURCE_RANGE srRange = {};
        srRange.aspect = XGL_IMAGE_ASPECT_COLOR;
        srRange.baseMipLevel = 0;
        srRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
        srRange.baseArraySlice = 0;
        srRange.arraySize = XGL_LAST_MIP_OR_SLICE;

        // prepare the whole back buffer for clear
        XGL_IMAGE_STATE_TRANSITION transitionToClear = {};
        transitionToClear.image = renderTarget->image();
        transitionToClear.oldState = renderTarget->state();
        transitionToClear.newState = XGL_IMAGE_STATE_CLEAR;
        transitionToClear.subresourceRange = srRange;
        xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToClear );
        renderTarget->state(( XGL_IMAGE_STATE ) transitionToClear.newState);

        // clear the back buffer to dark grey
        XGL_UINT clearColor[4] = {64, 64, 64, 0};
        xglCmdClearColorImageRaw( m_cmdBuffer, renderTarget->image(), clearColor, 1, &srRange );

        // prepare back buffer for rendering
        XGL_IMAGE_STATE_TRANSITION transitionToRender = {};
        transitionToRender.image = renderTarget->image();
        transitionToRender.oldState = renderTarget->state();
        transitionToRender.newState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
        transitionToRender.subresourceRange = srRange;
        xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToRender );
        renderTarget->state(( XGL_IMAGE_STATE ) transitionToClear.newState);
    }

    {
        // bind render target
        XGL_COLOR_ATTACHMENT_BIND_INFO colorBind = {};
        colorBind.view  = renderTarget->targetView();
        colorBind.colorAttachmentState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
        xglCmdBindAttachments(m_cmdBuffer, 1, &colorBind, NULL );
    }

    {
        // set all states
        xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_RASTER, m_stateRaster );
        xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_VIEWPORT, m_stateViewport );
        xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_COLOR_BLEND, m_colorBlend);
        xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_DEPTH_STENCIL, m_stateDepthStencil );
        xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_MSAA, m_stateMsaa );

        // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic memory view)
        xglCmdBindPipeline( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
        xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
    }

    xglCmdResetQueryPool(m_cmdBuffer, query, 0, 1);
    xglCmdBeginQuery(m_cmdBuffer, query, 0, 0);

    // render the cube
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    xglCmdEndQuery(m_cmdBuffer, query, 0);

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

    XGL_PIPELINE_STATISTICS_DATA stats;
    XGL_SIZE stats_size = sizeof(stats);
    err = xglGetQueryPoolResults(query, 0, 1, &stats_size, &stats);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(stats_size, sizeof(stats));

    ASSERT_EQ(stats.vsInvocations, 3);
    ASSERT_EQ(stats.cPrimitives, 1);
    ASSERT_EQ(stats.cInvocations, 1);

    DestroyQueryPool(query, query_mem);

//    renderTarget->WritePPM(test_info->test_case_name());
//    m_screen.Display(renderTarget, m_image_mem);
    RecordImage(renderTarget);

    ASSERT_XGL_SUCCESS(xglDestroyObject(pipeline));
    ASSERT_XGL_SUCCESS(xglDestroyObject(m_cmdBuffer));
    ASSERT_XGL_SUCCESS(xglDestroyObject(m_stateRaster));
    ASSERT_XGL_SUCCESS(xglDestroyObject(m_stateViewport));
    ASSERT_XGL_SUCCESS(xglDestroyObject(m_stateDepthStencil));
    ASSERT_XGL_SUCCESS(xglDestroyObject(m_stateMsaa));
    free(renderTarget);
}


TEST_F(XglRenderTest, TestDrawTriangle) {
    DrawTriangleTest();
}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    XglTestFramework::InitArgs(&argc, argv);

    ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    XglTestFramework::Finish();
    return result;
}
