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


//  VK tests
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

#include <vulkan.h>
#ifdef DUMP_STATE_DOT
#include "../layers/draw_state.h"
#endif
#ifdef PRINT_OBJECTS
#include "../layers/object_track.h"
#endif
#ifdef DEBUG_CALLBACK
#include <vk_debug_report_lunarg.h>
#endif
#include "gtest-1.7.0/include/gtest/gtest.h"

#include "icd-spv.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "vkrenderframework.h"
#ifdef DEBUG_CALLBACK
void VKAPI myDbgFunc(
    VK_DBG_MSG_TYPE     msgType,
    VkValidationLevel validationLevel,
    VkObject             srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData)
{
    switch (msgType)
    {
        case VK_DBG_MSG_WARNING:
            printf("CALLBACK WARNING : %s\n", pMsg);
            break;
        case VK_DBG_REPORT_ERROR_BIT:
            printf("CALLBACK ERROR : %s\n", pMsg);
            break;
        default:
            printf("EATING Msg of type %u\n", msgType);
            break;
    }
}
#endif


#undef ASSERT_NO_FATAL_FAILURE
#define ASSERT_NO_FATAL_FAILURE(x) x

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
struct Vertex
{
    float posX, posY, posZ, posW;    // Position data
    float r, g, b, a;                // Color
};

struct VertexUV
{
    float posX, posY, posZ, posW;    // Position data
    float u, v;                      // texture u,v
};

