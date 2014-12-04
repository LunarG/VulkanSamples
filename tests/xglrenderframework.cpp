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
    m_cmdBuffer( XGL_NULL_HANDLE ),
    m_stateRaster( XGL_NULL_HANDLE ),
    m_colorBlend( XGL_NULL_HANDLE ),
    m_stateViewport( XGL_NULL_HANDLE ),
    m_stateDepthStencil( XGL_NULL_HANDLE ),
    m_width( 256.0 ),                   // default window width
    m_height( 256.0 )                   // default window height
{
    m_renderTargetCount = 1;

    m_render_target_fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_render_target_fmt.numericFormat = XGL_NUM_FMT_UNORM;

    m_depthStencilBinding.view = XGL_NULL_HANDLE;
}

XglRenderFramework::~XglRenderFramework()
{

}

void XglRenderFramework::InitFramework()
{
    XGL_RESULT err;

    err = xglInitAndEnumerateGpus(&app_info, NULL,
                                  XGL_MAX_PHYSICAL_GPUS, &this->gpu_count, objs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(this->gpu_count, 1) << "No GPU available";

    m_device = new XglDevice(0, objs[0]);
    m_device->get_device_queue();
}

void XglRenderFramework::ShutdownFramework()
{
    if (m_colorBlend) xglDestroyObject(m_colorBlend);
    if (m_stateDepthStencil) xglDestroyObject(m_stateDepthStencil);
    if (m_stateRaster) xglDestroyObject(m_stateRaster);
    if (m_cmdBuffer) xglDestroyObject(m_cmdBuffer);

    if (m_stateViewport) {
        xglDestroyObject(m_stateViewport);
    }

    // reset the driver
    delete m_device;
    xglInitAndEnumerateGpus(&this->app_info, XGL_NULL_HANDLE, 0, &gpu_count, XGL_NULL_HANDLE);
}

void XglRenderFramework::InitState()
{
    XGL_RESULT err;

    m_render_target_fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_render_target_fmt.numericFormat = XGL_NUM_FMT_UNORM;

    // create a raster state (solid, back-face culling)
    XGL_DYNAMIC_RS_STATE_CREATE_INFO raster = {};
    raster.sType = XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO;
    raster.pointSize = 1.0;

    err = xglCreateDynamicRasterState( device(), &raster, &m_stateRaster );
    ASSERT_XGL_SUCCESS(err);

    XGL_DYNAMIC_CB_STATE_CREATE_INFO blend = {};
    blend.sType = XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO;
    err = xglCreateDynamicColorBlendState(device(), &blend, &m_colorBlend);
    ASSERT_XGL_SUCCESS( err );

    XGL_DYNAMIC_DS_STATE_CREATE_INFO depthStencil = {};
    depthStencil.sType = XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO;
    depthStencil.minDepth = 0.f;
    depthStencil.maxDepth = 1.f;
    depthStencil.stencilFrontRef = 0;
    depthStencil.stencilBackRef = 0;

    err = xglCreateDynamicDepthStencilState( device(), &depthStencil, &m_stateDepthStencil );
    ASSERT_XGL_SUCCESS( err );

    XGL_CMD_BUFFER_CREATE_INFO cmdInfo = {};

    cmdInfo.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmdInfo.queueType = XGL_QUEUE_TYPE_GRAPHICS;
    err = xglCreateCommandBuffer(device(), &cmdInfo, &m_cmdBuffer);
    ASSERT_XGL_SUCCESS(err) << "xglCreateCommandBuffer failed";
}

void XglRenderFramework::InitViewport(float width, float height)
{
    XGL_RESULT err;

    XGL_VIEWPORT viewport;

    XGL_DYNAMIC_VP_STATE_CREATE_INFO viewportCreate = {};
    viewportCreate.sType = XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO;
    viewportCreate.viewportCount         = 1;
    viewportCreate.scissorCount          = 0;
    viewport.originX  = 0;
    viewport.originY  = 0;
    viewport.width    = 1.f * width;
    viewport.height   = 1.f * height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    viewportCreate.pViewports = &viewport;

    err = xglCreateDynamicViewportState( device(), &viewportCreate, &m_stateViewport );
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
    XGL_UINT i;

    for (i = 0; i < m_renderTargetCount; i++) {
        XglImage *img = new XglImage(m_device);
        img->init(m_width, m_height, m_render_target_fmt,
                XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
                XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        m_colorBindings[i].view  = img->targetView();
        m_colorBindings[i].layout = XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        m_renderTargets.push_back(img);
    }
      // Create Framebuffer and RenderPass with color attachments and any depth/stencil attachment
    XGL_ATTACHMENT_LOAD_OP load_op = XGL_ATTACHMENT_LOAD_OP_LOAD;
    XGL_ATTACHMENT_STORE_OP store_op = XGL_ATTACHMENT_STORE_OP_STORE;
    XGL_DEPTH_STENCIL_BIND_INFO *dsBinding;
    if (m_depthStencilBinding.view)
        dsBinding = &m_depthStencilBinding;
    else
        dsBinding = NULL;
    const XGL_FRAMEBUFFER_CREATE_INFO fb_info = {
         .sType = XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .colorAttachmentCount = m_renderTargetCount,
         .pColorAttachments = m_colorBindings,
         .pDepthStencilAttachment = dsBinding,
         .sampleCount = 1,
    };
    XGL_RENDER_PASS_CREATE_INFO rp_info;
    memset(&rp_info, 0 , sizeof(rp_info));
    xglCreateFramebuffer(device(), &fb_info, &(rp_info.framebuffer));
    rp_info.sType = XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.renderArea.extent.width = m_width;
    rp_info.renderArea.extent.height = m_height;
    rp_info.pColorLoadOps = &load_op;
    rp_info.pColorStoreOps = &store_op;
    rp_info.depthLoadOp = XGL_ATTACHMENT_LOAD_OP_LOAD;
    rp_info.depthStoreOp = XGL_ATTACHMENT_STORE_OP_STORE;
    rp_info.stencilLoadOp = XGL_ATTACHMENT_LOAD_OP_LOAD;
    rp_info.stencilStoreOp = XGL_ATTACHMENT_STORE_OP_STORE;
    xglCreateRenderPass(device(), &rp_info, &m_renderPass);
}

XglDevice::XglDevice(XGL_UINT id, XGL_PHYSICAL_GPU obj) :
    xgl_testing::Device(obj), id(id)
{
    init();

    props = gpu().properties();
    queue_props = &gpu().queue_properties()[0];
}

void XglDevice::get_device_queue()
{
    ASSERT_NE(true, graphics_queues().empty());
    m_queue = graphics_queues()[0]->obj();
}

XglDescriptorSetObj::XglDescriptorSetObj(XglDevice *device)
{
    m_device = device;
    m_nextSlot = 0;

}

void XglDescriptorSetObj::AttachBufferView(XglConstantBufferObj *constantBuffer)
{
    m_bufferViews.push_back(&constantBuffer->m_bufferViewInfo);
    m_bufferSlots.push_back(m_nextSlot);
    m_nextSlot++;

}

void XglDescriptorSetObj::AttachSampler(XglSamplerObj *sampler)
{
    m_samplers.push_back(sampler);
    m_samplerSlots.push_back(m_nextSlot);
    m_nextSlot++;

}

void XglDescriptorSetObj::AttachImageView(XglTextureObj *texture)
{
    m_imageViews.push_back(&texture->m_textureViewInfo);
    m_imageSlots.push_back(m_nextSlot);
    m_nextSlot++;

}

XGL_DESCRIPTOR_SLOT_INFO* XglDescriptorSetObj::GetSlotInfo(vector<int>slots,
                                                           vector<XGL_DESCRIPTOR_SET_SLOT_TYPE>types,
                                                           vector<void *>objs )
{
    int nSlots = m_bufferSlots.size() + m_imageSlots.size() + m_samplerSlots.size();
    m_slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( nSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    memset(m_slotInfo,0,nSlots*sizeof(XGL_DESCRIPTOR_SLOT_INFO));

    for (int i=0; i<nSlots; i++)
    {
        m_slotInfo[i].slotObjectType = XGL_SLOT_UNUSED;
    }

    for (int i=0; i<slots.size(); i++)
    {
        for (int j=0; j<m_bufferSlots.size(); j++)
        {
            if ( m_bufferViews[j] == objs[i])
            {
                m_slotInfo[m_bufferSlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_bufferSlots[j]].slotObjectType = types[i];
            }
        }
        for (int j=0; j<m_imageSlots.size(); j++)
        {
            if ( m_imageViews[j] == objs[i])
            {
                m_slotInfo[m_imageSlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_imageSlots[j]].slotObjectType = types[i];
            }
        }
        for (int j=0; j<m_samplerSlots.size(); j++)
        {
            if ( m_samplers[j] == objs[i])
            {
                m_slotInfo[m_samplerSlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_samplerSlots[j]].slotObjectType = types[i];
            }
        }
    }

    // for (int i=0;i<nSlots;i++)
    // {
    //    printf("SlotInfo[%d]:  Index = %d, Type = %d\n",i,m_slotInfo[i].shaderEntityIndex, m_slotInfo[i].slotObjectType);
    //    fflush(stdout);
    // }

    return(m_slotInfo);

}
void XglDescriptorSetObj::CreateXGLDescriptorSet()
{
    init(*m_device, xgl_testing::DescriptorSet::create_info(m_nextSlot));

    begin();
    clear();

    for (int i=0; i<m_bufferViews.size();i++)
    {
        attach(m_bufferSlots[i], *m_bufferViews[i]);
    }
    for (int i=0; i<m_samplers.size();i++)
    {
        attach(m_samplerSlots[i], *m_samplers[i]);
    }
    for (int i=0; i<m_imageViews.size();i++)
    {
        attach(m_imageSlots[i], *m_imageViews[i]);
    }

    end();
}

XGL_DESCRIPTOR_SET XglDescriptorSetObj::GetDescriptorSetHandle()
{
    return obj();
}

int XglDescriptorSetObj::GetTotalSlots()
{
    return m_nextSlot;
}

void XglDescriptorSetObj::BindCommandBuffer(XGL_CMD_BUFFER commandBuffer)
{
    init(*m_device, xgl_testing::DescriptorSet::create_info(m_nextSlot));

    begin();
    clear();

    for (int i=0; i<m_bufferViews.size();i++)
    {
        attach(m_bufferSlots[i], *m_bufferViews[i]);
    }
    for (int i=0; i<m_samplers.size();i++)
    {
        attach(m_samplerSlots[i], *m_samplers[i]);
    }
    for (int i=0; i<m_imageViews.size();i++)
    {
        attach(m_imageSlots[i], *m_imageViews[i]);
    }

    end();

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic buffer view)
    xglCmdBindDescriptorSet(commandBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, obj(), 0 );
}

XglImage::XglImage(XglDevice *dev)
{
    m_device = dev;
    m_imageInfo.view = XGL_NULL_HANDLE;
    m_imageInfo.layout = XGL_IMAGE_LAYOUT_GENERAL;
}

void XglImage::init(XGL_UINT32 w, XGL_UINT32 h,
               XGL_FORMAT fmt, XGL_FLAGS usage,
               XGL_IMAGE_TILING tiling)
{
    XGL_UINT mipCount;

    mipCount = 0;

    XGL_UINT _w = w;
    XGL_UINT _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

    XGL_IMAGE_CREATE_INFO imageCreateInfo = xgl_testing::Image::create_info();
    imageCreateInfo.imageType = XGL_IMAGE_2D;
    imageCreateInfo.format = fmt;
    imageCreateInfo.extent.width = w;
    imageCreateInfo.extent.height = h;
    imageCreateInfo.mipLevels = mipCount;
    imageCreateInfo.tiling = tiling;

    imageCreateInfo.usage = usage;

    xgl_testing::Image::init(*m_device, imageCreateInfo);

    m_imageInfo.layout = XGL_IMAGE_LAYOUT_GENERAL;

    XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO createView = {
        XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
        XGL_NULL_HANDLE,
        obj(),
        {XGL_CH_FMT_R8G8B8A8, XGL_NUM_FMT_UNORM},
        0,
        0,
        1
    };

    m_targetView.init(*m_device, createView);
}

XGL_RESULT XglImage::MapMemory(XGL_VOID** ptr)
{
    *ptr = map();
    return (*ptr) ? XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

XGL_RESULT XglImage::UnmapMemory()
{
    unmap();
    return XGL_SUCCESS;
}

XglTextureObj::XglTextureObj(XglDevice *device)
{
    m_device = device;
    const XGL_FORMAT tex_format = { XGL_CH_FMT_B8G8R8A8, XGL_NUM_FMT_UNORM };
    const uint32_t tex_colors[2] = { 0xffff0000, 0xff00ff00 };

    memset(&m_textureViewInfo,0,sizeof(m_textureViewInfo));

    m_textureViewInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;

    const XGL_IMAGE_CREATE_INFO image = {
        .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = XGL_IMAGE_2D,
        .format = tex_format,
        .extent = { 16, 16, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = XGL_LINEAR_TILING,
        .usage = XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT,
        .flags = 0,
    };

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

    /* create image */
    init(*m_device, image);

    /* create image view */
    view.image = obj();
    m_textureView.init(*m_device, view);

    XGL_SUBRESOURCE_LAYOUT layout =
        subresource_layout(subresource(XGL_IMAGE_ASPECT_COLOR, 0, 0));
    m_rowPitch = layout.rowPitch;

    XGL_VOID *data;
    XGL_INT x, y;

    data = map();

    for (y = 0; y < extent().height; y++) {
        uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
        for (x = 0; x < extent().width; x++)
            row[x] = tex_colors[(x & 1) ^ (y & 1)];
    }

    unmap();

    m_textureViewInfo.view = m_textureView.obj();

}

void XglTextureObj::ChangeColors(uint32_t color1, uint32_t color2)
{
    const uint32_t tex_colors[2] = { color1, color2 };
    XGL_VOID *data;

    data = map();

    for (int y = 0; y < extent().height; y++) {
        uint32_t *row = (uint32_t *) ((char *) data + m_rowPitch * y);
        for (int x = 0; x < extent().width; x++)
            row[x] = tex_colors[(x & 1) ^ (y & 1)];
    }

    unmap();
}

XglSamplerObj::XglSamplerObj(XglDevice *device)
{
    m_device = device;

    XGL_SAMPLER_CREATE_INFO samplerCreateInfo;
    memset(&samplerCreateInfo,0,sizeof(samplerCreateInfo));
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

    init(*m_device, samplerCreateInfo);
}

/*
 * Basic ConstantBuffer constructor. Then use create methods to fill in the details.
 */
XglConstantBufferObj::XglConstantBufferObj(XglDevice *device)
{
    m_device = device;
    m_commandBuffer = 0;

    memset(&m_bufferViewInfo,0,sizeof(m_bufferViewInfo));
}

XglConstantBufferObj::XglConstantBufferObj(XglDevice *device, int constantCount, int constantSize, const void* data)
{
    m_device = device;
    m_commandBuffer = 0;

    memset(&m_bufferViewInfo,0,sizeof(m_bufferViewInfo));
    m_numVertices = constantCount;
    m_stride = constantSize;

    const size_t allocationSize = constantCount * constantSize;
    init(*m_device, allocationSize);

    void *pData = map();
    memcpy(pData, data, allocationSize);
    unmap();

    // set up the buffer view for the constant buffer
    XGL_BUFFER_VIEW_CREATE_INFO view_info = {};
    view_info.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.buffer = obj();
    view_info.viewType = XGL_BUFFER_VIEW_TYPED;
    view_info.stride = 16;
    view_info.format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    view_info.format.numericFormat = XGL_NUM_FMT_FLOAT;
    view_info.channels.r = XGL_CHANNEL_SWIZZLE_R;
    view_info.channels.g = XGL_CHANNEL_SWIZZLE_G;
    view_info.channels.b = XGL_CHANNEL_SWIZZLE_B;
    view_info.channels.a = XGL_CHANNEL_SWIZZLE_A;
    view_info.offset = 0;
    view_info.range  = allocationSize;
    m_bufferView.init(*m_device, view_info);

    this->m_bufferViewInfo.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO;
    this->m_bufferViewInfo.view = m_bufferView.obj();
}

void XglConstantBufferObj::Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset, XGL_UINT binding)
{
    xglCmdBindVertexBuffer(cmdBuffer, obj(), offset, binding);
}


void XglConstantBufferObj::BufferMemoryBarrier(
        XGL_FLAGS outputMask /*=
            XGL_MEMORY_OUTPUT_CPU_WRITE_BIT |
            XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT |
            XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_COPY_BIT*/,
        XGL_FLAGS inputMask /*=
            XGL_MEMORY_INPUT_CPU_READ_BIT |
            XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
            XGL_MEMORY_INPUT_INDEX_FETCH_BIT |
            XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
            XGL_MEMORY_INPUT_UNIFORM_READ_BIT |
            XGL_MEMORY_INPUT_SHADER_READ_BIT |
            XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_COPY_BIT*/)
{
    XGL_RESULT err = XGL_SUCCESS;

    if (!m_commandBuffer)
    {
        m_fence.init(*m_device, xgl_testing::Fence::create_info(0));

        m_commandBuffer = new XglCommandBufferObj(m_device);

    }
    else
    {
        m_device->wait(m_fence);
    }

    // open the command buffer
    XGL_CMD_BUFFER_BEGIN_INFO cmd_buf_info = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    err = m_commandBuffer->BeginCommandBuffer(&cmd_buf_info);
    ASSERT_XGL_SUCCESS(err);

    XGL_BUFFER_MEMORY_BARRIER memory_barrier =
        buffer_memory_barrier(outputMask, inputMask, 0, m_numVertices * m_stride);

    XGL_SET_EVENT set_events[] = { XGL_SET_EVENT_GPU_COMMANDS_COMPLETE };
    XGL_PIPELINE_BARRIER pipeline_barrier = {};
    pipeline_barrier.sType = XGL_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = set_events;
    pipeline_barrier.waitEvent = XGL_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.pMemBarriers = &memory_barrier;

    // write barrier to the command buffer
    m_commandBuffer->PipelineBarrier(&pipeline_barrier);

    // finish recording the command buffer
    err = m_commandBuffer->EndCommandBuffer();
    ASSERT_XGL_SUCCESS(err);

    XGL_UINT32     numMemRefs=1;
    XGL_MEMORY_REF memRefs;
    // this command buffer only uses the vertex buffer memory
    memRefs.flags = 0;
    memRefs.mem = memories()[0];

    // submit the command buffer to the universal queue
    XGL_CMD_BUFFER bufferArray[1];
    bufferArray[0] = m_commandBuffer->GetBufferHandle();
    err = xglQueueSubmit( m_device->m_queue, 1, bufferArray, numMemRefs, &memRefs, m_fence.obj() );
    ASSERT_XGL_SUCCESS(err);
}

XglIndexBufferObj::XglIndexBufferObj(XglDevice *device)
    : XglConstantBufferObj(device)
{

}

void XglIndexBufferObj::CreateAndInitBuffer(int numIndexes, XGL_INDEX_TYPE indexType, const void* data)
{
    XGL_FORMAT viewFormat;

    m_numVertices = numIndexes;
    m_indexType = indexType;
    viewFormat.numericFormat = XGL_NUM_FMT_UINT;
    switch (indexType) {
    case XGL_INDEX_8:
        m_stride = 1;
        viewFormat.channelFormat = XGL_CH_FMT_R8;
        break;
    case XGL_INDEX_16:
        m_stride = 2;
        viewFormat.channelFormat = XGL_CH_FMT_R16;
        break;
    case XGL_INDEX_32:
        m_stride = 4;
        viewFormat.channelFormat = XGL_CH_FMT_R32;
        break;
    default:
        assert(!"unknown index type");
        break;
    }

    const size_t allocationSize = numIndexes * m_stride;
    init(*m_device, allocationSize);

    void *pData = map();
    memcpy(pData, data, allocationSize);
    unmap();

    // set up the buffer view for the constant buffer
    XGL_BUFFER_VIEW_CREATE_INFO view_info = {};
    view_info.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.buffer = obj();
    view_info.viewType = XGL_BUFFER_VIEW_TYPED;
    view_info.stride = m_stride;
    view_info.format.channelFormat = viewFormat.channelFormat;
    view_info.format.numericFormat = viewFormat.numericFormat;
    view_info.channels.r = XGL_CHANNEL_SWIZZLE_R;
    view_info.channels.g = XGL_CHANNEL_SWIZZLE_G;
    view_info.channels.b = XGL_CHANNEL_SWIZZLE_B;
    view_info.channels.a = XGL_CHANNEL_SWIZZLE_A;
    view_info.offset = 0;
    view_info.range  = allocationSize;
    m_bufferView.init(*m_device, view_info);

    this->m_bufferViewInfo.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO;
    this->m_bufferViewInfo.view = m_bufferView.obj();
}

void XglIndexBufferObj::Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset)
{
    xglCmdBindIndexBuffer(cmdBuffer, obj(), offset, m_indexType);
}

XGL_INDEX_TYPE XglIndexBufferObj::GetIndexType()
{
    return m_indexType;
}

XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* XglShaderObj::GetStageCreateInfo(XglDescriptorSetObj *descriptorSet)
{
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO *stageInfo = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) calloc( 1,sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO) );
    stageInfo->sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo->shader.stage = m_stage;
    stageInfo->shader.shader = obj();
    stageInfo->shader.descriptorSetMappingCount = 1;
    stageInfo->shader.pDescriptorSetMapping = (XGL_DESCRIPTOR_SET_MAPPING *)malloc(sizeof(XGL_DESCRIPTOR_SET_MAPPING));
    stageInfo->shader.pDescriptorSetMapping->descriptorCount = 0;
    stageInfo->shader.linkConstBufferCount = 0;
    stageInfo->shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    stageInfo->shader.dynamicBufferViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    stageInfo->shader.dynamicBufferViewMapping.shaderEntityIndex = 0;

    stageInfo->shader.pDescriptorSetMapping->descriptorCount = descriptorSet->GetTotalSlots();
    if (stageInfo->shader.pDescriptorSetMapping->descriptorCount)
    {
        vector<int> allSlots;
        vector<XGL_DESCRIPTOR_SET_SLOT_TYPE> allTypes;
        vector<void *> allObjs;

        allSlots.reserve(m_bufferSlots.size() + m_imageSlots.size() + m_samplerSlots.size());
        allTypes.reserve(m_bufferTypes.size() + m_imageTypes.size() + m_samplerTypes.size());
        allObjs.reserve(m_bufferObjs.size() + m_imageObjs.size() + m_samplerObjs.size());

        if (m_bufferSlots.size())
        {
            allSlots.insert(allSlots.end(), m_bufferSlots.begin(), m_bufferSlots.end());
            allTypes.insert(allTypes.end(), m_bufferTypes.begin(), m_bufferTypes.end());
            allObjs.insert(allObjs.end(), m_bufferObjs.begin(), m_bufferObjs.end());
        }
        if (m_imageSlots.size())
        {
            allSlots.insert(allSlots.end(), m_imageSlots.begin(), m_imageSlots.end());
            allTypes.insert(allTypes.end(), m_imageTypes.begin(), m_imageTypes.end());
            allObjs.insert(allObjs.end(), m_imageObjs.begin(), m_imageObjs.end());
        }
        if (m_samplerSlots.size())
        {
            allSlots.insert(allSlots.end(), m_samplerSlots.begin(), m_samplerSlots.end());
            allTypes.insert(allTypes.end(), m_samplerTypes.begin(), m_samplerTypes.end());
            allObjs.insert(allObjs.end(), m_samplerObjs.begin(), m_samplerObjs.end());
        }

         slotInfo = descriptorSet->GetSlotInfo(allSlots, allTypes, allObjs);
         stageInfo->shader.pDescriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    }
    return stageInfo;
}

