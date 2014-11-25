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

    void DrawTriangleTest(const char *vertShaderText, const char *fragShaderText);
    void RotateTriangleVSUniform(glm::mat4 Projection, glm::mat4 View, glm::mat4 Model,
                                 XglConstantBufferObj constantBuffer);
    void GenericDrawTriangleTest(XglPipelineObj pipelineobj, XglDescriptorSetObj descriptorSet, int numTris);
    void QueueCommandBuffer(XGL_MEMORY_REF *memRefs, XGL_UINT32 numMemRefs);

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
    XglMemoryRefManager         m_memoryRefManager;


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


void XglRenderTest::DrawRotatedTriangleTest()
{
    // TODO : This test will pass a matrix into VS to affect triangle orientation.
}
void XglRenderTest::GenericDrawTriangleTest(XglPipelineObj pipelineobj, XglDescriptorSetObj descriptorSet,int numTris)
{
    XGL_PIPELINE pipeline; // Not really used but it's useful to keep original tests working to compare
    XGL_RESULT err = XGL_SUCCESS;

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

    pipelineobj.BindPipelineCommandBuffer(m_cmdBuffer,descriptorSet);
    descriptorSet.BindCommandBuffer(m_cmdBuffer);

    // render the triangle
    xglCmdDraw( m_cmdBuffer, 0, 3*numTris, 0, 1 );

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );
}

void XglRenderTest::QueueCommandBuffer(XGL_MEMORY_REF *memRefs, XGL_UINT32 numMemRefs)
{
    XGL_RESULT err = XGL_SUCCESS;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, numMemRefs, memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

void XglRenderTest::DrawTriangleTest(const char *vertShaderText, const char *fragShaderText)
{
    XGL_PIPELINE pipeline;
    XGL_RESULT err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);


    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Create descriptor set
    XglDescriptorSetObj descriptorSet(m_device);

    // Build command buffer
    err = xglBeginCommandBuffer(m_cmdBuffer, 0);
    ASSERT_XGL_SUCCESS(err);

    GenerateClearAndPrepareBufferCmds();
    GenerateBindRenderTargetCmd();
    GenerateBindStateAndPipelineCmds(&pipeline);

    pipelineobj.BindPipelineCommandBuffer(m_cmdBuffer,descriptorSet);
    descriptorSet.BindCommandBuffer(m_cmdBuffer);


    // render the triangle
    xglCmdDraw( m_cmdBuffer, 0, 3, 0, 1 );

    // finalize recording of the command buffer
    err = xglEndCommandBuffer( m_cmdBuffer );
    ASSERT_XGL_SUCCESS( err );

    // this command buffer only uses the vertex buffer memory
    m_numMemRefs = 0;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_numMemRefs, m_memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    RecordImage(m_renderTarget);

}

void XglRenderTest::RotateTriangleVSUniform(glm::mat4 Projection, glm::mat4 View, glm::mat4 Model,
                                            XglConstantBufferObj constantBuffer)
{
    int i;
    glm::mat4 MVP;
    int matrixSize = sizeof(MVP);
    XGL_RESULT err;

    for (i = 0; i < 8; i++) {
        XGL_UINT8 *pData;
        err = xglMapMemory(constantBuffer.m_constantBufferMem, 0, (XGL_VOID **) &pData);
        ASSERT_XGL_SUCCESS(err);

        Model = glm::rotate(Model, glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
        MVP = Projection * View * Model;
        memcpy(pData, (const void*) &MVP[0][0], matrixSize);

        err = xglUnmapMemory(constantBuffer.m_constantBufferMem);
        ASSERT_XGL_SUCCESS(err);

        // submit the command buffer to the universal queue
        err = xglQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer, m_memoryRefManager.GetNumRefs(), m_memoryRefManager.GetMemoryRefList(), NULL );
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

struct xgltriangle_vs_uniform {
    // Must start with MVP
    XGL_FLOAT   mvp[4][4];
    XGL_FLOAT   position[3][4];
    XGL_FLOAT   color[3][4];
};

void XglRenderTest::XGLTriangleTest(const char *vertShaderText, const char *fragShaderText)
{
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

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglConstantBufferObj constantBuffer(m_device, bufSize*2, sizeof(XGL_FLOAT), (const void*) &data);

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);
    vs.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &constantBuffer);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&constantBuffer);
    m_memoryRefManager.AddMemoryRef(&constantBuffer);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(m_memoryRefManager.GetMemoryRefList(), m_memoryRefManager.GetNumRefs());

    RotateTriangleVSUniform(Projection, View, Model, constantBuffer);

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

