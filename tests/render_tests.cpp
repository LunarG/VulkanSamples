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
    void CreateDefaultPipeline(XGL_PIPELINE* pipeline, XGL_SHADER* vs, XGL_SHADER* ps, int width, int height);
    void GenerateClearAndPrepareBufferCmds(XglImage *renderTarget);
    void GenerateBindRenderTargetCmd(XglImage *renderTarget);
    void GenerateBindStateAndPipelineCmds(XGL_PIPELINE* pipeline);

    XGL_DEVICE device() {return m_device->device();}
    void CreateShader(XGL_PIPELINE_SHADER_STAGE stage, XGL_SHADER *pshader);
    void InitPipeline();
    void InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
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
    XGL_GPU_MEMORY m_vtxBufferMem;
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
        this->app_info.pAppName = (const XGL_CHAR *) "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = (const XGL_CHAR *) "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

        memset(&m_vtxBufferView, 0, sizeof(m_vtxBufferView));
        m_vtxBufferView.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;

        err = xglInitAndEnumerateGpus(&app_info, NULL,
                                      MAX_GPUS, &this->gpu_count, objs);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_GE(1, this->gpu_count) << "No GPU available";

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
    XGL_UINT data_size = sizeof(mem_req);
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

void XglRenderTest::CreateShader(XGL_PIPELINE_SHADER_STAGE stage, XGL_SHADER *pshader)
{
    struct icd_bil_header *pBIL;
    char * memblock;
    const char *kernel;
    size_t kernel_size;
    XGL_RESULT err;

    const XGL_PHYSICAL_GPU_PROPERTIES *props = &m_device->props;
    const int gen = (strstr((const char *) props->gpuName, "Sandybridge")) ? 6 : 7;

    if (stage == XGL_SHADER_STAGE_VERTEX) {
        if (gen == 6) {
            kernel = (const char *) gen6_vs;
            kernel_size = sizeof(gen6_vs);
        } else {
            kernel = (const char *) gen7_vs;
            kernel_size = sizeof(gen7_vs);
        }
    } else {
        if (gen == 6) {
            kernel = (const char *) gen6_fs;
            kernel_size = sizeof(gen6_fs);
        } else {
            kernel = (const char *) gen7_fs;
            kernel_size = sizeof(gen7_fs);
        }
    }

    memblock = new char [sizeof(*pBIL) + kernel_size];
    ASSERT_TRUE(memblock != NULL) << "memory allocation failed";

    pBIL = (struct icd_bil_header *) memblock;
    pBIL->magic = ICD_BIL_MAGIC;
    pBIL->version = ICD_BIL_VERSION;

    pBIL->gen_magic = (stage == XGL_SHADER_STAGE_VERTEX) ? 'v' : 'w';
    memcpy(pBIL + 1, kernel, kernel_size);

    XGL_SHADER_CREATE_INFO createInfo;
    XGL_SHADER shader;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.pCode = memblock;
    createInfo.codeSize = sizeof(*pBIL) + kernel_size;
    createInfo.flags = 0;
    err = xglCreateShader(device(), &createInfo, &shader);
    ASSERT_XGL_SUCCESS(err);

    delete[] memblock;

    *pshader = shader;
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

void XglRenderTest::CreateDefaultPipeline(XGL_PIPELINE* pipeline, XGL_SHADER* vs, XGL_SHADER* ps, int width, int height)
{
    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;
    //XGL_PIPELINE pipeline;

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

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_VERTEX, vs));

    vs_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.pNext = XGL_NULL_HANDLE;
    vs_stage.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs_stage.shader.shader = *vs;
    vs_stage.shader.descriptorSetMapping[0].descriptorCount = 0;
    vs_stage.shader.linkConstBufferCount = 0;
    vs_stage.shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    vs_stage.shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    vs_stage.shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    ASSERT_NO_FATAL_FAILURE(CreateShader(XGL_SHADER_STAGE_FRAGMENT, ps));

    ps_stage.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ps_stage.pNext = &vs_stage;
    ps_stage.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    ps_stage.shader.shader = *ps;
    // TODO: Do we need a descriptor set mapping for fragment?
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

    err = xglCreateGraphicsPipeline(device(), &info, pipeline);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderTest::GenerateClearAndPrepareBufferCmds(XglImage *renderTarget)
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

void XglRenderTest::GenerateBindRenderTargetCmd(XglImage *renderTarget)
{
    // bind render target
    XGL_COLOR_ATTACHMENT_BIND_INFO colorBind = {};
    colorBind.view  = renderTarget->targetView();
    colorBind.colorAttachmentState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
    xglCmdBindAttachments(m_cmdBuffer, 1, &colorBind, NULL );
}

void XglRenderTest::GenerateBindStateAndPipelineCmds(XGL_PIPELINE* pipeline)
{
    // set all states
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_RASTER, m_stateRaster );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_VIEWPORT, m_stateViewport );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_COLOR_BLEND, m_colorBlend);
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_DEPTH_STENCIL, m_stateDepthStencil );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_MSAA, m_stateMsaa );

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic memory view)
    xglCmdBindPipeline( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, *pipeline );
}

void XglRenderTest::DrawRotatedTriangleTest()
{
    // TODO : This test will pass a matrix into VS to affect triangle orientation.
}

void XglRenderTest::DrawTriangleTest()
{
    XGL_PIPELINE pipeline;
    XGL_SHADER vs, ps;
    XGL_RESULT err;
    int width = 256, height = 256;
    CreateDefaultPipeline(&pipeline, &vs, &ps, width, height);
    //ASSERT_XGL_SUCCESS(err);

    err = m_device->AllocAndBindGpuMemory(pipeline, "Pipeline", &m_pipe_mem);
    ASSERT_XGL_SUCCESS(err);

    /*
     * Shaders are now part of the pipeline, don't need these anymore
     */
    ASSERT_XGL_SUCCESS(xglDestroyObject(ps));
    ASSERT_XGL_SUCCESS(xglDestroyObject(vs));

    XGL_QUERY_POOL query;
    XGL_GPU_MEMORY query_mem;
    ASSERT_NO_FATAL_FAILURE(CreateQueryPool(XGL_QUERY_PIPELINE_STATISTICS, 1, &query, &query_mem));

    XglImage *renderTarget;
    XGL_FORMAT fmt = {
        XGL_CH_FMT_R8G8B8A8,
        XGL_NUM_FMT_UNORM
    };
    ASSERT_NO_FATAL_FAILURE(m_device->CreateImage(width, height, fmt,
                                                  XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
                                                  XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                  &renderTarget));

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds(renderTarget);
    GenerateBindRenderTargetCmd(renderTarget);
    GenerateBindStateAndPipelineCmds(&pipeline);

//    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
//    xglCmdBindDynamicMemoryView( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS,  &m_constantBufferView );

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

    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

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

TEST_F(XglRenderTest, TestDrawRotatedTriangle) {
    DrawRotatedTriangleTest();
}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    XglTestFramework::InitArgs(&argc, argv);

    result = RUN_ALL_TESTS();

    XglTestFramework::Finish();
    return result;
}