void XglShaderObj::BindShaderEntitySlotToBuffer(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglConstantBufferObj *constantBuffer)
{
    m_bufferSlots.push_back(slot);
    m_bufferTypes.push_back(type);
    m_bufferObjs.push_back(&constantBuffer->m_bufferViewInfo);

}

void XglShaderObj::BindShaderEntitySlotToImage(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglTextureObj *texture)
{
    m_imageSlots.push_back(slot);
    m_imageTypes.push_back(type);
    m_imageObjs.push_back(&texture->m_textureViewInfo);

}

void XglShaderObj::BindShaderEntitySlotToSampler(int slot, XglSamplerObj *sampler)
{
    m_samplerSlots.push_back(slot);
    m_samplerTypes.push_back(XGL_SLOT_SHADER_SAMPLER);
    m_samplerObjs.push_back(sampler);

}

XglShaderObj::XglShaderObj(XglDevice *device, const char * shader_code, XGL_PIPELINE_SHADER_STAGE stage, XglRenderFramework *framework)
{
    XGL_RESULT err = XGL_SUCCESS;
    std::vector<unsigned int> bil;
    XGL_SHADER_CREATE_INFO createInfo;
    size_t shader_len;

    m_stage = stage;
    m_device = device;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    if (!framework->m_use_bil) {

        shader_len = strlen(shader_code);
        createInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
        createInfo.pCode = malloc(createInfo.codeSize);
        createInfo.flags = 0;

        /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
        ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
        ((uint32_t *) createInfo.pCode)[1] = 0;
        ((uint32_t *) createInfo.pCode)[2] = stage;
        memcpy(((uint32_t *) createInfo.pCode + 3), shader_code, shader_len + 1);

        err = init_try(*m_device, createInfo);
    }

    if (framework->m_use_bil || err) {
        std::vector<unsigned int> bil;
        err = XGL_SUCCESS;

        // Use Reference GLSL to BIL compiler
        framework->GLSLtoBIL(stage, shader_code, bil);
        createInfo.pCode = bil.data();
        createInfo.codeSize = bil.size() * sizeof(unsigned int);
        createInfo.flags = 0;

        init(*m_device, createInfo);
    }
}