TEST_F(XglRenderTest, BIL_XGLTriangle)
{
    bool saved_use_bil = XglTestFramework::m_use_bil;

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

    TEST_DESCRIPTION("XGL-style shaders, but force test framework to compile shader to BIL and pass BIL to driver.");

    XglTestFramework::m_use_bil = true;

    XGLTriangleTest(vertShaderText, fragShaderText);

    XglTestFramework::m_use_bil = saved_use_bil;
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

    TEST_DESCRIPTION("Same shader as GreenTriangle, but compiles shader to BIL and gives BIL to driver.");

    XglTestFramework::m_use_bil = true;
    DrawTriangleTest(vertShaderText, fragShaderText);
    XglTestFramework::m_use_bil = saved_use_bil;
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

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    const int constantCount = 8;
    const float constants[constantCount] =  { 1.0, 0.0, 0.0, 1.0,
                                              0.0, 0.0, 1.0, 1.0 };
    XglConstantBufferObj constantBuffer(m_device,constantCount, sizeof(constants[0]), (const void*) constants);
    ps.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &constantBuffer);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&constantBuffer);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);

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


    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.SetMemoryState(m_cmdBuffer,XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY);

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&meshBuffer);

    XGL_VERTEX_INPUT_BINDING_DESCRIPTION vi_binding = {
         sizeof(g_vbData[0]),              // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
         XGL_VERTEX_INPUT_STEP_RATE_VERTEX // stepRate;       // Rate at which binding is incremented
    };

    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION vi_attribs[2];
    vi_attribs[0].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[0].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[0].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[0].offsetInBytes = 0;                 // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[1].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[1].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[1].offsetInBytes = 16;                 // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);
    pipelineobj.AddVertexDataBuffer(&meshBuffer,0);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 2);
    QueueCommandBuffer(NULL, 0);
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

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    // Create identity matrix
    glm::mat4 Projection    = glm::mat4(1.0f);
    glm::mat4 View          = glm::mat4(1.0f);
    glm::mat4 Model         = glm::mat4(1.0f);
    glm::mat4 MVP = Projection * View * Model;
    const int matrixSize = sizeof(MVP) / sizeof(MVP[0]);

    XglConstantBufferObj MVPBuffer(m_device, matrixSize, sizeof(MVP[0]), (const void*) &MVP[0][0]);
    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    vs.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &MVPBuffer);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    // Create descriptor set and attach the constant buffer to it
    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&MVPBuffer);

    m_memoryRefManager.AddMemoryRef(&MVPBuffer);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(m_memoryRefManager.GetMemoryRefList(), m_memoryRefManager.GetNumRefs());

    RotateTriangleVSUniform(Projection, View, Model, MVPBuffer);
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

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);
}

TEST_F(XglRenderTest, TriVertFetchAndVertID)
{
    // This tests that attributes work in the presence of gl_VertexID

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
            "   vec4 vertices[3];"
            "      vertices[gl_VertexID % 3] = pos;\n"
            "   gl_Position = vertices[(gl_VertexID + 3) % 3];\n"
            "}\n";


    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 color;\n"
            "void main() {\n"
            "   gl_FragColor = color;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.SetMemoryState(m_cmdBuffer,XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY);

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&meshBuffer);

    XGL_VERTEX_INPUT_BINDING_DESCRIPTION vi_binding = {
         sizeof(g_vbData[0]),              // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
         XGL_VERTEX_INPUT_STEP_RATE_VERTEX // stepRate;       // Rate at which binding is incremented
    };

    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION vi_attribs[2];
    vi_attribs[0].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[0].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[0].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[0].offsetInBytes = 0;                 // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[1].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[1].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[1].offsetInBytes = 16;                 // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);
    pipelineobj.AddVertexDataBuffer(&meshBuffer,0);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 2);
    QueueCommandBuffer(NULL, 0);
}

