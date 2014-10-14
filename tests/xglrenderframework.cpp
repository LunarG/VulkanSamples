/*
 * XGL Tests
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include "xglrenderframework.h"

XglRenderFramework::XglRenderFramework() :
    m_colorBlend( XGL_NULL_HANDLE ),
    m_stateMsaa( XGL_NULL_HANDLE ),
    m_stateDepthStencil( XGL_NULL_HANDLE ),
    m_stateRaster( XGL_NULL_HANDLE ),
    m_cmdBuffer( XGL_NULL_HANDLE ),
    m_stateViewport( XGL_NULL_HANDLE ),
    m_width( 256.0 ),                   // default window width
    m_height( 256.0 )                   // default window height
{
    m_render_target_fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_render_target_fmt.numericFormat = XGL_NUM_FMT_UNORM;
}

XglRenderFramework::~XglRenderFramework()
{

}

void XglRenderFramework::InitFramework()
{
    XGL_RESULT err;

    memset(&m_vtxBufferView, 0, sizeof(m_vtxBufferView));
    m_vtxBufferView.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;

    memset(&m_constantBufferView, 0, sizeof(m_constantBufferView));
    m_constantBufferView.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;

    err = xglInitAndEnumerateGpus(&app_info, NULL,
                                  MAX_GPUS, &this->gpu_count, objs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(1, this->gpu_count) << "No GPU available";

    m_device = new XglDevice(0, objs[0]);
    m_device->get_device_queue();
}

void XglRenderFramework::ShutdownFramework()
{
    if (m_colorBlend) xglDestroyObject(m_colorBlend);
    if (m_stateMsaa) xglDestroyObject(m_stateMsaa);
    if (m_stateDepthStencil) xglDestroyObject(m_stateDepthStencil);
    if (m_stateRaster) xglDestroyObject(m_stateRaster);
    if (m_cmdBuffer) xglDestroyObject(m_cmdBuffer);

    if (m_stateViewport) {
        xglDestroyObject(m_stateViewport);
    }

    if (m_renderTarget) {
        // TODO: XglImage should be able to destroy itself
//        m_renderTarget->
//        xglDestroyObject(*m_renderTarget);
    }

    // reset the driver
    xglInitAndEnumerateGpus(&this->app_info, XGL_NULL_HANDLE, 0, &gpu_count, XGL_NULL_HANDLE);
}

void XglRenderFramework::InitState()
{
    XGL_RESULT err;

    m_render_target_fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_render_target_fmt.numericFormat = XGL_NUM_FMT_UNORM;

    // create a raster state (solid, back-face culling)
    XGL_RASTER_STATE_CREATE_INFO raster = {};
    raster.sType = XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO;
    raster.fillMode = XGL_FILL_SOLID;
    raster.cullMode = XGL_CULL_NONE;
    raster.frontFace = XGL_FRONT_FACE_CCW;
    err = xglCreateRasterState( device(), &raster, &m_stateRaster );
    ASSERT_XGL_SUCCESS(err);

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
}

void XglRenderFramework::InitConstantBuffer(int constantCount, int constantSize,
                                            const void* data)
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

/*
 * Update existing constant value with new data of exactly
 * the same size.
 */
void XglRenderFramework::UpdateConstantBuffer(const void* data)
{
    XGL_RESULT err = XGL_SUCCESS;
    XGL_UINT8 *pData;

    err = xglMapMemory(m_constantBufferMem, 0, (XGL_VOID **) &pData);
    ASSERT_XGL_SUCCESS(err);

    memcpy(pData + this->m_constantBufferView.offset, data, this->m_constantBufferView.range);

    err = xglUnmapMemory(m_constantBufferMem);
    ASSERT_XGL_SUCCESS(err);
}