XglPipelineObj::XglPipelineObj(XglDevice *device)
{
    m_device = device;
    m_vi_state.attributeCount = m_vi_state.bindingCount = 0;
    m_vertexBufferCount = 0;

    m_ia_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    m_ia_state.pNext = XGL_NULL_HANDLE;
    m_ia_state.topology = XGL_TOPOLOGY_TRIANGLE_LIST;
    m_ia_state.disableVertexReuse = XGL_FALSE;
    m_ia_state.primitiveRestartEnable = XGL_FALSE;
    m_ia_state.primitiveRestartIndex = 0;

    m_rs_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    m_rs_state.pNext = &m_ia_state;
    m_rs_state.depthClipEnable = XGL_FALSE;
    m_rs_state.rasterizerDiscardEnable = XGL_FALSE;
    m_rs_state.provokingVertex = XGL_PROVOKING_VERTEX_LAST;
    m_rs_state.fillMode = XGL_FILL_SOLID;
    m_rs_state.cullMode = XGL_CULL_NONE;
    m_rs_state.frontFace = XGL_FRONT_FACE_CCW;

    memset(&m_cb_state,0,sizeof(m_cb_state));
    m_cb_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    m_cb_state.pNext = &m_rs_state;
    m_cb_state.alphaToCoverageEnable = XGL_FALSE;
    m_cb_state.dualSourceBlendEnable = XGL_FALSE;
    m_cb_state.logicOp = XGL_LOGIC_OP_COPY;

    m_ms_state.pNext = &m_cb_state;
    m_ms_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
    m_ms_state.multisampleEnable = XGL_FALSE;
    m_ms_state.sampleMask = 1;                // Do we have to specify MSAA even just to disable it?
    m_ms_state.samples = 1;
    m_ms_state.minSampleShading = 0;
    m_ms_state.sampleShadingEnable = 0;

    m_ds_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO;
    m_ds_state.pNext = &m_ms_state,
    m_ds_state.format.channelFormat = XGL_CH_FMT_R32;
    m_ds_state.format.numericFormat = XGL_NUM_FMT_DS;
    m_ds_state.depthTestEnable      = XGL_FALSE;
    m_ds_state.depthWriteEnable     = XGL_FALSE;
    m_ds_state.depthBoundsEnable    = XGL_FALSE;
    m_ds_state.depthFunc = XGL_COMPARE_LESS_EQUAL;
    m_ds_state.back.stencilDepthFailOp = XGL_STENCIL_OP_KEEP;
    m_ds_state.back.stencilFailOp = XGL_STENCIL_OP_KEEP;
    m_ds_state.back.stencilPassOp = XGL_STENCIL_OP_KEEP;
    m_ds_state.back.stencilFunc = XGL_COMPARE_ALWAYS;
    m_ds_state.stencilTestEnable = XGL_FALSE;
    m_ds_state.front = m_ds_state.back;

    XGL_PIPELINE_CB_ATTACHMENT_STATE att = {};
    att.blendEnable = XGL_FALSE;
    att.format.channelFormat = XGL_CH_FMT_R8G8B8A8;
    att.format.numericFormat = XGL_NUM_FMT_UNORM;
    att.channelWriteMask = 0xf;
    AddColorAttachment(0, &att);

};