#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f
#define UV(_u_, _v_)                (_u_), (_v_)

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
    { XYZ1(  1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1(  1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
    { XYZ1(  1,  1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },

    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1(  1, -1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1(  1, -1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },
    { XYZ1(  1,  1,  1 ), XYZ1( 0.f, 1.f, 0.f ) },

    { XYZ1(  1,  1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1(  1,  1, -1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1(  1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1(  1, -1,  1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1(  1,  1, -1 ), XYZ1( 0.f, 0.f, 1.f ) },
    { XYZ1(  1, -1, -1 ), XYZ1( 0.f, 0.f, 1.f ) },

    { XYZ1( -1,  1,  1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 1.f, 1.f, 0.f ) },
    { XYZ1( -1, -1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },

    { XYZ1(  1,  1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1(  1,  1, -1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1(  1,  1, -1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( -1,  1,  1 ), XYZ1( 1.f, 0.f, 1.f ) },
    { XYZ1( -1,  1, -1 ), XYZ1( 1.f, 0.f, 1.f ) },

    { XYZ1( 1, -1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( 1, -1, -1 ), XYZ1( 0.f, 1.f, 1.f ) },
    { XYZ1( -1, -1, -1 ), XYZ1( 0.f, 1.f, 1.f ) },
};

static const VertexUV g_vb_texture_Data[] =
{
    { XYZ1( -1, -1, -1 ), UV( 0.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), UV( 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), UV( 1.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), UV( 1.f, 1.f ) },
    { XYZ1( -1, -1, -1 ), UV( 0.f, 0.f ) },
    { XYZ1( -1,  1, -1 ), UV( 0.f, 1.f ) },

    { XYZ1( -1, -1, -1 ), UV( 1.f, 0.f ) },
    { XYZ1(  1, -1, -1 ), UV( 0.f, 0.f ) },
    { XYZ1(  1,  1, -1 ), UV( 0.f, 1.f ) },
    { XYZ1( -1, -1, -1 ), UV( 1.f, 0.f ) },
    { XYZ1(  1,  1, -1 ), UV( 0.f, 1.f ) },
    { XYZ1( -1,  1, -1 ), UV( 1.f, 1.f ) },

    { XYZ1( -1, -1, -1 ), UV( 1.f, 1.f ) },
    { XYZ1(  1, -1,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  1, -1, -1 ), UV( 1.f, 0.f ) },
    { XYZ1( -1, -1, -1 ), UV( 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), UV( 0.f, 1.f ) },
    { XYZ1(  1, -1,  1 ), UV( 0.f, 0.f ) },

    { XYZ1( -1,  1, -1 ), UV( 1.f, 1.f ) },
    { XYZ1(  1,  1,  1 ), UV( 0.f, 0.f ) },
    { XYZ1( -1,  1,  1 ), UV( 0.f, 1.f ) },
    { XYZ1( -1,  1, -1 ), UV( 1.f, 1.f ) },
    { XYZ1(  1,  1, -1 ), UV( 1.f, 0.f ) },
    { XYZ1(  1,  1,  1 ), UV( 0.f, 0.f ) },

    { XYZ1(  1,  1, -1 ), UV( 1.f, 1.f ) },
    { XYZ1(  1, -1,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  1,  1,  1 ), UV( 0.f, 1.f ) },
    { XYZ1(  1, -1,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  1,  1, -1 ), UV( 1.f, 1.f ) },
    { XYZ1(  1, -1, -1 ), UV( 1.f, 0.f ) },

    { XYZ1( -1,  1,  1 ), UV( 0.f, 1.f ) },
    { XYZ1(  1,  1,  1 ), UV( 1.f, 1.f ) },
    { XYZ1( -1, -1,  1 ), UV( 0.f, 0.f ) },
    { XYZ1( -1, -1,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  1,  1,  1 ), UV( 1.f, 1.f ) },
    { XYZ1(  1, -1,  1 ), UV( 1.f, 0.f ) },
};

class VkRenderTest : public VkRenderFramework
{
public:

    void RotateTriangleVSUniform(glm::mat4 Projection, glm::mat4 View, glm::mat4 Model,
                                 VkConstantBufferObj *constantBuffer, VkCommandBufferObj *cmdBuffer);
    void GenericDrawPreparation(VkCommandBufferObj *cmdBuffer, VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet);
    void GenericDrawPreparation(VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet)
             { GenericDrawPreparation(m_cmdBuffer, pipelineobj, descriptorSet); }
    void InitDepthStencil();
    void VKTriangleTest(const char *vertShaderText, const char *fragShaderText, const bool rotate);

    VkResult BeginCommandBuffer(VkCommandBufferObj &cmdBuffer);
    VkResult BeginCommandBuffer(VkCommandBufferObj &cmdBuffer, VkCmdBufferBeginInfo *beginInfo);
    VkResult EndCommandBuffer(VkCommandBufferObj &cmdBuffer);
    /* Convenience functions that use built-in command buffer */
    VkResult BeginCommandBuffer() { return BeginCommandBuffer(*m_cmdBuffer); }
    VkResult BeginCommandBuffer(VkCmdBufferBeginInfo *beginInfo) { return BeginCommandBuffer(*m_cmdBuffer, beginInfo); }
    VkResult EndCommandBuffer() { return EndCommandBuffer(*m_cmdBuffer); }
    void Draw(uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
        { m_cmdBuffer->Draw(firstVertex, vertexCount, firstInstance, instanceCount); }
    void DrawIndexed(uint32_t firstVertex, uint32_t vertexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
        { m_cmdBuffer->DrawIndexed(firstVertex, vertexCount, vertexOffset,firstInstance, instanceCount); }
    void QueueCommandBuffer() { m_cmdBuffer->QueueCommandBuffer(); }
    void RotateTriangleVSUniform(glm::mat4 Projection, glm::mat4 View, glm::mat4 Model,
                                 VkConstantBufferObj *constantBuffer)
        {RotateTriangleVSUniform(Projection, View, Model, constantBuffer, m_cmdBuffer); }
    void BindVertexBuffer(VkConstantBufferObj *vertexBuffer, VkDeviceSize offset, uint32_t binding)
        { m_cmdBuffer->BindVertexBuffer(vertexBuffer, offset, binding); }
    void BindIndexBuffer(VkIndexBufferObj *indexBuffer, VkDeviceSize offset)
        { m_cmdBuffer->BindIndexBuffer(indexBuffer, offset); }



protected:
    VkImage m_texture;
    VkImageView m_textureView;
    VkDescriptorInfo m_descriptorInfo;
    VkDeviceMemory m_textureMem;

    VkSampler m_sampler;


    virtual void SetUp() {

        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "render_tests";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION;

        memset(&m_descriptorInfo, 0, sizeof(m_descriptorInfo));

        InitFramework();
    }

    virtual void TearDown() {
        // Clean up resources before we reset
        ShutdownFramework();
    }
};

VkResult VkRenderTest::BeginCommandBuffer(VkCommandBufferObj &cmdBuffer)
{
    VkResult result;

    result = cmdBuffer.BeginCommandBuffer();

    /*
     * For render test all drawing happens in a single render pass
     * on a single command buffer.
     */
    if (VK_SUCCESS == result && renderPass()) {
        cmdBuffer.BeginRenderPass(renderPassBeginInfo());
    }

    return result;
}

VkResult VkRenderTest::BeginCommandBuffer(VkCommandBufferObj &cmdBuffer, VkCmdBufferBeginInfo *beginInfo)
{
    VkResult result;

    result = cmdBuffer.BeginCommandBuffer(beginInfo);

    /*
     * For render test all drawing happens in a single render pass
     * on a single command buffer.
     */
    if (VK_SUCCESS == result && renderPass()) {
        cmdBuffer.BeginRenderPass(renderPassBeginInfo());
    }

    return result;
}

VkResult VkRenderTest::EndCommandBuffer(VkCommandBufferObj &cmdBuffer)
{
    VkResult result;

    if (renderPass()) {
        cmdBuffer.EndRenderPass();
    }

    result = cmdBuffer.EndCommandBuffer();

    return result;
}


void VkRenderTest::GenericDrawPreparation(VkCommandBufferObj *cmdBuffer, VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet)
{
    if (!m_clear_via_load_op) {
        if (m_depthStencil->Initialized()) {
            cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, m_depthStencil);
        } else {
            cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
        }
    }

    cmdBuffer->PrepareAttachments();
    cmdBuffer->BindDynamicLineWidthState(m_stateLineWidth);
    cmdBuffer->BindDynamicDepthBiasState(m_stateDepthBias);
    cmdBuffer->BindDynamicViewportState(m_stateViewport);
    cmdBuffer->BindDynamicBlendState(m_stateBlend);
    cmdBuffer->BindDynamicDepthBoundsState(m_stateDepthBounds);
    cmdBuffer->BindDynamicStencilState(m_stateStencil);
    descriptorSet.CreateVKDescriptorSet(cmdBuffer);
    VkResult err = pipelineobj.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());
    ASSERT_VK_SUCCESS(err);
    cmdBuffer->BindPipeline(pipelineobj);
    cmdBuffer->BindDescriptorSet(descriptorSet);
}

void VkRenderTest::RotateTriangleVSUniform(glm::mat4 Projection, glm::mat4 View, glm::mat4 Model,
                                            VkConstantBufferObj *constantBuffer, VkCommandBufferObj *cmdBuffer)
{
    int i;
    glm::mat4 MVP;
    int matrixSize = sizeof(MVP);
    VkResult err;

    /* Only do 3 positions to avoid back face cull */
    for (i = 0; i < 3; i++) {
        void *pData = constantBuffer->memory().map();

        Model = glm::rotate(Model, glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
        MVP = Projection * View * Model;
        memcpy(pData, (const void*) &MVP[0][0], matrixSize);

        constantBuffer->memory().unmap();

        // submit the command buffer to the universal queue
        cmdBuffer->QueueCommandBuffer();

        err = vkQueueWaitIdle( m_device->m_queue );
        ASSERT_VK_SUCCESS( err );

        // Wait for work to finish before cleaning up.
        vkDeviceWaitIdle(m_device->device());

        assert(m_renderTargets.size() == 1);
        RecordImage(m_renderTargets[0]);
    }
}

void dumpMatrix(const char *note, glm::mat4 MVP)
{
    int i;

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

struct vktriangle_vs_uniform {
    // Must start with MVP
    float   mvp[4][4];
    float   position[3][4];
    float   color[3][4];
};

void VkRenderTest::VKTriangleTest(const char *vertShaderText, const char *fragShaderText, const bool rotate)
{
#ifdef DEBUG_CALLBACK
    vkDbgRegisterMsgCallback(inst, myDbgFunc, NULL);
#endif
    // Create identity matrix
    int i;
    struct vktriangle_vs_uniform data;

    glm::mat4 Projection      = glm::mat4(1.0f);
    glm::mat4 View      = glm::mat4(1.0f);
    glm::mat4 Model      = glm::mat4(1.0f);
    glm::mat4 MVP = Projection * View * Model;
    const int matrixSize = sizeof(MVP);
    const int bufSize = sizeof(vktriangle_vs_uniform) / sizeof(float);
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

    VkConstantBufferObj constantBuffer(m_device, bufSize*2, sizeof(float), (const void*) &data);

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, constantBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkCmdBufferBeginInfo cbBeginInfo;
    memset(&cbBeginInfo, 0, sizeof(VkCmdBufferBeginInfo));
    cbBeginInfo.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cbBeginInfo.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT;
    ASSERT_VK_SUCCESS(BeginCommandBuffer(&cbBeginInfo));

    GenericDrawPreparation(pipelineobj, descriptorSet);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    ASSERT_VK_SUCCESS(EndCommandBuffer());

    QueueCommandBuffer();

    RecordImages(m_renderTargets);

    if (rotate)
        RotateTriangleVSUniform(Projection, View, Model, &constantBuffer);

#ifdef PRINT_OBJECTS
    OBJ_TRACK_GET_OBJECTS_COUNT pObjTrackGetObjectsCount = (OBJ_TRACK_GET_OBJECTS_COUNT)vkGetProcAddr(gpu(), (char*)"objTrackGetObjectsCount");
    uint64_t numObjects = pObjTrackGetObjectsCount(m_device);
    //OBJ_TRACK_GET_OBJECTS pGetObjsFunc = vkGetProcAddr(gpu(), (char*)"objTrackGetObjects");
    printf("DEBUG : Number of Objects : %lu\n", numObjects);
    OBJ_TRACK_GET_OBJECTS pObjTrackGetObjs = (OBJ_TRACK_GET_OBJECTS)vkGetProcAddr(gpu(), (char*)"objTrackGetObjects");
    OBJTRACK_NODE* pObjNodeArray = (OBJTRACK_NODE*)malloc(sizeof(OBJTRACK_NODE)*numObjects);
    pObjTrackGetObjs(m_device, numObjects, pObjNodeArray);
    for (i=0; i < numObjects; i++) {
        printf("Object %i of type %s has objID (%p) and %lu uses\n", i, string_VkObjectType(pObjNodeArray[i].objType), pObjNodeArray[i].pObj, pObjNodeArray[i].numUses);
    }
    free(pObjNodeArray);
#endif

}

TEST_F(VkRenderTest, VKTriangle_FragColor)
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
        "layout (location = 0) out vec4 uFragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   uFragColor = inColor;\n"
        "}\n";

    TEST_DESCRIPTION("VK-style shaders where fragment shader outputs to GLSL built-in gl_FragColor");
    VKTriangleTest(vertShaderText, fragShaderText, true);
}

TEST_F(VkRenderTest, VKTriangle_OutputLocation)
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

    TEST_DESCRIPTION("VK-style shaders where fragment shader outputs to output location 0, which should be the same as gl_FragColor");

    VKTriangleTest(vertShaderText, fragShaderText, true);
}

TEST_F(VkRenderTest, SPV_VKTriangle)
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

    TEST_DESCRIPTION("VK-style shaders, but force test framework to compile shader to SPV and pass SPV to driver.");

    ScopedUseGlsl useGlsl(false);
    VKTriangleTest(vertShaderText, fragShaderText, true);
}

TEST_F(VkRenderTest, SPV_GreenTriangle)
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
       "#version 140\n"
       "#extension GL_ARB_separate_shader_objects : enable\n"
       "#extension GL_ARB_shading_language_420pack : enable\n"
       "layout (location = 0) out vec4 outColor;\n"
       "void main() {\n"
       "   outColor = vec4(0,1,0,1);\n"
       "}\n";

    TEST_DESCRIPTION("Same shader as GreenTriangle, but compiles shader to SPV and gives SPV to driver.");

    ScopedUseGlsl useGlsl(false);

    VKTriangleTest(vertShaderText, fragShaderText, false);
}

TEST_F(VkRenderTest, YellowTriangle)
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
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "  outColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
            "}\n";

    VKTriangleTest(vertShaderText, fragShaderText, false);
}

TEST_F(VkRenderTest, QuadWithVertexFetch)
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = color;\n"
            "}\n";



    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BIND_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BIND_ID,                      // binding ID
         sizeof(g_vbData[0]),              // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
         VK_VERTEX_INPUT_STEP_RATE_VERTEX // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = MESH_BIND_ID;               // Binding ID
    vi_attribs[0].location = 0;                         // location, position
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[0].offsetInBytes = 0;                    // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = MESH_BIND_ID;               // Binding ID
    vi_attribs[1].location = 1;                         // location, color
    vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[1].offsetInBytes = 1*sizeof(float)*4;     // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);

    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    ASSERT_VK_SUCCESS(EndCommandBuffer());

    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriangleMRT)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 pos;\n"
            "void main() {\n"
            "   gl_Position = pos;\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 uFragData0;\n"
            "layout (location = 1) out vec4 uFragData1;\n"
            "void main() {\n"
            "   uFragData0 = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "   uFragData1 = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "}\n";
    const float vb_data[][2] = {
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        { -1.0f,  1.0f }
    };

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device, sizeof(vb_data) / sizeof(vb_data[0]), sizeof(vb_data[0]), vb_data);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

#define MESH_BUF_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BUF_ID,                            // Binding ID
        sizeof(vb_data[0]),                     // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attrib;
    vi_attrib.binding = MESH_BUF_ID;            // index into vertexBindingDescriptions
    vi_attrib.location = 0;
    vi_attrib.format = VK_FORMAT_R32G32_SFLOAT;   // format of source data
    vi_attrib.offsetInBytes = 0;                // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(&vi_attrib, 1);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    VkDescriptorSetObj descriptorSet(m_device);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget(2));

    VkPipelineColorBlendAttachmentState att = {};
    att.blendEnable = VK_FALSE;
    att.channelWriteMask = 0xf;
    pipelineobj.AddColorAttachment(1, &att);

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    ASSERT_VK_SUCCESS(EndCommandBuffer());
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, QuadWithIndexedVertexFetch)
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = color;\n"
            "}\n";

    const Vertex g_vbData[] =
    {
        // first tri
        { XYZ1( -1, -1, -1 ), XYZ1( 0.f, 0.f, 0.f ) },  // LL: black
        { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },   // LR: red
        { XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },  // UL: green

        // second tri
        { XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },  // UL: green
        { XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },   // LR: red
        { XYZ1( 1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },   // UR: yellow
    };

    const uint16_t g_idxData[6] = {
        0, 1, 2,
        3, 4, 5,
    };

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkIndexBufferObj indexBuffer(m_device);
    indexBuffer.CreateAndInitBuffer(sizeof(g_idxData)/sizeof(g_idxData[0]), VK_INDEX_TYPE_UINT16, g_idxData);
    indexBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);


#define MESH_BIND_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BIND_ID,                           // binding ID
        sizeof(g_vbData[0]),                    // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = MESH_BIND_ID;               // binding ID from BINDING_DESCRIPTION array to use for this attribute
    vi_attribs[0].location = 0;                         // layout location of vertex attribute
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[0].offsetInBytes = 0;                    // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = MESH_BIND_ID;               // binding ID from BINDING_DESCRIPTION array to use for this attribute
    vi_attribs[1].location = 1;                         // layout location of vertex attribute
    vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[1].offsetInBytes = 16;                   // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    BindVertexBuffer(&meshBuffer, 0, MESH_BIND_ID);
    BindIndexBuffer(&indexBuffer, 0);

    // render two triangles
    DrawIndexed(0, 6, 0, 0, 1);

    // finalize recording of the command buffer
    ASSERT_VK_SUCCESS(EndCommandBuffer());
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, GreyandRedCirclesonBlue)
{
    // This tests gl_FragCoord

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "layout (location = 1) out vec4 outColor2;\n"
            "void main() {\n"
            "   gl_Position = pos;\n"
            "   outColor = vec4(0.9, 0.9, 0.9, 1.0);\n"
            "   outColor2 = vec4(0.2, 0.2, 0.4, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            //"#extension GL_ARB_fragment_coord_conventions : enable\n"
            //"layout (pixel_center_integer) in vec4 gl_FragCoord;\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 1) in vec4 color2;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "    vec2 pos = mod(gl_FragCoord.xy, vec2(50.0)) - vec2(25.0);\n"
            "    float dist_squared = dot(pos, pos);\n"
            "    outColor = (dist_squared < 400.0)\n"
            "        ? ((gl_FragCoord.y < 100.0) ? vec4(1.0, 0.0, 0.0, 0.0) : color)\n"
            "        : color2;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BIND_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BIND_ID,                           // binding ID
        sizeof(g_vbData[0]),                    // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[1];
    vi_attribs[0].binding = MESH_BIND_ID;               // binding ID
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[0].offsetInBytes = 0;                    // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,1);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render triangle
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, RedCirclesonBlue)
{
    // This tests that we correctly handle unread fragment inputs

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "layout (location = 1) out vec4 outColor2;\n"
            "void main() {\n"
            "   gl_Position = pos;\n"
            "   outColor = vec4(0.9, 0.9, 0.9, 1.0);\n"
            "   outColor2 = vec4(0.2, 0.2, 0.4, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            //"#extension GL_ARB_fragment_coord_conventions : enable\n"
            //"layout (pixel_center_integer) in vec4 gl_FragCoord;\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 1) in vec4 color2;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "    vec2 pos = mod(gl_FragCoord.xy, vec2(50.0)) - vec2(25.0);\n"
            "    float dist_squared = dot(pos, pos);\n"
            "    outColor = (dist_squared < 400.0)\n"
            "        ? vec4(1.0, 0.0, 0.0, 1.0)\n"
            "        : color2;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BIND_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BIND_ID,                           // binding ID
        sizeof(g_vbData[0]),                    // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[1];
    vi_attribs[0].binding = MESH_BIND_ID;               // binding ID
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[0].offsetInBytes = 0;                    // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,1);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, GreyCirclesonBlueFade)
{
    // This tests reading gl_ClipDistance from FS

    static const char *vertShaderText =
            //"#version 140\n"
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "out gl_PerVertex {\n"
            "    vec4 gl_Position;\n"
            "    float gl_ClipDistance[1];\n"
            "};\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "layout (location = 1) out vec4 outColor2;\n"
            "void main() {\n"
            "   gl_Position = pos;\n"
            "   outColor = vec4(0.9, 0.9, 0.9, 1.0);\n"
            "   outColor2 = vec4(0.2, 0.2, 0.4, 1.0);\n"
            "   float dists[3];\n"
            "      dists[0] = 0.0;\n"
            "      dists[1] = 1.0;\n"
            "      dists[2] = 1.0;\n"
            "   gl_ClipDistance[0] = dists[gl_VertexID % 3];\n"
            "}\n";


    static const char *fragShaderText =
            "#version 140\n"
            //"#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            //"#extension GL_ARB_fragment_coord_conventions : enable\n"
            //"layout (pixel_center_integer) in vec4 gl_FragCoord;\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 1) in vec4 color2;\n"
            "layout (location = 0) out vec4 uFragColor;\n"
            "void main() {\n"
            "    vec2 pos = mod(gl_FragCoord.xy, vec2(50.0)) - vec2(25.0);\n"
            "    float dist_squared = dot(pos, pos);\n"
            "    uFragColor = (dist_squared < 400.0)\n"
            "        ? color * gl_ClipDistance[0]\n"
            "        : color2;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BIND_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BIND_ID,                           // binding ID
        sizeof(g_vbData[0]),                    // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[1];
    vi_attribs[0].binding = MESH_BIND_ID;               // binding ID
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[0].offsetInBytes = 0;                    // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,1);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, GreyCirclesonBlueDiscard)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "layout (location = 1) out vec4 outColor2;\n"
            "void main() {\n"
            "   gl_Position = pos;\n"
            "   outColor = vec4(0.9, 0.9, 0.9, 1.0);\n"
            "   outColor2 = vec4(0.2, 0.2, 0.4, 1.0);\n"
            "}\n";


    static const char *fragShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            //"#extension GL_ARB_fragment_coord_conventions : enable\n"
            //"layout (pixel_center_integer) in vec4 gl_FragCoord;\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 1) in vec4 color2;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "    vec2 pos = mod(gl_FragCoord.xy, vec2(50.0)) - vec2(25.0);\n"
            "    float dist_squared = dot(pos, pos);\n"
            "    if (dist_squared < 100.0)\n"
            "        discard;\n"
            "    outColor = (dist_squared < 400.0)\n"
            "        ? color\n"
            "        : color2;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BIND_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BIND_ID,                           // binding ID
        sizeof(g_vbData[0]),                    // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[1];
    vi_attribs[0].binding = MESH_BIND_ID;               // binding ID
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[0].offsetInBytes = 0;                    // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs,1);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}


TEST_F(VkRenderTest, TriangleVSUniform)
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
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    // Create identity matrix
    glm::mat4 Projection    = glm::mat4(1.0f);
    glm::mat4 View          = glm::mat4(1.0f);
    glm::mat4 Model         = glm::mat4(1.0f);
    glm::mat4 MVP = Projection * View * Model;
    const int matrixSize = sizeof(MVP) / sizeof(MVP[0]);

    VkConstantBufferObj MVPBuffer(m_device, matrixSize, sizeof(MVP[0]), (const void*) &MVP[0][0]);
    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    // Create descriptor set and attach the constant buffer to it
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MVPBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkCmdBufferBeginInfo cbBeginInfo;
    memset(&cbBeginInfo, 0, sizeof(VkCmdBufferBeginInfo));
    cbBeginInfo.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cbBeginInfo.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT;
    ASSERT_VK_SUCCESS(BeginCommandBuffer(&cbBeginInfo));

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);

    RotateTriangleVSUniform(Projection, View, Model, &MVPBuffer);
}

TEST_F(VkRenderTest, MixTriangle)
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
           "layout (location = 0) out vec4 outColor;\n"
           "void main() {\n"
           "   outColor = bar * scale + foo * (1.0-scale);\n"
           "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, QuadVertFetchAndVertID)
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = color;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BUF_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BUF_ID,                            // Binding ID
        sizeof(g_vbData[0]),                    // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = MESH_BUF_ID;            // binding ID
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32_SFLOAT;   // format of source data
    vi_attribs[0].offsetInBytes = 0;                // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = MESH_BUF_ID;            // binding ID
    vi_attribs[1].location = 1;
    vi_attribs[1].format = VK_FORMAT_R32G32_SFLOAT;   // format of source data
    vi_attribs[1].offsetInBytes = 16;                // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs, 2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, QuadSparseVertFetch)
{
    // This tests that attributes work in the presence of gl_VertexID

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            //XYZ1( -1, -1, -1 )
            "layout (location = 1) in vec4 pos;\n"
            "layout (location = 4) in vec4 inColor;\n"
            //XYZ1( 0.f, 0.f, 0.f )
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = color;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    struct VDATA
    {
        float t1, t2, t3, t4;            // filler data
        float posX, posY, posZ, posW;    // Position data
        float r, g, b, a;                // Color
    };
    const struct VDATA vData[] =
    {
        { XYZ1(0, 0, 0), XYZ1( -1, -1, -1 ), XYZ1( 0.f, 0.f, 0.f ) },
        { XYZ1(0, 0, 0), XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
        { XYZ1(0, 0, 0), XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },
        { XYZ1(0, 0, 0), XYZ1( -1,  1, -1 ), XYZ1( 0.f, 1.f, 0.f ) },
        { XYZ1(0, 0, 0), XYZ1( 1, -1, -1 ), XYZ1( 1.f, 0.f, 0.f ) },
        { XYZ1(0, 0, 0), XYZ1( 1,  1, -1 ), XYZ1( 1.f, 1.f, 0.f ) },
    };

    VkConstantBufferObj meshBuffer(m_device,sizeof(vData)/sizeof(vData[0]),sizeof(vData[0]), vData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BUF_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BUF_ID,                            // Binding ID
        sizeof(vData[0]),                       // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = MESH_BUF_ID;                                        // binding ID
    vi_attribs[0].location = 4;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;                         // format of source data
    vi_attribs[0].offsetInBytes = sizeof(float) * 4 * 2;   // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = MESH_BUF_ID;                                        // binding ID
    vi_attribs[1].location = 1;
    vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;                         // format of source data
    vi_attribs[1].offsetInBytes = sizeof(float) * 4 * 1;   // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs, 2);
    pipelineobj.AddVertexInputBindings(&vi_binding, 1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, MESH_BUF_ID);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriVertFetchDeadAttr)
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = color;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vbData)/sizeof(g_vbData[0]),sizeof(g_vbData[0]), g_vbData);
    meshBuffer.BufferMemoryBarrier();

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);

#define MESH_BUF_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BUF_ID,                            // Binding ID
        sizeof(g_vbData[0]),                    // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = MESH_BUF_ID;            // binding ID
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;            // format of source data
    vi_attribs[0].offsetInBytes = 0;                // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = MESH_BUF_ID;            // binding ID
    vi_attribs[1].location = 1;
    vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;            // format of source data
    vi_attribs[1].offsetInBytes = 16;                // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs, 2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render two triangles
    Draw(0, 6, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, CubeWithVertexFetchAndMVP)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform bufferVals {\n"
            "    mat4 mvp;\n"
            "} myBufferVals;\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 1) in vec4 inColor;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   gl_Position = myBufferVals.mvp * pos;\n"
            "   gl_Position.y = -gl_Position.y;\n"
            "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = color;\n"
            "}\n";
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    glm::mat4 View       = glm::lookAt(
                           glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                           glm::vec3(0,0,0), // and looks at the origin
                           glm::vec3(0,-1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                           );

    glm::mat4 Model = glm::mat4(1.0f);

    glm::mat4 MVP = Projection * View * Model;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    m_depth_stencil_fmt = VK_FORMAT_D16_UNORM;
    m_depthStencil->Init(m_device, (int32_t)m_width, (int32_t)m_height, m_depth_stencil_fmt);

    VkConstantBufferObj meshBuffer(m_device,sizeof(g_vb_solid_face_colors_Data)/sizeof(g_vb_solid_face_colors_Data[0]),
            sizeof(g_vb_solid_face_colors_Data[0]), g_vb_solid_face_colors_Data);

    const int buf_size = sizeof(MVP) / sizeof(float);

    VkConstantBufferObj MVPBuffer(m_device, buf_size, sizeof(MVP[0]), (const void*) &MVP[0][0]);
    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkPipelineDepthStencilStateCreateInfo ds_state;
    ds_state.depthTestEnable = VK_TRUE;
    ds_state.depthWriteEnable = VK_TRUE;
    ds_state.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds_state.depthBoundsTestEnable = VK_FALSE;
    ds_state.stencilTestEnable = VK_FALSE;
    ds_state.back.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
    ds_state.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds_state.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds_state.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds_state.front = ds_state.back;
    pipelineobj.SetDepthStencil(&ds_state);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MVPBuffer);

#define MESH_BUF_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BUF_ID,                            // Binding ID
        sizeof(g_vbData[0]),                     // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX       // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = MESH_BUF_ID;            // binding ID
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;            // format of source data
    vi_attribs[0].offsetInBytes = 0;                // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = MESH_BUF_ID;            // binding ID
    vi_attribs[1].location = 1;
    vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;            // format of source data
    vi_attribs[1].offsetInBytes = 16;                // Offset of first element in bytes from base of vertex

    pipelineobj.AddVertexInputAttribs(vi_attribs, 2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget(m_depthStencil->BindInfo()));

    ASSERT_VK_SUCCESS(BeginCommandBuffer());
    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render triangles
    Draw(0, 36, 0, 1);


    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, VSTexture)
{
    // The expected result from this test is a green and red triangle;
    // one red vertex on the left, two green vertices on the right.
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 texColor;\n"
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
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 texColor;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = texColor;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);
    VkSamplerObj sampler(m_device);
    VkTextureObj texture(m_device);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler, &texture);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}



TEST_F(VkRenderTest, TexturedTriangle)
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

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);
    VkSamplerObj sampler(m_device);
    VkTextureObj texture(m_device);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler, &texture);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}