TEST_F(XglRenderTest, TriVertFetchDeadAttr)
{
    // This tests that attributes work in the presence of gl_VertexID
    // and a dead attribute in position 0. Draws a triangle with yellow,
    // red and green corners, starting at top and going clockwise.

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
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-1.0, -1.0);\n"
            "      vertices[1] = vec2( 1.0, -1.0);\n"
            "      vertices[2] = vec2( 0.0,  1.0);\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";


    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 color;\n"
            "void main() {\n"
            "   gl_FragColor = color;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.SetMemoryState(m_cmdBuffer,XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY);

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&meshBuffer);

    XGL_VERTEX_INPUT_BINDING_DESCRIPTION vi_binding = {
         sizeof(g_vbData[0]),              // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
         XGL_VERTEX_INPUT_STEP_RATE_VERTEX // stepRate;       // Rate at which binding is incremented
    };

    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION vi_attribs[2];
    vi_attribs[0].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[0].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[0].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[0].offsetInBytes = 0;                 // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = 0;                       // index into vertexBindingDescriptions
    vi_attribs[1].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    vi_attribs[1].format.numericFormat = XGL_NUM_FMT_FLOAT;
    vi_attribs[1].offsetInBytes = 16;                 // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);
    pipelineobj.AddVertexDataBuffer(&meshBuffer,0);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 2);
    QueueCommandBuffer(NULL, 0);

}

TEST_F(XglRenderTest, CubeWithVertexFetchAndMVP)
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
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    glm::mat4 View       = glm::lookAt(
                           glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                           glm::vec3(0,0,0), // and looks at the origin
                           glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                           );

    glm::mat4 Model = glm::mat4(1.0f);

    glm::mat4 MVP = Projection * View * Model;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitDepthStencil());

    XglConstantBufferObj meshBuffer(m_device,sizeof(g_vb_solid_face_colors_Data)/sizeof(g_vb_solid_face_colors_Data[0]),
            sizeof(g_vb_solid_face_colors_Data[0]), g_vb_solid_face_colors_Data);

    const int buf_size = sizeof(MVP) / sizeof(XGL_FLOAT);

    XglConstantBufferObj MVPBuffer(m_device, buf_size, sizeof(MVP[0]), (const void*) &MVP[0][0]);
    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    vs.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &MVPBuffer);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&MVPBuffer);

    m_memoryRefManager.AddMemoryRef(&meshBuffer);
    m_memoryRefManager.AddMemoryRef(&MVPBuffer);

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

    pipelineobj.AddVertexInputBindings(&vi_binding,1);
    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexDataBuffer(&meshBuffer,0);

    ClearDepthStencil(1.0f);
    GenericDrawTriangleTest(pipelineobj, descriptorSet, 12);

    QueueCommandBuffer(m_memoryRefManager.GetMemoryRefList(), m_memoryRefManager.GetNumRefs());

}