void XglPipelineObj::AddShader(XglShaderObj* shader)
{
    m_shaderObjs.push_back(shader);
}

void XglPipelineObj::AddVertexInputAttribs(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* vi_attrib, int count)
{
    m_vi_state.pVertexAttributeDescriptions = vi_attrib;
    m_vi_state.attributeCount = count;
}

void XglPipelineObj::AddVertexInputBindings(XGL_VERTEX_INPUT_BINDING_DESCRIPTION* vi_binding, int count)
{
    m_vi_state.pVertexBindingDescriptions = vi_binding;
    m_vi_state.bindingCount = count;
}

void XglPipelineObj::AddVertexDataBuffer(XglConstantBufferObj* vertexDataBuffer, int binding)
{
    m_vertexBufferObjs.push_back(vertexDataBuffer);
    m_vertexBufferBindings.push_back(binding);
    m_vertexBufferCount++;
}

void XglPipelineObj::AddColorAttachment(XGL_UINT binding, const XGL_PIPELINE_CB_ATTACHMENT_STATE *att)
{
    if (binding+1 > m_colorAttachments.size())
    {
        m_colorAttachments.resize(binding+1);
    }
    m_colorAttachments[binding] = *att;
}

void XglPipelineObj::SetDepthStencil(XGL_PIPELINE_DS_STATE_CREATE_INFO *ds_state)
{
    m_ds_state.format = ds_state->format;
    m_ds_state.depthTestEnable = ds_state->depthTestEnable;
    m_ds_state.depthWriteEnable = ds_state->depthWriteEnable;
    m_ds_state.depthBoundsEnable = ds_state->depthBoundsEnable;
    m_ds_state.depthFunc = ds_state->depthFunc;
    m_ds_state.stencilTestEnable = ds_state->stencilTestEnable;
    m_ds_state.back = ds_state->back;
    m_ds_state.front = ds_state->front;
}