TEST_F(VkRenderTest, TexturedTriangleClip)
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

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);
    VkSamplerObj sampler(m_device);
    VkTextureObj texture(m_device);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler, &texture);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, FSTriangle)
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

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);
    VkSamplerObj sampler(m_device);
    VkTextureObj texture(m_device);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler, &texture);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}
TEST_F(VkRenderTest, SamplerBindingsTriangle)
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = textureLod(surface2, samplePos.xy, 0.0);\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkSamplerObj sampler1(m_device);
    VkSamplerObj sampler2(m_device);
    VkSamplerObj sampler3(m_device);

    uint32_t tex_colors[2] = { 0xffff0000, 0xffff0000 };
    VkTextureObj texture1(m_device, tex_colors); // Red
    tex_colors[0] = 0xff00ff00; tex_colors[1] = 0xff00ff00;
    VkTextureObj texture2(m_device, tex_colors); // Green
    tex_colors[0] = 0xff0000ff; tex_colors[1] = 0xff0000ff;
    VkTextureObj texture3(m_device, tex_colors); // Blue

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler1, &texture1);
    descriptorSet.AppendSamplerTexture(&sampler2, &texture2);
    for (int i = 0; i < 10; i++)
        descriptorSet.AppendDummy();
    descriptorSet.AppendSamplerTexture(&sampler3, &texture3);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriangleVSUniformBlock)
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

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

    VkConstantBufferObj colorBuffer(m_device, valCount, sizeof(bufferVals[0]), (const void*) bufferVals);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, colorBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriangleFSUniformBlockBinding)
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
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = myBlueVal.color;\n"
            "   outColor += myRedVal.color;\n"
            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

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

    VkConstantBufferObj redBuffer(m_device, redCount, sizeof(redVals[0]), (const void*) redVals);

    VkConstantBufferObj greenBuffer(m_device, greenCount, sizeof(greenVals[0]), (const void*) greenVals);

    VkConstantBufferObj blueBuffer(m_device, blueCount, sizeof(blueVals[0]), (const void*) blueVals);

    VkConstantBufferObj whiteBuffer(m_device, whiteCount, sizeof(whiteVals[0]), (const void*) whiteVals);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, redBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, greenBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, blueBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, whiteBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriangleFSAnonymousUniformBlockBinding)
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
            "layout (std140, binding = 0) uniform redVal   { vec4 red; };"
            "layout (std140, binding = 1) uniform greenVal { vec4 green; };"
            "layout (std140, binding = 2) uniform blueVal  { vec4 blue; };"
            "layout (std140, binding = 3) uniform whiteVal { vec4 white; };"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = blue;\n"
            "   outColor += red;\n"
            "}\n";
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

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

    VkConstantBufferObj redBuffer(m_device, redCount, sizeof(redVals[0]), (const void*) redVals);

    VkConstantBufferObj greenBuffer(m_device, greenCount, sizeof(greenVals[0]), (const void*) greenVals);

    VkConstantBufferObj blueBuffer(m_device, blueCount, sizeof(blueVals[0]), (const void*) blueVals);

    VkConstantBufferObj whiteBuffer(m_device, whiteCount, sizeof(whiteVals[0]), (const void*) whiteVals);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, redBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, greenBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, blueBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, whiteBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriangleFSAnonymousUniformBlockBindingWithStruct)
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
            "\n"
            "    struct PS_INPUT {\n"
            "        vec2 member0;\n"
            "        vec4 member1;\n"
            "        vec4 member2;\n"
            "        vec4 member3;\n"
            "        vec4 member4;\n"
            "        vec4 member5;\n"
            "        vec4 member6;\n"
            "        vec4 member7;\n"
            "        vec4 member8;\n"
            "        vec4 member9;\n"
            "    };\n"
            "\n"
            "layout (std140, binding = 0) uniform redVal   { vec4 red; };"
            "layout (std140, binding = 1) uniform greenVal { vec4 green; };"
            "layout (std140, binding = 2) uniform blueVal  { vec4 blue; };"
            "layout (std140, binding = 3) uniform whiteVal { vec4 white; };"
            "layout (location = 0) out vec4 outColor;\n"
            "PS_INPUT MainFs()\n"
            "{\n"
            "    PS_INPUT o;\n"
            "    o.member9 = red;\n"
            "    return o;\n"
            "}\n"
            "\n"
            "void main()\n"
            "{\n"
            "    PS_INPUT o;\n"
            "    o = MainFs();\n"
            "    outColor = blue;"
            "    outColor += o.member9;\n"
            "}\n";;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

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

    VkConstantBufferObj redBuffer(m_device, redCount, sizeof(redVals[0]), (const void*) redVals);

    VkConstantBufferObj greenBuffer(m_device, greenCount, sizeof(greenVals[0]), (const void*) greenVals);

    VkConstantBufferObj blueBuffer(m_device, blueCount, sizeof(blueVals[0]), (const void*) blueVals);

    VkConstantBufferObj whiteBuffer(m_device, whiteCount, sizeof(whiteVals[0]), (const void*) whiteVals);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, redBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, greenBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, blueBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, whiteBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, CubeWithVertexFetchAndMVPAndTexture)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding=0) uniform bufferVals {\n"
            "    mat4 mvp;\n"
            "} myBufferVals;\n"
            "layout (location=0) in vec4 pos;\n"
            "layout (location=1) in vec2 input_uv;\n"
            "layout (location=0) out vec2 UV;\n"
            "void main() {\n"
            "   UV = input_uv;\n"
            "   gl_Position = myBufferVals.mvp * pos;\n"
            "   gl_Position.y = -gl_Position.y;\n"
            "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (binding=1) uniform sampler2D surface;\n"
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
    int num_verts = sizeof(g_vb_texture_Data) / sizeof(g_vb_texture_Data[0]);


    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    m_depth_stencil_fmt = VK_FORMAT_D16_UNORM;
    m_depthStencil->Init(m_device, (int32_t)m_width, (int32_t)m_height, m_depth_stencil_fmt);

    VkConstantBufferObj meshBuffer(m_device, num_verts,
            sizeof(g_vb_texture_Data[0]), g_vb_texture_Data);
    meshBuffer.BufferMemoryBarrier();

    const int buf_size = sizeof(MVP) / sizeof(float);

    VkConstantBufferObj mvpBuffer(m_device, buf_size, sizeof(MVP[0]), (const void*) &MVP[0][0]);
    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);
    VkSamplerObj sampler(m_device);
    VkTextureObj texture(m_device);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mvpBuffer);
    descriptorSet.AppendSamplerTexture(&sampler, &texture);