TEST_F(XglRenderTest, VSTexture)
{
    // The expected result from this test is a green and red triangle;
    // one red vertex on the left, two green vertices on the right.
    static const char *vertShaderText =
            "#version 130\n"
            "out vec4 texColor;\n"
            "uniform sampler2D surface;\n"
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
            "#version 130\n"
            "in vec4 texColor;\n"
            "void main() {\n"
            "   gl_FragColor = texColor;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);
    XglSamplerObj sampler(m_device);
    XglTextureObj texture(m_device);

    vs.BindShaderEntitySlotToImage(0, XGL_SLOT_SHADER_RESOURCE, &texture);
    vs.BindShaderEntitySlotToSampler(0, &sampler);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachImageView(&texture);
    descriptorSet.AttachSampler(&sampler);

    m_memoryRefManager.AddMemoryRef(&texture);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);

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
            "layout (location=0) out vec4 outColor;\n"
            "void main() {\n"
            "   vec4 texColor = textureLod(surface, samplePos, 0.0);\n"
            "   outColor = texColor;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);
    XglSamplerObj sampler(m_device);
    XglTextureObj texture(m_device);

    ps.BindShaderEntitySlotToImage(0, XGL_SLOT_SHADER_RESOURCE, &texture);
    ps.BindShaderEntitySlotToSampler(0, &sampler);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachImageView(&texture);
    descriptorSet.AttachSampler(&sampler);

    m_memoryRefManager.AddMemoryRef(&texture);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);
}
TEST_F(XglRenderTest, TexturedTriangleClip)
{
    // The expected result from this test is a red and green checkered triangle
    static const char *vertShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec2 samplePos;\n"
            "out gl_PerVertex {\n"
            "    vec4 gl_Position;\n"
            "    float gl_ClipDistance[1];\n"
            "};\n"
            "void main() {\n"
            "   vec2 vertices[3];"
            "      vertices[0] = vec2(-0.5, -0.5);\n"
            "      vertices[1] = vec2( 0.5, -0.5);\n"
            "      vertices[2] = vec2( 0.5,  0.5);\n"
            "   vec2 positions[3];"
            "      positions[0] = vec2( 0.0, 0.0);\n"
            "      positions[1] = vec2( 1.0, 0.0);\n"
            "      positions[2] = vec2( 1.0, 1.0);\n"
            "   float dists[3];\n"
            "      dists[0] = 1.0;\n"
            "      dists[1] = 1.0;\n"
            "      dists[2] = -1.0;\n"
            "   gl_ClipDistance[0] = dists[gl_VertexID % 3];\n"
            "   samplePos = positions[gl_VertexID % 3];\n"
            "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec2 samplePos;\n"
            "layout (binding = 0) uniform sampler2D surface;\n"
            "layout (location=0) out vec4 outColor;\n"
            "void main() {\n"
            //"   vec4 texColor = textureLod(surface, samplePos, 0.0 + gl_ClipDistance[0]);\n"
            "   vec4 texColor = textureLod(surface, samplePos, 0.0);\n"
            "   outColor = texColor;\n"
            "}\n";


    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);
    XglSamplerObj sampler(m_device);
    XglTextureObj texture(m_device);

    ps.BindShaderEntitySlotToImage(0, XGL_SLOT_SHADER_RESOURCE, &texture);
    ps.BindShaderEntitySlotToSampler(0, &sampler);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachImageView(&texture);
    descriptorSet.AttachSampler(&sampler);

    m_memoryRefManager.AddMemoryRef(&texture);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);
}
TEST_F(XglRenderTest, FSTriangle)
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
            "layout (location=0) out vec4 outColor;\n"
            "void main() {\n"
            "   vec4 texColor = textureLod(surface, samplePos, 0.0);\n"
            "   outColor = texColor;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);
    XglSamplerObj sampler(m_device);
    XglTextureObj texture(m_device);

    ps.BindShaderEntitySlotToImage(0, XGL_SLOT_SHADER_RESOURCE, &texture);
    ps.BindShaderEntitySlotToSampler(0, &sampler);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachImageView(&texture);
    descriptorSet.AttachSampler(&sampler);

    m_memoryRefManager.AddMemoryRef(&texture);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);
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
            "layout (binding = 12) uniform sampler2D surface2;\n"
            "void main() {\n"
            "   gl_FragColor = textureLod(surface2, samplePos.xy, 0.0);\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    XglSamplerObj sampler1(m_device);
    XglSamplerObj sampler2(m_device);
    XglSamplerObj sampler3(m_device);

    XglTextureObj texture1(m_device); // Red
    texture1.ChangeColors(0xffff0000,0xffff0000);
    XglTextureObj texture2(m_device); // Green
    texture2.ChangeColors(0xff00ff00,0xff00ff00);
    XglTextureObj texture3(m_device); // Blue
    texture3.ChangeColors(0xff0000ff,0xff0000ff);

    ps.BindShaderEntitySlotToImage(0, XGL_SLOT_SHADER_RESOURCE, &texture1);
    ps.BindShaderEntitySlotToSampler(0, &sampler1);

    ps.BindShaderEntitySlotToImage(1, XGL_SLOT_SHADER_RESOURCE, &texture2);
    ps.BindShaderEntitySlotToSampler(1, &sampler2);

    ps.BindShaderEntitySlotToImage(12, XGL_SLOT_SHADER_RESOURCE, &texture3);
    ps.BindShaderEntitySlotToSampler(12, &sampler3);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachImageView(&texture1);
    descriptorSet.AttachSampler(&sampler1);
    descriptorSet.AttachImageView(&texture2);
    descriptorSet.AttachSampler(&sampler2);
    descriptorSet.AttachImageView(&texture3);
    descriptorSet.AttachSampler(&sampler3);

    m_memoryRefManager.AddMemoryRef(&texture1);
    m_memoryRefManager.AddMemoryRef(&texture2);
    m_memoryRefManager.AddMemoryRef(&texture3);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);

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

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

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

    XglConstantBufferObj colorBuffer(m_device, valCount, sizeof(bufferVals[0]), (const void*) bufferVals);
    vs.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &colorBuffer);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&colorBuffer);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);

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
            "layout (std140, binding = 18) uniform whiteVal { vec4 color; } myWhiteVal\n;"
            "void main() {\n"
            "   gl_FragColor = myBlueVal.color;\n"
            "   gl_FragColor += myRedVal.color;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    // We're going to create a number of uniform buffers, and then allow
    // the shader to select which it wants to read from with a binding

    // Let's populate the buffers with a single color each:
    //    layout (std140, binding = 0) uniform bufferVals { vec4 red;   } myRedVal;
    //    layout (std140, binding = 1) uniform bufferVals { vec4 green; } myGreenVal;
    //    layout (std140, binding = 2) uniform bufferVals { vec4 blue;  } myBlueVal;
    //    layout (std140, binding = 18) uniform bufferVals { vec4 white; } myWhiteVal;

    const float redVals[4]   = { 1.0, 0.0, 0.0, 1.0 };
    const float greenVals[4] = { 0.0, 1.0, 0.0, 1.0 };
    const float blueVals[4]  = { 0.0, 0.0, 1.0, 1.0 };
    const float whiteVals[4] = { 1.0, 1.0, 1.0, 1.0 };

    const int redCount   = sizeof(redVals)   / sizeof(float);
    const int greenCount = sizeof(greenVals) / sizeof(float);
    const int blueCount  = sizeof(blueVals)  / sizeof(float);
    const int whiteCount = sizeof(whiteVals) / sizeof(float);

    XglConstantBufferObj redBuffer(m_device, redCount, sizeof(redVals[0]), (const void*) redVals);
    ps.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &redBuffer);

    XglConstantBufferObj greenBuffer(m_device, greenCount, sizeof(greenVals[0]), (const void*) greenVals);
    ps.BindShaderEntitySlotToMemory(1, XGL_SLOT_SHADER_RESOURCE, &greenBuffer);

    XglConstantBufferObj blueBuffer(m_device, blueCount, sizeof(blueVals[0]), (const void*) blueVals);
    ps.BindShaderEntitySlotToMemory(2, XGL_SLOT_SHADER_RESOURCE, &blueBuffer);

    XglConstantBufferObj whiteBuffer(m_device, whiteCount, sizeof(whiteVals[0]), (const void*) whiteVals);
    ps.BindShaderEntitySlotToMemory(18, XGL_SLOT_SHADER_RESOURCE, &whiteBuffer);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&redBuffer);
    descriptorSet.AttachMemoryView(&greenBuffer);
    descriptorSet.AttachMemoryView(&blueBuffer);
    descriptorSet.AttachMemoryView(&whiteBuffer);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);

}