void XglPipelineObj::CreateXGLPipeline(XglDescriptorSetObj *descriptorSet)
{
    XGL_VOID* head_ptr = &m_ds_state;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};

    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* shaderCreateInfo;

    for (int i=0; i<m_shaderObjs.size(); i++)
    {
        shaderCreateInfo = m_shaderObjs[i]->GetStageCreateInfo(descriptorSet);
        shaderCreateInfo->pNext = head_ptr;
        head_ptr = shaderCreateInfo;
    }

    if (m_vi_state.attributeCount && m_vi_state.bindingCount)
    {
        m_vi_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
        m_vi_state.pNext = head_ptr;
        head_ptr = &m_vi_state;
    }

    info.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = head_ptr;
    info.flags = 0;

    m_cb_state.attachmentCount = m_colorAttachments.size();
    m_cb_state.pAttachments = &m_colorAttachments[0];

    init(*m_device, info);
}

XGL_PIPELINE XglPipelineObj::GetPipelineHandle()
{
    return obj();
}

void XglPipelineObj::BindPipelineCommandBuffer(XGL_CMD_BUFFER m_cmdBuffer, XglDescriptorSetObj *descriptorSet)
{
    XGL_VOID* head_ptr = &m_ds_state;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};

    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* shaderCreateInfo;

    for (int i=0; i<m_shaderObjs.size(); i++)
    {
        shaderCreateInfo = m_shaderObjs[i]->GetStageCreateInfo(descriptorSet);
        shaderCreateInfo->pNext = head_ptr;
        head_ptr = shaderCreateInfo;
    }

    if (m_vi_state.attributeCount && m_vi_state.bindingCount)
    {
        m_vi_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
        m_vi_state.pNext = head_ptr;
        head_ptr = &m_vi_state;
    }

    info.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = head_ptr;
    info.flags = 0;

    init(*m_device, info);

    xglCmdBindPipeline( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, obj() );


    for (int i=0; i < m_vertexBufferCount; i++)
    {
        m_vertexBufferObjs[i]->Bind(m_cmdBuffer, 0,  m_vertexBufferBindings[i]);
    }
}