void XglRenderFramework::CreateQueryPool(XGL_QUERY_TYPE type, XGL_UINT slots,
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

void XglRenderFramework::DestroyQueryPool(XGL_QUERY_POOL pool, XGL_GPU_MEMORY mem)
{
    ASSERT_XGL_SUCCESS(xglBindObjectMemory(pool, XGL_NULL_HANDLE, 0));
    ASSERT_XGL_SUCCESS(xglFreeMemory(mem));
    ASSERT_XGL_SUCCESS(xglDestroyObject(pool));
}

void XglRenderFramework::CreateShader(XGL_PIPELINE_SHADER_STAGE stage,
                                      const char *shader_code,
                                      XGL_SHADER *pshader)
{
    XGL_RESULT err = XGL_SUCCESS;
    std::vector<unsigned int> bil;
    XGL_SHADER_CREATE_INFO createInfo;
    size_t shader_len;
    XGL_SHADER shader;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    if (!this->m_use_bil) {
        shader_len = strlen(shader_code);
        createInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
        createInfo.pCode = malloc(createInfo.codeSize);
        createInfo.flags = 0;

        /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
        ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
        ((uint32_t *) createInfo.pCode)[1] = 0;
        ((uint32_t *) createInfo.pCode)[2] = stage;
        memcpy(((uint32_t *) createInfo.pCode + 3), shader_code, shader_len + 1);

        err = xglCreateShader(device(), &createInfo, &shader);
        if (err) {
            free((void *) createInfo.pCode);
        }
    }

    // Only use BIL if GLSL compile fails or it's requested via m_use_bil
    if (this->m_use_bil || err) {
        // Use Reference GLSL to BIL compiler
        GLSLtoBIL(stage, shader_code, bil);
        createInfo.pCode = bil.data();
        createInfo.codeSize = bil.size() * sizeof(unsigned int);
        createInfo.flags = 0;
        err = xglCreateShader(device(), &createInfo, &shader);
    }

    ASSERT_XGL_SUCCESS(err);

    *pshader = shader;
}

void XglRenderFramework::InitViewport(float width, float height)
{
    XGL_RESULT err;

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
    m_width = width;
    m_height = height;
}

void XglRenderFramework::InitViewport()
{
    InitViewport(m_width, m_height);
}

void XglRenderFramework::InitRenderTarget()
{
    m_device->CreateImage(m_width, m_height, m_render_target_fmt,
                          XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
                          XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          &m_renderTarget);
}

void XglRenderFramework::CreateDefaultPipeline(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps)
{
    XGL_RESULT err;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs_stage;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO ps_stage;

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

void XglRenderFramework::GenerateBindRenderTargetCmd()
{
    // bind render target
    XGL_COLOR_ATTACHMENT_BIND_INFO colorBind = {};
    colorBind.view  = m_renderTarget->targetView();
    colorBind.colorAttachmentState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
    xglCmdBindAttachments(m_cmdBuffer, 1, &colorBind, NULL );
}

void XglRenderFramework::GenerateBindStateAndPipelineCmds(XGL_PIPELINE* pipeline)
{
    // set all states
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_RASTER, m_stateRaster );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_VIEWPORT, m_stateViewport );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_COLOR_BLEND, m_colorBlend);
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_DEPTH_STENCIL, m_stateDepthStencil );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_MSAA, m_stateMsaa );

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic memory view)
    xglCmdBindPipeline( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, *pipeline );

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic memory view)
    xglCmdBindDescriptorSet(m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
}

void XglRenderFramework::GenerateClearAndPrepareBufferCmds()
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
    transitionToClear.image = m_renderTarget->image();
    transitionToClear.oldState = m_renderTarget->state();
    transitionToClear.newState = XGL_IMAGE_STATE_CLEAR;
    transitionToClear.subresourceRange = srRange;
    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToClear );
    m_renderTarget->state(( XGL_IMAGE_STATE ) transitionToClear.newState);

    // clear the back buffer to dark grey
    XGL_UINT clearColor[4] = {64, 64, 64, 0};
    xglCmdClearColorImageRaw( m_cmdBuffer, m_renderTarget->image(), clearColor, 1, &srRange );

    // prepare back buffer for rendering
    XGL_IMAGE_STATE_TRANSITION transitionToRender = {};
    transitionToRender.image = m_renderTarget->image();
    transitionToRender.oldState = m_renderTarget->state();
    transitionToRender.newState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
    transitionToRender.subresourceRange = srRange;
    xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToRender );
    m_renderTarget->state(( XGL_IMAGE_STATE ) transitionToClear.newState);
}