TEST_F(XglRenderTest, TriangleFSAnonymousUniformBlockBinding)
{
    // This test is the same as TriangleFSUniformBlockBinding, but
    // it does not provide an instance name.
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
            "#version 430\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) uniform vec4 foo;\n"
            "layout (location = 1) uniform vec4 bar;\n"
            "layout (std140, binding = 0) uniform redVal   { vec4 red; };"
            "layout (std140, binding = 1) uniform greenVal { vec4 green; };"
            "layout (std140, binding = 2) uniform blueVal  { vec4 blue; };"
            "layout (std140, binding = 18) uniform whiteVal { vec4 white; };"
            "void main() {\n"
            "   gl_FragColor = blue;\n"
            "   gl_FragColor += red;\n"
            "}\n";
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);

    // We're going to create a number of uniform buffers, and then allow
    // the shader to select which it wants to read from with a binding

    // Let's populate the buffers with a single color each:
    //    layout (std140, binding = 0) uniform bufferVals { vec4 red;   } myRedVal;
    //    layout (std140, binding = 1) uniform bufferVals { vec4 green; } myGreenVal;
    //    layout (std140, binding = 2) uniform bufferVals { vec4 blue;  } myBlueVal;
    //    layout (std140, binding = 3) uniform bufferVals { vec4 white; } myWhiteVal;

    const float redVals[4]   = { 1.0, 0.0, 0.0, 1.0 };
    const float greenVals[4] = { 0.0, 1.0, 0.0, 1.0 };
    const float blueVals[4]  = { 0.0, 0.0, 1.0, 1.0 };
    const float whiteVals[4] = { 1.0, 1.0, 1.0, 1.0 };

    const int redCount   = sizeof(redVals)   / sizeof(float);
    const int greenCount = sizeof(greenVals) / sizeof(float);
    const int blueCount  = sizeof(blueVals)  / sizeof(float);
    const int whiteCount = sizeof(whiteVals) / sizeof(float);

    XglConstantBufferObj redBuffer(m_device, redCount, sizeof(redVals[0]), (const void*) redVals);
    ps.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &redBuffer);

    XglConstantBufferObj greenBuffer(m_device, greenCount, sizeof(greenVals[0]), (const void*) greenVals);
    ps.BindShaderEntitySlotToMemory(1, XGL_SLOT_SHADER_RESOURCE, &greenBuffer);

    XglConstantBufferObj blueBuffer(m_device, blueCount, sizeof(blueVals[0]), (const void*) blueVals);
    ps.BindShaderEntitySlotToMemory(2, XGL_SLOT_SHADER_RESOURCE, &blueBuffer);

    XglConstantBufferObj whiteBuffer(m_device, whiteCount, sizeof(whiteVals[0]), (const void*) whiteVals);
    ps.BindShaderEntitySlotToMemory(3, XGL_SLOT_SHADER_RESOURCE, &whiteBuffer);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AttachMemoryView(&redBuffer);
    descriptorSet.AttachMemoryView(&greenBuffer);
    descriptorSet.AttachMemoryView(&blueBuffer);
    descriptorSet.AttachMemoryView(&whiteBuffer);

    GenericDrawTriangleTest(pipelineobj, descriptorSet, 1);
    QueueCommandBuffer(NULL, 0);

}