XglMemoryRefManager::XglMemoryRefManager() {

}

void XglMemoryRefManager::AddMemoryRef(XglConstantBufferObj *constantBuffer) {
    const std::vector<XGL_GPU_MEMORY> mems = constantBuffer->memories();
    if (!mems.empty())
        m_bufferObjs.push_back(mems[0]);
}

void XglMemoryRefManager::AddMemoryRef(XglTextureObj *texture) {
    const std::vector<XGL_GPU_MEMORY> mems = texture->memories();
    if (!mems.empty())
        m_bufferObjs.push_back(mems[0]);
}

XGL_MEMORY_REF* XglMemoryRefManager::GetMemoryRefList() {

    XGL_MEMORY_REF *localRefs;
    XGL_UINT32     numRefs=m_bufferObjs.size();

    if (numRefs <= 0)
        return NULL;

    localRefs = (XGL_MEMORY_REF*) malloc( numRefs * sizeof(XGL_MEMORY_REF) );
    for (int i=0; i<numRefs; i++)
    {
        localRefs[i].flags = 0;
        localRefs[i].mem = m_bufferObjs[i];
    }
    return localRefs;
}
int XglMemoryRefManager::GetNumRefs() {
    return m_bufferObjs.size();
}

XglCommandBufferObj::XglCommandBufferObj(XglDevice *device)
    : xgl_testing::CmdBuffer(*device, xgl_testing::CmdBuffer::create_info(XGL_QUEUE_TYPE_GRAPHICS))
{
    m_device = device;
    m_renderTargetCount = 0;
}