#define MESH_BIND_ID 0
    VkVertexInputBindingDescription vi_binding = {
        MESH_BIND_ID,                      // binding ID
        sizeof(g_vb_texture_Data[0]),               // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
        VK_VERTEX_INPUT_STEP_RATE_VERTEX  // stepRate;       // Rate at which binding is incremented
    };

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = MESH_BIND_ID;                 // Binding ID
    vi_attribs[0].location = 0;                           // location
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // format of source data
    vi_attribs[0].offsetInBytes = 0;                      // Offset of first element in bytes from base of vertex
    vi_attribs[1].binding = MESH_BIND_ID;                 // Binding ID
    vi_attribs[1].location = 1;                           // location
    vi_attribs[1].format = VK_FORMAT_R32G32_SFLOAT;       // format of source data
    vi_attribs[1].offsetInBytes = 16;                     // Offset of uv components

    pipelineobj.AddVertexInputAttribs(vi_attribs,2);
    pipelineobj.AddVertexInputBindings(&vi_binding,1);

    VkPipelineDepthStencilStateCreateInfo ds_state;
    ds_state.depthTestEnable = VK_TRUE;
    ds_state.depthWriteEnable = VK_TRUE;
    ds_state.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds_state.depthBoundsTestEnable = VK_FALSE;
    ds_state.stencilTestEnable = VK_FALSE;
    ds_state.back.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
    ds_state.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds_state.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds_state.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds_state.front = ds_state.back;
    pipelineobj.SetDepthStencil(&ds_state);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget(m_depthStencil->BindInfo()));

    VkCmdBufferBeginInfo cbBeginInfo;
    memset(&cbBeginInfo, 0, sizeof(VkCmdBufferBeginInfo));
    cbBeginInfo.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cbBeginInfo.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT;
    ASSERT_VK_SUCCESS(BeginCommandBuffer(&cbBeginInfo));

    GenericDrawPreparation(pipelineobj, descriptorSet);

    BindVertexBuffer(&meshBuffer, 0, 0);
#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, num_verts, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
    RotateTriangleVSUniform(Projection, View, Model, &mvpBuffer);
}