TEST_F(XglRenderTest, CubeWithVertexFetchAndMVPAndTexture)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding=0) uniform bufferVals {\n"
            "    mat4 mvp;\n"
            "} myBufferVals;\n"
            "layout (location=0) in vec4 pos;\n"
            "layout (location=0) out vec2 UV;\n"
            "void main() {\n"
            "   vec2 positions[3];"
            "      positions[0] = vec2( 0.0, 0.0);\n"
            "      positions[1] = vec2( 0.25, 0.1);\n"
            "      positions[2] = vec2( 0.1, 0.25);\n"
            "   UV = positions[gl_VertexID % 3];\n"
            "   gl_Position = myBufferVals.mvp * pos;\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (binding=0) uniform sampler2D surface;\n"
            "layout (location=0) out vec4 outColor;\n"
            "layout (location=0) in vec2 UV;\n"
            "void main() {\n"
            "    outColor= textureLod(surface, UV, 0.0);\n"
            "}\n";
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    glm::mat4 View       = glm::lookAt(
                           glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                           glm::vec3(0,0,0), // and looks at the origin
                           glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                           );

    glm::mat4 Model = glm::mat4(1.0f);

    glm::mat4 MVP = Projection * View * Model;


    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitDepthStencil());

    XglConstantBufferObj meshBuffer(m_device,sizeof(g_vb_solid_face_colors_Data)/sizeof(g_vb_solid_face_colors_Data[0]),
            sizeof(g_vb_solid_face_colors_Data[0]), g_vb_solid_face_colors_Data);
    meshBuffer.SetMemoryState(m_cmdBuffer,XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY);


    const int buf_size = sizeof(MVP) / sizeof(XGL_FLOAT);

    XglConstantBufferObj mvpBuffer(m_device, buf_size, sizeof(MVP[0]), (const void*) &MVP[0][0]);
    XglShaderObj vs(m_device,vertShaderText,XGL_SHADER_STAGE_VERTEX );
    XglShaderObj ps(m_device,fragShaderText, XGL_SHADER_STAGE_FRAGMENT);
    XglSamplerObj sampler(m_device);
    XglTextureObj texture(m_device);

    // vs.BindShaderEntitySlotToMemory(0, XGL_SLOT_VERTEX_INPUT, (XGL_OBJECT) &meshBuffer.m_constantBufferView);
    vs.BindShaderEntitySlotToMemory(0, XGL_SLOT_SHADER_RESOURCE, &mvpBuffer);
    ps.BindShaderEntitySlotToImage(0, XGL_SLOT_SHADER_RESOURCE, &texture);
    ps.BindShaderEntitySlotToSampler(0, &sampler);

    XglPipelineObj pipelineobj(m_device);
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    XglDescriptorSetObj descriptorSet(m_device);

    descriptorSet.AttachMemoryView(&mvpBuffer);
    descriptorSet.AttachImageView(&texture);
    descriptorSet.AttachSampler(&sampler);

    m_memoryRefManager.AddMemoryRef(&meshBuffer);
    m_memoryRefManager.AddMemoryRef(&mvpBuffer);
    m_memoryRefManager.AddMemoryRef(&texture);

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

    pipelineobj.AddVertexInputBindings(&vi_binding,1);
    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexDataBuffer(&meshBuffer,0);

    ClearDepthStencil(1.0f);
    GenericDrawTriangleTest(pipelineobj, descriptorSet, 12);

    QueueCommandBuffer(m_memoryRefManager.GetMemoryRefList(), m_memoryRefManager.GetNumRefs());

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