XGL_CMD_BUFFER XglCommandBufferObj::GetBufferHandle()
{
    return obj();
}

XGL_RESULT XglCommandBufferObj::BeginCommandBuffer(XGL_CMD_BUFFER_BEGIN_INFO *pInfo)
{
    begin(pInfo);
    return XGL_SUCCESS;
}

XGL_RESULT XglCommandBufferObj::BeginCommandBuffer(XGL_RENDER_PASS renderpass_obj)
{
    begin(renderpass_obj);
    return XGL_SUCCESS;
}

XGL_RESULT XglCommandBufferObj::BeginCommandBuffer()
{
    begin();
    return XGL_SUCCESS;
}

XGL_RESULT XglCommandBufferObj::EndCommandBuffer()
{
    end();
    return XGL_SUCCESS;
}

void XglCommandBufferObj::PipelineBarrier(XGL_PIPELINE_BARRIER *barrierPtr)
{
    xglCmdPipelineBarrier(obj(), barrierPtr);
}

void XglCommandBufferObj::ClearAllBuffers(XGL_DEPTH_STENCIL_BIND_INFO *depthStencilBinding, XGL_IMAGE depthStencilImage)
{
    XGL_UINT i;
    const XGL_FLAGS output_mask =
        XGL_MEMORY_OUTPUT_CPU_WRITE_BIT |
        XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT |
        XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
        XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
        XGL_MEMORY_OUTPUT_COPY_BIT;
    const XGL_FLAGS input_mask = 0;

    // whatever we want to do, we do it to the whole buffer
    XGL_IMAGE_SUBRESOURCE_RANGE srRange = {};
    srRange.aspect = XGL_IMAGE_ASPECT_COLOR;
    srRange.baseMipLevel = 0;
    srRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
    srRange.baseArraySlice = 0;
    srRange.arraySize = XGL_LAST_MIP_OR_SLICE;

    XGL_IMAGE_MEMORY_BARRIER memory_barrier = {};
    memory_barrier.sType = XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.outputMask = output_mask;
    memory_barrier.inputMask = input_mask;
    memory_barrier.newLayout = XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL;
    memory_barrier.subresourceRange = srRange;

    XGL_SET_EVENT set_events[] = { XGL_SET_EVENT_GPU_COMMANDS_COMPLETE };
    XGL_PIPELINE_BARRIER pipeline_barrier = {};
    pipeline_barrier.sType = XGL_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = set_events;
    pipeline_barrier.waitEvent = XGL_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.pMemBarriers = &memory_barrier;

    // clear the back buffer to dark grey
    XGL_UINT clearColor[4] = {64, 64, 64, 0};

    for (i = 0; i < m_renderTargetCount; i++) {
        memory_barrier.image = m_renderTargets[i]->image();
        memory_barrier.oldLayout = m_renderTargets[i]->layout();
        xglCmdPipelineBarrier( obj(), &pipeline_barrier);
        m_renderTargets[i]->layout(memory_barrier.newLayout);

        xglCmdClearColorImageRaw( obj(), m_renderTargets[i]->image(), clearColor, 1, &srRange );
    }

    if (depthStencilImage)
    {
        XGL_IMAGE_SUBRESOURCE_RANGE dsRange = {};
        dsRange.aspect = XGL_IMAGE_ASPECT_DEPTH;
        dsRange.baseMipLevel = 0;
        dsRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
        dsRange.baseArraySlice = 0;
        dsRange.arraySize = XGL_LAST_MIP_OR_SLICE;

        // prepare the depth buffer for clear

        memory_barrier.oldLayout = depthStencilBinding->layout;
        memory_barrier.newLayout = XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL;
        memory_barrier.image = depthStencilImage;
        memory_barrier.subresourceRange = dsRange;

        xglCmdPipelineBarrier( obj(), &pipeline_barrier);
        depthStencilBinding->layout = memory_barrier.newLayout;

        xglCmdClearDepthStencil(obj(), depthStencilImage, 1.0f, 0, 1, &dsRange);

        // prepare depth buffer for rendering
        memory_barrier.image = depthStencilImage;
        memory_barrier.oldLayout = XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL;
        memory_barrier.newLayout = depthStencilBinding->layout;
        memory_barrier.subresourceRange = dsRange;
        xglCmdPipelineBarrier( obj(), &pipeline_barrier);
        depthStencilBinding->layout = memory_barrier.newLayout;
    }
}