TEST_F(VkRenderTest, TriangleMixedSamplerUniformBlockBinding)
{
    // This test mixes binding slots of textures and buffers, ensuring
    // that sparse and overlapping assignments work.
    // The expected result from this test is a purple triangle, although
    // you can modify it to move the desired result around.

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
            "layout (binding = 0) uniform sampler2D surface0;\n"
            "layout (binding = 3) uniform sampler2D surface1;\n"
            "layout (binding = 1) uniform sampler2D surface2;\n"
            "layout (binding = 2) uniform sampler2D surface3;\n"


            "layout (std140, binding = 4) uniform redVal   { vec4 red; };"
            "layout (std140, binding = 6) uniform greenVal { vec4 green; };"
            "layout (std140, binding = 5) uniform blueVal  { vec4 blue; };"
            "layout (std140, binding = 7) uniform whiteVal { vec4 white; };"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = red * vec4(0.00001);\n"
            "   outColor += white * vec4(0.00001);\n"
            "   outColor += textureLod(surface2, vec2(0.5), 0.0)* vec4(0.00001);\n"
            "   outColor += textureLod(surface1, vec2(0.0), 0.0);//* vec4(0.00001);\n"
            "}\n";
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    const float redVals[4]   = { 1.0, 0.0, 0.0, 1.0 };
    const float greenVals[4] = { 0.0, 1.0, 0.0, 1.0 };
    const float blueVals[4]  = { 0.0, 0.0, 1.0, 1.0 };
    const float whiteVals[4] = { 1.0, 1.0, 1.0, 1.0 };

    const int redCount   = sizeof(redVals)   / sizeof(float);
    const int greenCount = sizeof(greenVals) / sizeof(float);
    const int blueCount  = sizeof(blueVals)  / sizeof(float);
    const int whiteCount = sizeof(whiteVals) / sizeof(float);

    VkConstantBufferObj redBuffer(m_device, redCount, sizeof(redVals[0]), (const void*) redVals);
    VkConstantBufferObj greenBuffer(m_device, greenCount, sizeof(greenVals[0]), (const void*) greenVals);
    VkConstantBufferObj blueBuffer(m_device, blueCount, sizeof(blueVals[0]), (const void*) blueVals);
    VkConstantBufferObj whiteBuffer(m_device, whiteCount, sizeof(whiteVals[0]), (const void*) whiteVals);

    uint32_t tex_colors[2] = { 0xff800000, 0xff800000 };
    VkSamplerObj sampler0(m_device);
    VkTextureObj texture0(m_device, tex_colors); // Light Red
    tex_colors[0] = 0xff000080; tex_colors[1] = 0xff000080;
    VkSamplerObj sampler2(m_device);
    VkTextureObj texture2(m_device, tex_colors); // Light Blue
    tex_colors[0] = 0xff008000; tex_colors[1] = 0xff008000;
    VkSamplerObj sampler4(m_device);
    VkTextureObj texture4(m_device, tex_colors); // Light Green

    // NOTE:  Bindings 1,3,5,7,8,9,11,12,14,16 work for this sampler, but 6 does not!!!
    // TODO:  Get back here ASAP and understand why.
    tex_colors[0] = 0xffff00ff; tex_colors[1] = 0xffff00ff;
    VkSamplerObj sampler7(m_device);
    VkTextureObj texture7(m_device, tex_colors); // Red and Blue

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler0, &texture0);
    descriptorSet.AppendSamplerTexture(&sampler2, &texture2);
    descriptorSet.AppendSamplerTexture(&sampler4, &texture4);
    descriptorSet.AppendSamplerTexture(&sampler7, &texture7);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, redBuffer);
    // swap blue and green
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, blueBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, greenBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, whiteBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriangleMatchingSamplerUniformBlockBinding)
{
    // This test matches binding slots of textures and buffers, requiring
    // the driver to give them distinct number spaces.
    // The expected result from this test is a red triangle, although
    // you can modify it to move the desired result around.

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
            "layout (binding = 0) uniform sampler2D surface0;\n"
            "layout (binding = 1) uniform sampler2D surface1;\n"
            "layout (binding = 2) uniform sampler2D surface2;\n"
            "layout (binding = 3) uniform sampler2D surface3;\n"
            "layout (std140, binding = 4) uniform redVal   { vec4 red; };"
            "layout (std140, binding = 5) uniform greenVal { vec4 green; };"
            "layout (std140, binding = 6) uniform blueVal  { vec4 blue; };"
            "layout (std140, binding = 7) uniform whiteVal { vec4 white; };"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = red;// * vec4(0.00001);\n"
            "   outColor += white * vec4(0.00001);\n"
            "   outColor += textureLod(surface1, vec2(0.5), 0.0)* vec4(0.00001);\n"
            "   outColor += textureLod(surface3, vec2(0.0), 0.0)* vec4(0.00001);\n"
            "}\n";
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    const float redVals[4]   = { 1.0, 0.0, 0.0, 1.0 };
    const float greenVals[4] = { 0.0, 1.0, 0.0, 1.0 };
    const float blueVals[4]  = { 0.0, 0.0, 1.0, 1.0 };
    const float whiteVals[4] = { 1.0, 1.0, 1.0, 1.0 };

    const int redCount   = sizeof(redVals)   / sizeof(float);
    const int greenCount = sizeof(greenVals) / sizeof(float);
    const int blueCount  = sizeof(blueVals)  / sizeof(float);
    const int whiteCount = sizeof(whiteVals) / sizeof(float);

    VkConstantBufferObj redBuffer(m_device, redCount, sizeof(redVals[0]), (const void*) redVals);
    VkConstantBufferObj greenBuffer(m_device, greenCount, sizeof(greenVals[0]), (const void*) greenVals);
    VkConstantBufferObj blueBuffer(m_device, blueCount, sizeof(blueVals[0]), (const void*) blueVals);
    VkConstantBufferObj whiteBuffer(m_device, whiteCount, sizeof(whiteVals[0]), (const void*) whiteVals);

    uint32_t tex_colors[2] = { 0xff800000, 0xff800000 };
    VkSamplerObj sampler0(m_device);
    VkTextureObj texture0(m_device, tex_colors); // Light Red
    tex_colors[0] = 0xff000080; tex_colors[1] = 0xff000080;
    VkSamplerObj sampler2(m_device);
    VkTextureObj texture2(m_device, tex_colors); // Light Blue
    tex_colors[0] = 0xff008000; tex_colors[1] = 0xff008000;
    VkSamplerObj sampler4(m_device);
    VkTextureObj texture4(m_device, tex_colors); // Light Green
    tex_colors[0] = 0xffff00ff; tex_colors[1] = 0xffff00ff;
    VkSamplerObj sampler7(m_device);
    VkTextureObj texture7(m_device, tex_colors); // Red and Blue

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler0, &texture0);
    descriptorSet.AppendSamplerTexture(&sampler2, &texture2);
    descriptorSet.AppendSamplerTexture(&sampler4, &texture4);
    descriptorSet.AppendSamplerTexture(&sampler7, &texture7);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, redBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, greenBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, blueBuffer);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, whiteBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TriangleUniformBufferLayout)
{
    // This test populates a buffer with a variety of different data
    // types, then reads them out with a shader.
    // The expected result from this test is a green triangle

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform mixedBuffer {\n"
            "    vec4 fRed;\n"
            "    vec4 fGreen;\n"
            "    layout(row_major) mat4 worldToProj;\n"
            "    layout(row_major) mat4 projToWorld;\n"
            "    layout(row_major) mat4 worldToView;\n"
            "    layout(row_major) mat4 viewToProj;\n"
            "    layout(row_major) mat4 worldToShadow[4];\n"
            "    float fZero;\n"
            "    float fOne;\n"
            "    float fTwo;\n"
            "    float fThree;\n"
            "    vec3 fZeroZeroZero;\n"
            "    float fFour;\n"
            "    vec3 fZeroZeroOne;\n"
            "    float fFive;\n"
            "    vec3 fZeroOneZero;\n"
            "    float fSix;\n"
            "    float fSeven;\n"
            "    float fEight;\n"
            "    float fNine;\n"
            "    vec2 fZeroZero;\n"
            "    vec2 fZeroOne;\n"
            "    vec4 fBlue;\n"
            "    vec2 fOneZero;\n"
            "    vec2 fOneOne;\n"
            "    vec3 fZeroOneOne;\n"
            "    float fTen;\n"
            "    float fEleven;\n"
            "    float fTwelve;\n"
            "    vec3 fOneZeroZero;\n"
            "    vec4 uvOffsets[4];\n"
            "};\n"
            "layout (location = 0) out vec4 color;"
            "void main() {\n"

            "   vec4 right = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "   vec4 wrong = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "   \n"

            // do some exact comparisons, even though we should
            // really have an epsilon involved.
            "   vec4 outColor = right;\n"
            "   if (fRed != vec4(1.0, 0.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fGreen != vec4(0.0, 1.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fBlue != vec4(0.0, 0.0, 1.0, 1.0))\n"
            "       outColor = wrong;\n"

            "   color = outColor;\n"

            // generic position stuff
            "   vec2 vertices;\n"
            "   int vertexSelector = gl_VertexID;\n"
            "   if (vertexSelector == 0)\n"
            "      vertices = vec2(-0.5, -0.5);\n"
            "   else if (vertexSelector == 1)\n"
            "      vertices = vec2( 0.5, -0.5);\n"
            "   else if (vertexSelector == 2)\n"
            "      vertices = vec2( 0.5, 0.5);\n"
            "   else\n"
            "      vertices = vec2( 0.0,  0.0);\n"
            "   gl_Position = vec4(vertices, 0.0, 1.0);\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform mixedBuffer {\n"
            "    vec4 fRed;\n"
            "    vec4 fGreen;\n"
            "    layout(row_major) mat4 worldToProj;\n"
            "    layout(row_major) mat4 projToWorld;\n"
            "    layout(row_major) mat4 worldToView;\n"
            "    layout(row_major) mat4 viewToProj;\n"
            "    layout(row_major) mat4 worldToShadow[4];\n"
            "    float fZero;\n"
            "    float fOne;\n"
            "    float fTwo;\n"
            "    float fThree;\n"
            "    vec3 fZeroZeroZero;\n"
            "    float fFour;\n"
            "    vec3 fZeroZeroOne;\n"
            "    float fFive;\n"
            "    vec3 fZeroOneZero;\n"
            "    float fSix;\n"
            "    float fSeven;\n"
            "    float fEight;\n"
            "    float fNine;\n"
            "    vec2 fZeroZero;\n"
            "    vec2 fZeroOne;\n"
            "    vec4 fBlue;\n"
            "    vec2 fOneZero;\n"
            "    vec2 fOneOne;\n"
            "    vec3 fZeroOneOne;\n"
            "    float fTen;\n"
            "    float fEleven;\n"
            "    float fTwelve;\n"
            "    vec3 fOneZeroZero;\n"
            "    vec4 uvOffsets[4];\n"
            "};\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 0) out vec4 uFragColor;\n"
            "void main() {\n"
            "   vec4 right = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "   vec4 wrong = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "   \n"

            // start with VS value to ensure it passed
            "   vec4 outColor = color;\n"

            // do some exact comparisons, even though we should
            // really have an epsilon involved.
            "   if (fRed != vec4(1.0, 0.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fGreen != vec4(0.0, 1.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (projToWorld[1] != vec4(0.0, 2.0, 0.0, 0.0))\n"
            "       outColor = wrong;\n"
            "   if (worldToShadow[2][1] != vec4(0.0, 7.0, 0.0, 0.0))\n"
            "       outColor = wrong;\n"
            "   if (fTwo != 2.0)\n"
            "       outColor = wrong;\n"
            "   if (fOneOne != vec2(1.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fTen != 10.0)\n"
            "       outColor = wrong;\n"
            "   if (uvOffsets[2] != vec4(0.9, 1.0, 1.1, 1.2))\n"
            "       outColor = wrong;\n"
            "   \n"
            "   uFragColor = outColor;\n"
            "}\n";


    const float mixedVals[196] = {   1.0f, 0.0f, 0.0f, 1.0f,   //        vec4 fRed;            // align
                                     0.0f, 1.0f, 0.0f, 1.0f,   //        vec4 fGreen;          // align
                                     1.0f, 0.0f, 0.0f, 1.0f,   //        layout(row_major) mat4 worldToProj;
                                     0.0f, 1.0f, 0.0f, 1.0f,   //        align
                                     0.0f, 0.0f, 1.0f, 1.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 1.0f,   //        align
                                     2.0f, 0.0f, 0.0f, 2.0f,   //        layout(row_major) mat4 projToWorld;
                                     0.0f, 2.0f, 0.0f, 2.0f,   //        align
                                     0.0f, 0.0f, 2.0f, 2.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 2.0f,   //        align
                                     3.0f, 0.0f, 0.0f, 3.0f,   //        layout(row_major) mat4 worldToView;
                                     0.0f, 3.0f, 0.0f, 3.0f,   //        align
                                     0.0f, 0.0f, 3.0f, 3.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 3.0f,   //        align
                                     4.0f, 0.0f, 0.0f, 4.0f,   //        layout(row_major) mat4 viewToProj;
                                     0.0f, 4.0f, 0.0f, 4.0f,   //        align
                                     0.0f, 0.0f, 4.0f, 4.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 4.0f,   //        align
                                     5.0f, 0.0f, 0.0f, 5.0f,   //        layout(row_major) mat4 worldToShadow[4];
                                     0.0f, 5.0f, 0.0f, 5.0f,   //        align
                                     0.0f, 0.0f, 5.0f, 5.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 5.0f,   //        align
                                     6.0f, 0.0f, 0.0f, 6.0f,   //        align
                                     0.0f, 6.0f, 0.0f, 6.0f,   //        align
                                     0.0f, 0.0f, 6.0f, 6.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 6.0f,   //        align
                                     7.0f, 0.0f, 0.0f, 7.0f,   //        align
                                     0.0f, 7.0f, 0.0f, 7.0f,   //        align
                                     0.0f, 0.0f, 7.0f, 7.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 7.0f,   //        align
                                     8.0f, 0.0f, 0.0f, 8.0f,   //        align
                                     0.0f, 8.0f, 0.0f, 8.0f,   //        align
                                     0.0f, 0.0f, 8.0f, 8.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 8.0f,   //        align
                                     0.0f,                     //        float fZero;          // align
                                     1.0f,                     //        float fOne;           // pack
                                     2.0f,                     //        float fTwo;           // pack
                                     3.0f,                     //        float fThree;         // pack
                                     0.0f, 0.0f, 0.0f,         //        vec3 fZeroZeroZero;   // align
                                     4.0f,                     //        float fFour;          // pack
                                     0.0f, 0.0f, 1.0f,         //        vec3 fZeroZeroOne;    // align
                                     5.0f,                     //        float fFive;          // pack
                                     0.0f, 1.0f, 0.0f,         //        vec3 fZeroOneZero;    // align
                                     6.0f,                     //        float fSix;           // pack
                                     7.0f,                     //        float fSeven;         // align
                                     8.0f,                     //        float fEight;         // pack
                                     9.0f,                     //        float fNine;          // pack
                                     0.0f,                     //        BUFFER
                                     0.0f, 0.0f,               //        vec2 fZeroZero;       // align
                                     0.0f, 1.0f,               //        vec2 fZeroOne;        // pack
                                     0.0f, 0.0f, 1.0f, 1.0f,   //        vec4 fBlue;           // align
                                     1.0f, 0.0f,               //        vec2 fOneZero;        // align
                                     1.0f, 1.0f,               //        vec2 fOneOne;         // pack
                                     0.0f, 1.0f, 1.0f,         //        vec3 fZeroOneOne;     // align
                                     10.0f,                    //        float fTen;           // pack
                                     11.0f,                    //        float fEleven;        // align
                                     12.0f,                    //        float fTwelve;        // pack
                                     0.0f, 0.0f,               //        BUFFER
                                     1.0f, 0.0f, 0.0f,         //        vec3 fOneZeroZero;    // align
                                     0.0f,                     //        BUFFER
                                     0.1f, 0.2f, 0.3f, 0.4f,   //        vec4 uvOffsets[4];
                                     0.5f, 0.6f, 0.7f, 0.8f,   //        align
                                     0.9f, 1.0f, 1.1f, 1.2f,   //        align
                                     1.3f, 1.4f, 1.5f, 1.6f,   //        align
                                  };

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    const int constCount   = sizeof(mixedVals)   / sizeof(float);

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkConstantBufferObj mixedBuffer(m_device, constCount, sizeof(mixedVals[0]), (const void*) mixedVals);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mixedBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

#ifdef DUMP_STATE_DOT
    DRAW_STATE_DUMP_DOT_FILE pDSDumpDot = (DRAW_STATE_DUMP_DOT_FILE)vkGetProcAddr(gpu(), (char*)"drawStateDumpDotFile");
    pDSDumpDot((char*)"triTest2.dot");
#endif
    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, TextureGather)
{
    // This test introduces textureGather and textureGatherOffset
    // Each call is compared against an expected inline color result
    // Green triangle means everything worked as expected
    // Red means something went wrong

    // disable SPV until texture gather is turned on in glsl->SPV
    if (!m_use_glsl) {
        printf("Skipping test that requires GLSL path (TextureGather)\n");
        return;
    }

    ScopedUseGlsl useGlsl(true);

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
            "layout (binding = 0) uniform sampler2D surface0;\n"
            "layout (binding = 1) uniform sampler2D surface1;\n"
            "layout (binding = 2) uniform sampler2D surface2;\n"
            "layout (binding = 3) uniform sampler2D surface3;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"

            "   vec4 right = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "   vec4 wrong = vec4(1.0, 0.0, 0.0, 1.0);\n"

            "   vec4 color = right;\n"

            // Grab a normal texture sample to ensure it can work in conjuntion
            // with textureGather (there are some intracacies in the backend)
            "   vec4 sampledColor = textureLod(surface2, vec2(0.5), 0.0);\n"
            "   if (sampledColor != vec4(0.0, 0.0, 1.0, 1.0))\n"
            "       color = wrong;\n"

            "   vec4 gatheredColor = textureGather(surface0, vec2(0.5), 0);\n"
            // This just grabbed four red components from a red surface
            "   if (gatheredColor != vec4(1.0, 1.0, 1.0, 1.0))\n"
            "       color = wrong;\n"

            // Yes, this is using an offset of 0, we don't have enough fine grained
            // control of the texture contents here.
            "   gatheredColor = textureGatherOffset(surface1, vec2(0.5), ivec2(0, 0), 1);\n"
            "   if (gatheredColor != vec4(1.0, 1.0, 1.0, 1.0))\n"
            "       color = wrong;\n"

            "   outColor = color;\n"

            "}\n";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    uint32_t tex_colors[2] = { 0xffff0000, 0xffff0000 };
    VkSamplerObj sampler0(m_device);
    VkTextureObj texture0(m_device, tex_colors); // Red
    tex_colors[0] = 0xff00ff00; tex_colors[1] = 0xff00ff00;
    VkSamplerObj sampler1(m_device);
    VkTextureObj texture1(m_device, tex_colors); // Green
    tex_colors[0] = 0xff0000ff; tex_colors[1] = 0xff0000ff;
    VkSamplerObj sampler2(m_device);
    VkTextureObj texture2(m_device, tex_colors); // Blue
    tex_colors[0] = 0xffff00ff; tex_colors[1] = 0xffff00ff;
    VkSamplerObj sampler3(m_device);
    VkTextureObj texture3(m_device, tex_colors); // Red and Blue

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler0, &texture0);
    descriptorSet.AppendSamplerTexture(&sampler1, &texture1);
    descriptorSet.AppendSamplerTexture(&sampler2, &texture2);
    descriptorSet.AppendSamplerTexture(&sampler3, &texture3);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet);

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, GeometryShaderHelloWorld)
{
    // This test introduces a geometry shader that simply
    // changes the color of each vertex to red, green, blue

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) out vec4 color;"
            "void main() {\n"

            // VS writes out red
            "   color = vec4(1.0, 0.0, 0.0, 1.0);\n"

            // generic position stuff
            "   vec2 vertices;\n"
            "   int vertexSelector = gl_VertexID;\n"
            "   if (vertexSelector == 0)\n"
            "      vertices = vec2(-0.5, -0.5);\n"
            "   else if (vertexSelector == 1)\n"
            "      vertices = vec2( 0.5, -0.5);\n"
            "   else if (vertexSelector == 2)\n"
            "      vertices = vec2( 0.5, 0.5);\n"
            "   else\n"
            "      vertices = vec2( 0.0,  0.0);\n"
            "   gl_Position = vec4(vertices, 0.0, 1.0);\n"

            "}\n";

    static const char *geomShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout( triangles ) in;\n"
            "layout( triangle_strip, max_vertices = 3 ) out;\n"
            "layout( location = 0 ) in vec4 inColor[3];\n"
            "layout( location = 0 ) out vec4 outColor;\n"
            "void main()\n"
            "{\n"

            // first vertex, pass through red
            "    gl_Position = gl_in[0].gl_Position;\n"
            "    outColor = inColor[0];\n"
            "    EmitVertex();\n"

            // second vertex, green
            "    gl_Position = gl_in[1].gl_Position;\n"
            "    outColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "    EmitVertex();\n"

            // third vertex, blue
            "    gl_Position = gl_in[2].gl_Position;\n"
            "    outColor = vec4(0.0, 0.0, 1.0, 1.0);\n"
            "    EmitVertex();\n"

            // done
            "    EndPrimitive();\n"
            "}\n";


    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            // pass through
            "   outColor = color;\n"
            "}\n";



    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device, vertShaderText, VK_SHADER_STAGE_VERTEX,   this);
    VkShaderObj gs(m_device, geomShaderText, VK_SHADER_STAGE_GEOMETRY, this);
    VkShaderObj ps(m_device, fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&gs);
    pipelineobj.AddShader(&ps);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    VkDescriptorSetObj descriptorSet(m_device);

    GenericDrawPreparation(pipelineobj, descriptorSet);

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, GSUniformBufferLayout)
{
    // This test is just like TriangleUniformBufferLayout but adds
    // geometry as a stage that also does UBO lookups
    // The expected result from this test is a green triangle

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform mixedBuffer {\n"
            "    vec4 fRed;\n"
            "    vec4 fGreen;\n"
            "    layout(row_major) mat4 worldToProj;\n"
            "    layout(row_major) mat4 projToWorld;\n"
            "    layout(row_major) mat4 worldToView;\n"
            "    layout(row_major) mat4 viewToProj;\n"
            "    layout(row_major) mat4 worldToShadow[4];\n"
            "    float fZero;\n"
            "    float fOne;\n"
            "    float fTwo;\n"
            "    float fThree;\n"
            "    vec3 fZeroZeroZero;\n"
            "    float fFour;\n"
            "    vec3 fZeroZeroOne;\n"
            "    float fFive;\n"
            "    vec3 fZeroOneZero;\n"
            "    float fSix;\n"
            "    float fSeven;\n"
            "    float fEight;\n"
            "    float fNine;\n"
            "    vec2 fZeroZero;\n"
            "    vec2 fZeroOne;\n"
            "    vec4 fBlue;\n"
            "    vec2 fOneZero;\n"
            "    vec2 fOneOne;\n"
            "    vec3 fZeroOneOne;\n"
            "    float fTen;\n"
            "    float fEleven;\n"
            "    float fTwelve;\n"
            "    vec3 fOneZeroZero;\n"
            "    vec4 uvOffsets[4];\n"
            "};\n"
            "layout (location = 0) out vec4 color;"
            "void main() {\n"

            "   vec4 right = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "   vec4 wrong = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "   \n"

            // do some exact comparisons, even though we should
            // really have an epsilon involved.
            "   vec4 outColor = right;\n"
            "   if (fRed != vec4(1.0, 0.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fGreen != vec4(0.0, 1.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fBlue != vec4(0.0, 0.0, 1.0, 1.0))\n"
            "       outColor = wrong;\n"

            "   color = outColor;\n"

            // generic position stuff
            "   vec2 vertices;\n"
            "   int vertexSelector = gl_VertexID;\n"
            "   if (vertexSelector == 0)\n"
            "      vertices = vec2(-0.5, -0.5);\n"
            "   else if (vertexSelector == 1)\n"
            "      vertices = vec2( 0.5, -0.5);\n"
            "   else if (vertexSelector == 2)\n"
            "      vertices = vec2( 0.5, 0.5);\n"
            "   else\n"
            "      vertices = vec2( 0.0,  0.0);\n"
            "   gl_Position = vec4(vertices, 0.0, 1.0);\n"
            "}\n";

    static const char *geomShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"

            // GS layout stuff
            "layout( triangles ) in;\n"
            "layout( triangle_strip, max_vertices = 3 ) out;\n"

            // Between stage IO
            "layout( location = 0 ) in vec4 inColor[3];\n"
            "layout( location = 0 ) out vec4 color;\n"

            "layout (std140, binding = 0) uniform mixedBuffer {\n"
            "    vec4 fRed;\n"
            "    vec4 fGreen;\n"
            "    layout(row_major) mat4 worldToProj;\n"
            "    layout(row_major) mat4 projToWorld;\n"
            "    layout(row_major) mat4 worldToView;\n"
            "    layout(row_major) mat4 viewToProj;\n"
            "    layout(row_major) mat4 worldToShadow[4];\n"
            "    float fZero;\n"
            "    float fOne;\n"
            "    float fTwo;\n"
            "    float fThree;\n"
            "    vec3 fZeroZeroZero;\n"
            "    float fFour;\n"
            "    vec3 fZeroZeroOne;\n"
            "    float fFive;\n"
            "    vec3 fZeroOneZero;\n"
            "    float fSix;\n"
            "    float fSeven;\n"
            "    float fEight;\n"
            "    float fNine;\n"
            "    vec2 fZeroZero;\n"
            "    vec2 fZeroOne;\n"
            "    vec4 fBlue;\n"
            "    vec2 fOneZero;\n"
            "    vec2 fOneOne;\n"
            "    vec3 fZeroOneOne;\n"
            "    float fTen;\n"
            "    float fEleven;\n"
            "    float fTwelve;\n"
            "    vec3 fOneZeroZero;\n"
            "    vec4 uvOffsets[4];\n"
            "};\n"

            "void main()\n"
            "{\n"

            "   vec4 right = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "   vec4 wrong = vec4(1.0, 0.0, 0.0, 1.0);\n"

            // Each vertex will validate it can read VS output
            // then check a few values from the UBO

            // first vertex
            "   vec4 outColor = inColor[0];\n"

            "   if (fRed != vec4(1.0, 0.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fGreen != vec4(0.0, 1.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fBlue != vec4(0.0, 0.0, 1.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (projToWorld[1] != vec4(0.0, 2.0, 0.0, 0.0))\n"
            "       outColor = wrong;\n"

            "   gl_Position = gl_in[0].gl_Position;\n"
            "   color = outColor;\n"
            "   EmitVertex();\n"

            // second vertex
            "   outColor = inColor[1];\n"

            "   if (worldToShadow[2][1] != vec4(0.0, 7.0, 0.0, 0.0))\n"
            "       outColor = wrong;\n"
            "   if (fSix != 6.0)\n"
            "       outColor = wrong;\n"
            "   if (fOneOne != vec2(1.0, 1.0))\n"
            "       outColor = wrong;\n"

            "   gl_Position = gl_in[1].gl_Position;\n"
            "   color = outColor;\n"
            "   EmitVertex();\n"

            // third vertex
            "   outColor = inColor[2];\n"

            "   if (fSeven != 7.0)\n"
            "       outColor = wrong;\n"
            "   if (uvOffsets[2] != vec4(0.9, 1.0, 1.1, 1.2))\n"
            "       outColor = wrong;\n"

            "   gl_Position = gl_in[2].gl_Position;\n"
            "   color = outColor;\n"
            "   EmitVertex();\n"

            // done
            "    EndPrimitive();\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform mixedBuffer {\n"
            "    vec4 fRed;\n"
            "    vec4 fGreen;\n"
            "    layout(row_major) mat4 worldToProj;\n"
            "    layout(row_major) mat4 projToWorld;\n"
            "    layout(row_major) mat4 worldToView;\n"
            "    layout(row_major) mat4 viewToProj;\n"
            "    layout(row_major) mat4 worldToShadow[4];\n"
            "    float fZero;\n"
            "    float fOne;\n"
            "    float fTwo;\n"
            "    float fThree;\n"
            "    vec3 fZeroZeroZero;\n"
            "    float fFour;\n"
            "    vec3 fZeroZeroOne;\n"
            "    float fFive;\n"
            "    vec3 fZeroOneZero;\n"
            "    float fSix;\n"
            "    float fSeven;\n"
            "    float fEight;\n"
            "    float fNine;\n"
            "    vec2 fZeroZero;\n"
            "    vec2 fZeroOne;\n"
            "    vec4 fBlue;\n"
            "    vec2 fOneZero;\n"
            "    vec2 fOneOne;\n"
            "    vec3 fZeroOneOne;\n"
            "    float fTen;\n"
            "    float fEleven;\n"
            "    float fTwelve;\n"
            "    vec3 fOneZeroZero;\n"
            "    vec4 uvOffsets[4];\n"
            "};\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 0) out vec4 uFragColor;\n"
            "void main() {\n"
            "   vec4 right = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "   vec4 wrong = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "   \n"

            // start with GS value to ensure it passed
            "   vec4 outColor = color;\n"

            // do some exact comparisons, even though we should
            // really have an epsilon involved.
            "   if (fRed != vec4(1.0, 0.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fGreen != vec4(0.0, 1.0, 0.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (projToWorld[1] != vec4(0.0, 2.0, 0.0, 0.0))\n"
            "       outColor = wrong;\n"
            "   if (worldToShadow[2][1] != vec4(0.0, 7.0, 0.0, 0.0))\n"
            "       outColor = wrong;\n"
            "   if (fTwo != 2.0)\n"
            "       outColor = wrong;\n"
            "   if (fOneOne != vec2(1.0, 1.0))\n"
            "       outColor = wrong;\n"
            "   if (fTen != 10.0)\n"
            "       outColor = wrong;\n"
            "   if (uvOffsets[2] != vec4(0.9, 1.0, 1.1, 1.2))\n"
            "       outColor = wrong;\n"
            "   \n"
            "   uFragColor = outColor;\n"
            "}\n";


    const float mixedVals[196] = {   1.0f, 0.0f, 0.0f, 1.0f,   //        vec4 fRed;            // align
                                     0.0f, 1.0f, 0.0f, 1.0f,   //        vec4 fGreen;          // align
                                     1.0f, 0.0f, 0.0f, 1.0f,   //        layout(row_major) mat4 worldToProj;
                                     0.0f, 1.0f, 0.0f, 1.0f,   //        align
                                     0.0f, 0.0f, 1.0f, 1.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 1.0f,   //        align
                                     2.0f, 0.0f, 0.0f, 2.0f,   //        layout(row_major) mat4 projToWorld;
                                     0.0f, 2.0f, 0.0f, 2.0f,   //        align
                                     0.0f, 0.0f, 2.0f, 2.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 2.0f,   //        align
                                     3.0f, 0.0f, 0.0f, 3.0f,   //        layout(row_major) mat4 worldToView;
                                     0.0f, 3.0f, 0.0f, 3.0f,   //        align
                                     0.0f, 0.0f, 3.0f, 3.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 3.0f,   //        align
                                     4.0f, 0.0f, 0.0f, 4.0f,   //        layout(row_major) mat4 viewToProj;
                                     0.0f, 4.0f, 0.0f, 4.0f,   //        align
                                     0.0f, 0.0f, 4.0f, 4.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 4.0f,   //        align
                                     5.0f, 0.0f, 0.0f, 5.0f,   //        layout(row_major) mat4 worldToShadow[4];
                                     0.0f, 5.0f, 0.0f, 5.0f,   //        align
                                     0.0f, 0.0f, 5.0f, 5.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 5.0f,   //        align
                                     6.0f, 0.0f, 0.0f, 6.0f,   //        align
                                     0.0f, 6.0f, 0.0f, 6.0f,   //        align
                                     0.0f, 0.0f, 6.0f, 6.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 6.0f,   //        align
                                     7.0f, 0.0f, 0.0f, 7.0f,   //        align
                                     0.0f, 7.0f, 0.0f, 7.0f,   //        align
                                     0.0f, 0.0f, 7.0f, 7.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 7.0f,   //        align
                                     8.0f, 0.0f, 0.0f, 8.0f,   //        align
                                     0.0f, 8.0f, 0.0f, 8.0f,   //        align
                                     0.0f, 0.0f, 8.0f, 8.0f,   //        align
                                     0.0f, 0.0f, 0.0f, 8.0f,   //        align
                                     0.0f,                     //        float fZero;          // align
                                     1.0f,                     //        float fOne;           // pack
                                     2.0f,                     //        float fTwo;           // pack
                                     3.0f,                     //        float fThree;         // pack
                                     0.0f, 0.0f, 0.0f,         //        vec3 fZeroZeroZero;   // align
                                     4.0f,                     //        float fFour;          // pack
                                     0.0f, 0.0f, 1.0f,         //        vec3 fZeroZeroOne;    // align
                                     5.0f,                     //        float fFive;          // pack
                                     0.0f, 1.0f, 0.0f,         //        vec3 fZeroOneZero;    // align
                                     6.0f,                     //        float fSix;           // pack
                                     7.0f,                     //        float fSeven;         // align
                                     8.0f,                     //        float fEight;         // pack
                                     9.0f,                     //        float fNine;          // pack
                                     0.0f,                     //        BUFFER
                                     0.0f, 0.0f,               //        vec2 fZeroZero;       // align
                                     0.0f, 1.0f,               //        vec2 fZeroOne;        // pack
                                     0.0f, 0.0f, 1.0f, 1.0f,   //        vec4 fBlue;           // align
                                     1.0f, 0.0f,               //        vec2 fOneZero;        // align
                                     1.0f, 1.0f,               //        vec2 fOneOne;         // pack
                                     0.0f, 1.0f, 1.0f,         //        vec3 fZeroOneOne;     // align
                                     10.0f,                    //        float fTen;           // pack
                                     11.0f,                    //        float fEleven;        // align
                                     12.0f,                    //        float fTwelve;        // pack
                                     0.0f, 0.0f,               //        BUFFER
                                     1.0f, 0.0f, 0.0f,         //        vec3 fOneZeroZero;    // align
                                     0.0f,                     //        BUFFER
                                     0.1f, 0.2f, 0.3f, 0.4f,   //        vec4 uvOffsets[4];
                                     0.5f, 0.6f, 0.7f, 0.8f,   //        align
                                     0.9f, 1.0f, 1.1f, 1.2f,   //        align
                                     1.3f, 1.4f, 1.5f, 1.6f,   //        align
                                  };



    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    const int constCount   = sizeof(mixedVals)   / sizeof(float);

    VkShaderObj vs(m_device, vertShaderText, VK_SHADER_STAGE_VERTEX,   this);
    VkShaderObj gs(m_device, geomShaderText, VK_SHADER_STAGE_GEOMETRY, this);
    VkShaderObj ps(m_device, fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkConstantBufferObj mixedBuffer(m_device, constCount, sizeof(mixedVals[0]), (const void*) mixedVals);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&gs);
    pipelineobj.AddShader(&ps);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mixedBuffer);

    GenericDrawPreparation(pipelineobj, descriptorSet);

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, GSPositions)
{
    // This test adds more inputs from the vertex shader and perturbs positions
    // Expected result is white triangle with weird positions

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"

            "layout(location = 0) out vec3 out_a;\n"
            "layout(location = 1) out vec3 out_b;\n"
            "layout(location = 2) out vec3 out_c;\n"

            "void main() {\n"

            // write a solid color to each
            "   out_a = vec3(1.0, 0.0, 0.0);\n"
            "   out_b = vec3(0.0, 1.0, 0.0);\n"
            "   out_c = vec3(0.0, 0.0, 1.0);\n"

            // generic position stuff
            "   vec2 vertices;\n"
            "   int vertexSelector = gl_VertexID;\n"
            "   if (vertexSelector == 0)\n"
            "      vertices = vec2(-0.5, -0.5);\n"
            "   else if (vertexSelector == 1)\n"
            "      vertices = vec2( 0.5, -0.5);\n"
            "   else if (vertexSelector == 2)\n"
            "      vertices = vec2( 0.5, 0.5);\n"
            "   else\n"
            "      vertices = vec2( 0.0,  0.0);\n"
            "   gl_Position = vec4(vertices, 0.0, 1.0);\n"

            "}\n";

    static const char *geomShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout( triangles ) in;\n"
            "layout( triangle_strip, max_vertices = 3 ) out;\n"

            "layout(location = 0) in vec3 in_a[3];\n"
            "layout(location = 1) in vec3 in_b[3];\n"
            "layout(location = 2) in vec3 in_c[3];\n"

            "layout(location = 0) out vec3 out_a;\n"
            "layout(location = 1) out vec3 out_b;\n"
            "layout(location = 2) out vec3 out_c;\n"

            "void main()\n"
            "{\n"

            "    gl_Position = gl_in[0].gl_Position;\n"
            "    gl_Position.xy *= vec2(0.75);\n"
            "    out_a = in_a[0];\n"
            "    out_b = in_b[0];\n"
            "    out_c = in_c[0];\n"
            "    EmitVertex();\n"

            "    gl_Position = gl_in[1].gl_Position;\n"
            "    gl_Position.xy *= vec2(1.5);\n"
            "    out_a = in_a[1];\n"
            "    out_b = in_b[1];\n"
            "    out_c = in_c[1];\n"
            "    EmitVertex();\n"

            "    gl_Position = gl_in[2].gl_Position;\n"
            "    gl_Position.xy *= vec2(-0.1);\n"
            "    out_a = in_a[2];\n"
            "    out_b = in_b[2];\n"
            "    out_c = in_c[2];\n"
            "    EmitVertex();\n"

            "    EndPrimitive();\n"
            "}\n";


    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"

            "layout(location = 0) in vec3 in_a;\n"
            "layout(location = 1) in vec3 in_b;\n"
            "layout(location = 2) in vec3 in_c;\n"
            "layout (location = 0) out vec4 outColor;\n"

            "void main() {\n"
            "   outColor = vec4(in_a.x, in_b.y, in_c.z, 1.0);\n"
            "}\n";



    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device, vertShaderText, VK_SHADER_STAGE_VERTEX,   this);
    VkShaderObj gs(m_device, geomShaderText, VK_SHADER_STAGE_GEOMETRY, this);
    VkShaderObj ps(m_device, fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&gs);
    pipelineobj.AddShader(&ps);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    VkDescriptorSetObj descriptorSet(m_device);

    GenericDrawPreparation(pipelineobj, descriptorSet);

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, GSTriStrip)
{
    // This test emits multiple multiple triangles using a GS
    // Correct result is an multicolor circle

    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"

            "void main() {\n"

            // generic position stuff
            "   vec2 vertices;\n"
            "   int vertexSelector = gl_VertexID;\n"
            "   if (vertexSelector == 0)\n"
            "      vertices = vec2(-0.5, -0.5);\n"
            "   else if (vertexSelector == 1)\n"
            "      vertices = vec2( 0.5, -0.5);\n"
            "   else if (vertexSelector == 2)\n"
            "      vertices = vec2( 0.5, 0.5);\n"
            "   else\n"
            "      vertices = vec2( 0.0,  0.0);\n"
            "   gl_Position = vec4(vertices, 0.0, 1.0);\n"

            "}\n";

    static const char *geomShaderText =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout( triangles ) in;\n"
            "layout( triangle_strip, max_vertices = 18 ) out;\n"

            "layout(location = 0) out vec4 outColor;\n"

            "void main()\n"
            "{\n"
            // init with first position to get zw
            "    gl_Position = gl_in[0].gl_Position;\n"

            "    vec4 red    = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "    vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
            "    vec4 blue   = vec4(0.0, 0.0, 1.0, 1.0);\n"
            "    vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);\n"

            // different color per tri
            "    vec4[6] colors = { red,    white,   \n"
            "                       yellow, white,   \n"
            "                       blue,   white }; \n"

            // fan out the triangles
            "    vec2[18] positions = { vec2(0.0, 0.0), vec2(-0.5,   0.0), vec2(-0.25, -0.5),   \n"
            "                           vec2(0.0, 0.0), vec2(-0.25, -0.5), vec2( 0.25, -0.5),   \n"
            "                           vec2(0.0, 0.0), vec2( 0.25, -0.5), vec2( 0.5,   0.0),   \n"
            "                           vec2(0.0, 0.0), vec2( 0.5,   0.0), vec2( 0.25,  0.5),   \n"
            "                           vec2(0.0, 0.0), vec2( 0.25,  0.5), vec2(-0.25,  0.5),   \n"
            "                           vec2(0.0, 0.0), vec2(-0.25,  0.5), vec2(-0.5,   0.0) }; \n"

            // make a triangle list of 6
            "    for (int i = 0; i < 6; ++i) { \n"
            "        outColor = colors[i]; \n"
            "        for (int j = 0; j < 3; ++j) { \n"
            "            gl_Position.xy = positions[i * 3 + j]; \n"
            "            EmitVertex(); \n"
            "        } \n"
            "        EndPrimitive();\n"
            "    } \n"

            "}\n";


    static const char *fragShaderText =
            "#version 150\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"


            "layout(binding = 0) uniform windowDimensions {\n"
            "    vec4 dimensions;\n"
            "};\n"

            "layout(location = 0) in vec4 inColor;\n"
            "layout(origin_upper_left) in vec4 gl_FragCoord;\n"
            "layout (location = 0) out vec4 outColor;\n"

            "void main() {\n"

            // discard to make a nice circle
            "    vec2 pos = abs(gl_FragCoord.xy) - vec2(dimensions.x, dimensions.y) / 2;\n"
            "    float dist = sqrt(dot(pos, pos));\n"
            "    if (dist > 50.0)\n"
            "        discard;\n"

            "    outColor = inColor;\n"

            "}\n";



    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkShaderObj vs(m_device, vertShaderText, VK_SHADER_STAGE_VERTEX,   this);
    VkShaderObj gs(m_device, geomShaderText, VK_SHADER_STAGE_GEOMETRY, this);
    VkShaderObj ps(m_device, fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&gs);
    pipelineobj.AddShader(&ps);

    const float dimensions[4]  = { VkRenderFramework::m_width, VkRenderFramework::m_height , 0.0, 0.0};

    VkConstantBufferObj windowDimensions(m_device, sizeof(dimensions) / sizeof(dimensions[0]), sizeof(dimensions[0]), (const void*) dimensions);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, windowDimensions);

    GenericDrawPreparation(pipelineobj, descriptorSet);

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, RenderPassLoadOpClear)
{
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    /* clear via load op to full green */
    m_clear_via_load_op = true;
    m_clear_color.float32[0] = 0;
    m_clear_color.float32[1] = 1;
    m_clear_color.float32[2] = 0;
    m_clear_color.float32[3] = 0;
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());
    /* This command buffer contains ONLY the load op! */
    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

TEST_F(VkRenderTest, RenderPassAttachmentClear)
{
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    /* clear via load op to full red */
    m_clear_via_load_op = true;
    m_clear_color.float32[0] = 1;
    m_clear_color.float32[1] = 0;
    m_clear_color.float32[2] = 0;
    m_clear_color.float32[3] = 0;
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    /* Load op has cleared to red */

    /* Draw, draw, draw... */

    /* Now, partway through this renderpass we want to clear the color
     * attachment again, this time to green.
     */
    VkClearColorValue clear_color;
    clear_color.float32[0] = 0;
    clear_color.float32[1] = 1;
    clear_color.float32[2] = 0;
    clear_color.float32[3] = 0;
    VkRect3D clear_rect = { { 0, 0, 0 }, { (int)m_width, (int)m_height, 1 } };
    vkCmdClearColorAttachment(m_cmdBuffer->handle(), 0,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              &clear_color, 1, &clear_rect);

    EndCommandBuffer();
    QueueCommandBuffer();

    RecordImages(m_renderTargets);
}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    VkTestFramework::InitArgs(&argc, argv);

    ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    VkTestFramework::Finish();
    return result;
}