void XglCommandBufferObj::PrepareAttachments()
{
    XGL_UINT i;
    const XGL_FLAGS output_mask =
        XGL_MEMORY_OUTPUT_CPU_WRITE_BIT |
        XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT |
        XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
        XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
        XGL_MEMORY_OUTPUT_COPY_BIT;
    const XGL_FLAGS input_mask =
        XGL_MEMORY_INPUT_CPU_READ_BIT |
        XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
        XGL_MEMORY_INPUT_INDEX_FETCH_BIT |
        XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
        XGL_MEMORY_INPUT_UNIFORM_READ_BIT |
        XGL_MEMORY_INPUT_SHADER_READ_BIT |
        XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
        XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
        XGL_MEMORY_INPUT_COPY_BIT;

    XGL_IMAGE_SUBRESOURCE_RANGE srRange = {};
    srRange.aspect = XGL_IMAGE_ASPECT_COLOR;
    srRange.baseMipLevel = 0;
    srRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
    srRange.baseArraySlice = 0;
    srRange.arraySize = XGL_LAST_MIP_OR_SLICE;

    XGL_IMAGE_MEMORY_BARRIER memory_barrier = {};
    memory_barrier.sType = XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.outputMask = output_mask;
    memory_barrier.inputMask = input_mask;
    memory_barrier.newLayout = XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    memory_barrier.subresourceRange = srRange;

    XGL_SET_EVENT set_events[] = { XGL_SET_EVENT_GPU_COMMANDS_COMPLETE };
    XGL_PIPELINE_BARRIER pipeline_barrier = {};
    pipeline_barrier.sType = XGL_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = set_events;
    pipeline_barrier.waitEvent = XGL_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.pMemBarriers = &memory_barrier;

    for(i=0; i<m_renderTargetCount; i++)
    {
        memory_barrier.image = m_renderTargets[i]->image();
        memory_barrier.oldLayout = m_renderTargets[i]->layout();
        xglCmdPipelineBarrier( obj(), &pipeline_barrier);
        m_renderTargets[i]->layout(memory_barrier.newLayout);
    }
}

void XglCommandBufferObj::BindStateObject(XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT stateObject)
{
    xglCmdBindDynamicStateObject( obj(), stateBindPoint, stateObject);
}

void XglCommandBufferObj::AddRenderTarget(XglImage *renderTarget)
{
    m_renderTargets.push_back(renderTarget);
    m_renderTargetCount++;
}

void XglCommandBufferObj::DrawIndexed(XGL_UINT firstIndex, XGL_UINT indexCount, XGL_INT vertexOffset, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    xglCmdDrawIndexed(obj(), firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

void XglCommandBufferObj::Draw(XGL_UINT firstVertex, XGL_UINT vertexCount, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    xglCmdDraw(obj(), firstVertex, vertexCount, firstInstance, instanceCount);
}

void XglCommandBufferObj::QueueCommandBuffer(XGL_MEMORY_REF *memRefs, XGL_UINT32 numMemRefs)
{
    XGL_RESULT err = XGL_SUCCESS;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &obj(), numMemRefs, memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

}
void XglCommandBufferObj::BindPipeline(XGL_PIPELINE pipeline)
{
        xglCmdBindPipeline( obj(), XGL_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
}

void XglCommandBufferObj::BindDescriptorSet(XGL_DESCRIPTOR_SET descriptorSet)
{
    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic buffer view)
    xglCmdBindDescriptorSet(obj(), XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, descriptorSet, 0 );
}
void XglCommandBufferObj::BindIndexBuffer(XglIndexBufferObj *indexBuffer, XGL_UINT offset)
{
    xglCmdBindIndexBuffer(obj(), indexBuffer->obj(), offset, indexBuffer->GetIndexType());
}
void XglCommandBufferObj::BindVertexBuffer(XglConstantBufferObj *vertexBuffer, XGL_UINT offset, XGL_UINT binding)
{
    xglCmdBindVertexBuffer(obj(), vertexBuffer->obj(), offset, binding);
}
