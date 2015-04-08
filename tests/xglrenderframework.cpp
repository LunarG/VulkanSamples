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
    m_height( 256.0 ),                  // default window height
    m_render_target_fmt( XGL_FMT_R8G8B8A8_UNORM ),
    m_depth_stencil_fmt( XGL_FMT_UNDEFINED ),
    m_depth_clear_color( 1.0 ),
    m_stencil_clear_color( 0 )
{

    // clear the back buffer to dark grey
    m_clear_color.color.rawColor[0] = 64;
    m_clear_color.color.rawColor[1] = 64;
    m_clear_color.color.rawColor[2] = 64;
    m_clear_color.color.rawColor[3] = 0;
    m_clear_color.useRawValue = true;
}

XglRenderFramework::~XglRenderFramework()
{

}

void XglRenderFramework::InitFramework()
{
    XGL_RESULT err;

    err = xglCreateInstance(&app_info, NULL, &this->inst);
    ASSERT_XGL_SUCCESS(err);
    err = xglEnumerateGpus(inst, XGL_MAX_PHYSICAL_GPUS, &this->gpu_count,
                           objs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(this->gpu_count, 1) << "No GPU available";

    m_device = new XglDevice(0, objs[0]);
    m_device->get_device_queue();

    m_depthStencil = new XglDepthStencilObj();
}

void XglRenderFramework::ShutdownFramework()
{
    if (m_colorBlend) xglDestroyObject(m_colorBlend);
    if (m_stateDepthStencil) xglDestroyObject(m_stateDepthStencil);
    if (m_stateRaster) xglDestroyObject(m_stateRaster);
    if (m_cmdBuffer) xglDestroyObject(m_cmdBuffer);
    if (m_framebuffer) xglDestroyObject(m_framebuffer);
    if (m_renderPass) xglDestroyObject(m_renderPass);

    if (m_stateViewport) {
        xglDestroyObject(m_stateViewport);
    }
    while (!m_renderTargets.empty()) {
        xglDestroyObject(m_renderTargets.back()->targetView());
        xglBindObjectMemory(m_renderTargets.back()->image(), 0, XGL_NULL_HANDLE, 0);
        xglDestroyObject(m_renderTargets.back()->image());
        xglFreeMemory(m_renderTargets.back()->memory());
        m_renderTargets.pop_back();
    }

    delete m_depthStencil;

    // reset the driver
    delete m_device;
    xglDestroyInstance(this->inst);
}

void XglRenderFramework::InitState()
{
    XGL_RESULT err;

    m_render_target_fmt = XGL_FMT_B8G8R8A8_UNORM;

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
    cmdInfo.queueNodeIndex = m_device->graphics_queue_node_index_;

    err = xglCreateCommandBuffer(device(), &cmdInfo, &m_cmdBuffer);
    ASSERT_XGL_SUCCESS(err) << "xglCreateCommandBuffer failed";
}

void XglRenderFramework::InitViewport(float width, float height)
{
    XGL_RESULT err;

    XGL_VIEWPORT viewport;
    XGL_RECT scissor;

    XGL_DYNAMIC_VP_STATE_CREATE_INFO viewportCreate = {};
    viewportCreate.sType = XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO;
    viewportCreate.viewportAndScissorCount         = 1;
    viewport.originX  = 0;
    viewport.originY  = 0;
    viewport.width    = 1.f * width;
    viewport.height   = 1.f * height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewportCreate.pViewports = &viewport;
    viewportCreate.pScissors = &scissor;

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
    InitRenderTarget(1);
}

void XglRenderFramework::InitRenderTarget(uint32_t targets)
{
    InitRenderTarget(targets, NULL);
}

void XglRenderFramework::InitRenderTarget(XGL_DEPTH_STENCIL_BIND_INFO *dsBinding)
{
    InitRenderTarget(1, dsBinding);
}

void XglRenderFramework::InitRenderTarget(uint32_t targets, XGL_DEPTH_STENCIL_BIND_INFO *dsBinding)
{
    std::vector<XGL_ATTACHMENT_LOAD_OP> load_ops;
    std::vector<XGL_ATTACHMENT_STORE_OP> store_ops;
    std::vector<XGL_CLEAR_COLOR> clear_colors;

    uint32_t i;

    for (i = 0; i < targets; i++) {
        XglImage *img = new XglImage(m_device);
        img->init(m_width, m_height, m_render_target_fmt,
                XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
                XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        m_colorBindings[i].view  = img->targetView();
        m_colorBindings[i].layout = XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        m_renderTargets.push_back(img);
        load_ops.push_back(XGL_ATTACHMENT_LOAD_OP_LOAD);
        store_ops.push_back(XGL_ATTACHMENT_STORE_OP_STORE);
        clear_colors.push_back(m_clear_color);
//        m_mem_ref_mgr.AddMemoryRefs(*img);
    }
      // Create Framebuffer and RenderPass with color attachments and any depth/stencil attachment
    XGL_FRAMEBUFFER_CREATE_INFO fb_info = {};
    fb_info.sType = XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.colorAttachmentCount = m_renderTargets.size();
    fb_info.pColorAttachments = m_colorBindings;
    fb_info.pDepthStencilAttachment = dsBinding;
    fb_info.sampleCount = 1;
    fb_info.width = (uint32_t)m_width;
    fb_info.height = (uint32_t)m_height;
    fb_info.layers = 1;

    xglCreateFramebuffer(device(), &fb_info, &m_framebuffer);

    XGL_RENDER_PASS_CREATE_INFO rp_info = {};
    rp_info.sType = XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.renderArea.extent.width = m_width;
    rp_info.renderArea.extent.height = m_height;

    rp_info.colorAttachmentCount = m_renderTargets.size();
    rp_info.pColorFormats = &m_render_target_fmt;
    rp_info.pColorLayouts = &m_colorBindings[0].layout;
    rp_info.pColorLoadOps = &load_ops[0];
    rp_info.pColorStoreOps = &store_ops[0];
    rp_info.pColorLoadClearValues = &clear_colors[0];
    rp_info.depthStencilFormat = m_depth_stencil_fmt;
    if (dsBinding) {
        rp_info.depthStencilLayout = dsBinding->layout;
    }
    rp_info.depthLoadOp = XGL_ATTACHMENT_LOAD_OP_LOAD;
    rp_info.depthLoadClearValue = m_depth_clear_color;
    rp_info.depthStoreOp = XGL_ATTACHMENT_STORE_OP_STORE;
    rp_info.stencilLoadOp = XGL_ATTACHMENT_LOAD_OP_LOAD;
    rp_info.stencilLoadClearValue = m_stencil_clear_color;
    rp_info.stencilStoreOp = XGL_ATTACHMENT_STORE_OP_STORE;
    xglCreateRenderPass(device(), &rp_info, &m_renderPass);
}



XglDevice::XglDevice(uint32_t id, XGL_PHYSICAL_GPU obj) :
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

XglDescriptorSetObj::~XglDescriptorSetObj()
{
    delete m_set;
}

int XglDescriptorSetObj::AppendDummy()
{
    /* request a descriptor but do not update it */
    XGL_DESCRIPTOR_TYPE_COUNT tc = {};
    tc.type = XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER;
    tc.count = 1;
    m_type_counts.push_back(tc);

    return m_nextSlot++;
}

int XglDescriptorSetObj::AppendBuffer(XGL_DESCRIPTOR_TYPE type, XglConstantBufferObj &constantBuffer)
{
    XGL_DESCRIPTOR_TYPE_COUNT tc = {};
    tc.type = type;
    tc.count = 1;
    m_type_counts.push_back(tc);

    m_updateBuffers.push_back(xgl_testing::DescriptorSet::update(type,
                m_nextSlot, 0, 1, &constantBuffer.m_bufferViewInfo));

    // Track mem references for this descriptor set object
    mem_ref_mgr.AddMemoryRefs(constantBuffer);

    return m_nextSlot++;
}

int XglDescriptorSetObj::AppendSamplerTexture( XglSamplerObj* sampler, XglTextureObj* texture)
{
    XGL_DESCRIPTOR_TYPE_COUNT tc = {};
    tc.type = XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE;
    tc.count = 1;
    m_type_counts.push_back(tc);

    XGL_SAMPLER_IMAGE_VIEW_INFO tmp = {};
    tmp.sampler = sampler->obj();
    tmp.pImageView = &texture->m_textureViewInfo;
    m_samplerTextureInfo.push_back(tmp);

    m_updateSamplerTextures.push_back(xgl_testing::DescriptorSet::update(m_nextSlot, 0, 1,
                (const XGL_SAMPLER_IMAGE_VIEW_INFO *) NULL));

    // Track mem references for the texture referenced here
    mem_ref_mgr.AddMemoryRefs(*texture);

    return m_nextSlot++;
}

XGL_DESCRIPTOR_SET_LAYOUT_CHAIN XglDescriptorSetObj::GetLayoutChain() const
{
    return m_layout_chain.obj();
}

XGL_DESCRIPTOR_SET XglDescriptorSetObj::GetDescriptorSetHandle() const
{
    return m_set->obj();
}

void XglDescriptorSetObj::CreateXGLDescriptorSet(XglCommandBufferObj *cmdBuffer)
{
    // create XGL_DESCRIPTOR_POOL
    XGL_DESCRIPTOR_POOL_CREATE_INFO pool = {};
    pool.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.count = m_type_counts.size();
    pool.pTypeCount = &m_type_counts[0];
    init(*m_device, XGL_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, pool);

    // create XGL_DESCRIPTOR_SET_LAYOUT
    vector<XGL_DESCRIPTOR_SET_LAYOUT_BINDING> bindings;
    bindings.resize(m_type_counts.size());
    for (int i = 0; i < m_type_counts.size(); i++) {
        bindings[i].descriptorType = m_type_counts[i].type;
        bindings[i].count = m_type_counts[i].count;
        bindings[i].stageFlags = XGL_SHADER_STAGE_FLAGS_ALL;
        bindings[i].pImmutableSamplers = NULL;
    }

    // create XGL_DESCRIPTOR_SET_LAYOUT
    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO layout = {};
    layout.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout.count = bindings.size();
    layout.pBinding = &bindings[0];

    m_layout.init(*m_device, layout);

    vector<const xgl_testing::DescriptorSetLayout *> layouts;
    layouts.push_back(&m_layout);
    m_layout_chain.init(*m_device, layouts);

    // create XGL_DESCRIPTOR_SET
    m_set = alloc_sets(XGL_DESCRIPTOR_SET_USAGE_STATIC, m_layout);

    // build the update array
    vector<const void *> update_array;

    for (int i = 0; i < m_updateBuffers.size(); i++) {
        update_array.push_back(&m_updateBuffers[i]);
    }
    for (int i = 0; i < m_updateSamplerTextures.size(); i++) {
        m_updateSamplerTextures[i].pSamplerImageViews = &m_samplerTextureInfo[i];
        update_array.push_back(&m_updateSamplerTextures[i]);
    }

    // do the updates
    m_device->begin_descriptor_pool_update(XGL_DESCRIPTOR_UPDATE_MODE_FASTEST);
    clear_sets(*m_set);
    m_set->update(update_array);
    m_device->end_descriptor_pool_update(*cmdBuffer);
}

XglImage::XglImage(XglDevice *dev)
{
    m_device = dev;
    m_imageInfo.view = XGL_NULL_HANDLE;
    m_imageInfo.layout = XGL_IMAGE_LAYOUT_GENERAL;
}

void XglImage::ImageMemoryBarrier(
        XglCommandBufferObj *cmd_buf,
        XGL_IMAGE_ASPECT aspect,
        XGL_FLAGS output_mask /*=
            XGL_MEMORY_OUTPUT_CPU_WRITE_BIT |
            XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT |
            XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_COPY_BIT*/,
        XGL_FLAGS input_mask /*=
            XGL_MEMORY_INPUT_CPU_READ_BIT |
            XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
            XGL_MEMORY_INPUT_INDEX_FETCH_BIT |
            XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
            XGL_MEMORY_INPUT_UNIFORM_READ_BIT |
            XGL_MEMORY_INPUT_SHADER_READ_BIT |
            XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_COPY_BIT*/,
        XGL_IMAGE_LAYOUT image_layout)
{
    const XGL_IMAGE_SUBRESOURCE_RANGE subresourceRange = subresource_range(aspect, 0, 1, 0, 1);
    XGL_IMAGE_MEMORY_BARRIER barrier;
    barrier = image_memory_barrier(output_mask, input_mask, layout(), image_layout,
                                   subresourceRange);

    XGL_IMAGE_MEMORY_BARRIER *pmemory_barrier = &barrier;

    XGL_PIPE_EVENT pipe_events[] = { XGL_PIPE_EVENT_GPU_COMMANDS_COMPLETE };
    XGL_PIPELINE_BARRIER pipeline_barrier = {};
    pipeline_barrier.sType = XGL_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.pNext = NULL;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = pipe_events;
    pipeline_barrier.waitEvent = XGL_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.ppMemBarriers = (const void **)&pmemory_barrier;

    // write barrier to the command buffer
    xglCmdPipelineBarrier(cmd_buf->obj(), &pipeline_barrier);
}

void XglImage::SetLayout(XglCommandBufferObj *cmd_buf,
                         XGL_IMAGE_ASPECT aspect,
                         XGL_IMAGE_LAYOUT image_layout)
{
    XGL_FLAGS output_mask, input_mask;
    const XGL_FLAGS all_cache_outputs =
            XGL_MEMORY_OUTPUT_CPU_WRITE_BIT |
            XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT |
            XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_COPY_BIT;
    const XGL_FLAGS all_cache_inputs =
            XGL_MEMORY_INPUT_CPU_READ_BIT |
            XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
            XGL_MEMORY_INPUT_INDEX_FETCH_BIT |
            XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
            XGL_MEMORY_INPUT_UNIFORM_READ_BIT |
            XGL_MEMORY_INPUT_SHADER_READ_BIT |
            XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_COPY_BIT;

    if (image_layout == m_imageInfo.layout) {
        return;
    }

    switch (image_layout) {
    case XGL_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL:
        output_mask = XGL_MEMORY_OUTPUT_COPY_BIT;
        input_mask = XGL_MEMORY_INPUT_SHADER_READ_BIT | XGL_MEMORY_INPUT_COPY_BIT;
        break;

    case XGL_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL:
        output_mask = XGL_MEMORY_OUTPUT_COPY_BIT;
        input_mask = XGL_MEMORY_INPUT_SHADER_READ_BIT | XGL_MEMORY_INPUT_COPY_BIT;
        break;

    case XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL:
        break;

    case XGL_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        output_mask = XGL_MEMORY_OUTPUT_COPY_BIT;
        input_mask = XGL_MEMORY_INPUT_SHADER_READ_BIT | XGL_MEMORY_INPUT_COPY_BIT;
        break;

    default:
        output_mask =  all_cache_outputs;
        input_mask = all_cache_inputs;
        break;
    }

    ImageMemoryBarrier(cmd_buf, aspect, output_mask, input_mask, image_layout);
    m_imageInfo.layout = image_layout;
}

void XglImage::SetLayout(XGL_IMAGE_ASPECT aspect,
                         XGL_IMAGE_LAYOUT image_layout)
{
    XGL_RESULT err;
    XglCommandBufferObj cmd_buf(m_device);

    /* Build command buffer to set image layout in the driver */
    err = cmd_buf.BeginCommandBuffer();
    assert(!err);

    SetLayout(&cmd_buf, aspect, image_layout);

    err = cmd_buf.EndCommandBuffer();
    assert(!err);

    cmd_buf.QueueCommandBuffer();
}

bool XglImage::IsCompatible(XGL_FLAGS usage, XGL_FLAGS features)
{
    if ((usage & XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT) &&
            !(features & XGL_FORMAT_IMAGE_SHADER_READ_BIT))
        return false;

    if ((usage & XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT) &&
            !(features & XGL_FORMAT_IMAGE_SHADER_WRITE_BIT))
        return false;

    return true;
}

void XglImage::init(uint32_t w, uint32_t h,
               XGL_FORMAT fmt, XGL_FLAGS usage,
               XGL_IMAGE_TILING requested_tiling)
{
    uint32_t mipCount;
    XGL_FORMAT_PROPERTIES image_fmt;
    XGL_IMAGE_TILING tiling;
    XGL_RESULT err;
    size_t size;

    mipCount = 0;

    uint32_t _w = w;
    uint32_t _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

    size = sizeof(image_fmt);
    err = xglGetFormatInfo(m_device->obj(), fmt,
        XGL_INFO_TYPE_FORMAT_PROPERTIES,
        &size, &image_fmt);
    ASSERT_XGL_SUCCESS(err);

    if (requested_tiling == XGL_LINEAR_TILING) {
        if (IsCompatible(usage, image_fmt.linearTilingFeatures)) {
            tiling = XGL_LINEAR_TILING;
        } else if (IsCompatible(usage, image_fmt.optimalTilingFeatures)) {
            tiling = XGL_OPTIMAL_TILING;
        } else {
            ASSERT_TRUE(false) << "Error: Cannot find requested tiling configuration";
        }
    } else if (IsCompatible(usage, image_fmt.optimalTilingFeatures)) {
        tiling = XGL_OPTIMAL_TILING;
    } else if (IsCompatible(usage, image_fmt.linearTilingFeatures)) {
        tiling = XGL_LINEAR_TILING;
    } else {
         ASSERT_TRUE(false) << "Error: Cannot find requested tiling configuration";
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

    if (usage & XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT) {
        SetLayout(XGL_IMAGE_ASPECT_COLOR, XGL_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    } else {
        SetLayout(XGL_IMAGE_ASPECT_COLOR, XGL_IMAGE_LAYOUT_GENERAL);
    }
}

XGL_RESULT XglImage::MapMemory(void** ptr)
{
    *ptr = map();
    return (*ptr) ? XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

XGL_RESULT XglImage::UnmapMemory()
{
    unmap();
    return XGL_SUCCESS;
}

XGL_RESULT XglImage::CopyImage(XglImage &src_image)
{
    XGL_RESULT err;
    XglCommandBufferObj cmd_buf(m_device);
    XGL_IMAGE_LAYOUT src_image_layout, dest_image_layout;

    /* Build command buffer to copy staging texture to usable texture */
    err = cmd_buf.BeginCommandBuffer();
    assert(!err);

    /* TODO: Can we determine image aspect from image object? */
    src_image_layout = src_image.layout();
    src_image.SetLayout(&cmd_buf, XGL_IMAGE_ASPECT_COLOR, XGL_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL);

    dest_image_layout = this->layout();
    this->SetLayout(&cmd_buf, XGL_IMAGE_ASPECT_COLOR, XGL_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL);

    XGL_IMAGE_COPY copy_region = {};
    copy_region.srcSubresource.aspect = XGL_IMAGE_ASPECT_COLOR;
    copy_region.srcSubresource.arraySlice = 0;
    copy_region.srcSubresource.mipLevel = 0;
    copy_region.srcOffset.x = 0;
    copy_region.srcOffset.y = 0;
    copy_region.srcOffset.z = 0;
    copy_region.destSubresource.aspect = XGL_IMAGE_ASPECT_COLOR;
    copy_region.destSubresource.arraySlice = 0;
    copy_region.destSubresource.mipLevel = 0;
    copy_region.destOffset.x = 0;
    copy_region.destOffset.y = 0;
    copy_region.destOffset.z = 0;
    copy_region.extent = src_image.extent();

    xglCmdCopyImage(cmd_buf.obj(),
                    src_image.obj(), src_image.layout(),
                    obj(), layout(),
                    1, &copy_region);
    cmd_buf.mem_ref_mgr.AddMemoryRefs(src_image);
    cmd_buf.mem_ref_mgr.AddMemoryRefs(*this);

    src_image.SetLayout(&cmd_buf, XGL_IMAGE_ASPECT_COLOR, src_image_layout);

    this->SetLayout(&cmd_buf, XGL_IMAGE_ASPECT_COLOR, dest_image_layout);

    err = cmd_buf.EndCommandBuffer();
    assert(!err);

    cmd_buf.QueueCommandBuffer();

    return XGL_SUCCESS;
}

XglTextureObj::XglTextureObj(XglDevice *device, uint32_t *colors)
    :XglImage(device)
{
    m_device = device;
    const XGL_FORMAT tex_format = XGL_FMT_B8G8R8A8_UNORM;
    uint32_t tex_colors[2] = { 0xffff0000, 0xff00ff00 };
    void *data;
    int32_t x, y;
    XglImage stagingImage(device);

    stagingImage.init(16, 16, tex_format, 0, XGL_LINEAR_TILING);
    XGL_SUBRESOURCE_LAYOUT layout = stagingImage.subresource_layout(subresource(XGL_IMAGE_ASPECT_COLOR, 0, 0));

    if (colors == NULL)
        colors = tex_colors;

    memset(&m_textureViewInfo,0,sizeof(m_textureViewInfo));

    m_textureViewInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;

    XGL_IMAGE_VIEW_CREATE_INFO view = {};
    view.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.pNext = NULL;
    view.image = XGL_NULL_HANDLE;
    view.viewType = XGL_IMAGE_VIEW_2D;
    view.format = tex_format;
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
    init(16, 16, tex_format, XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT, XGL_OPTIMAL_TILING);

    /* create image view */
    view.image = obj();
    m_textureView.init(*m_device, view);
    m_textureViewInfo.view = m_textureView.obj();

    data = stagingImage.map();

    for (y = 0; y < extent().height; y++) {
        uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
        for (x = 0; x < extent().width; x++)
            row[x] = colors[(x & 1) ^ (y & 1)];
    }
    stagingImage.unmap();
    XglImage::CopyImage(stagingImage);
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

XglConstantBufferObj::~XglConstantBufferObj()
{
    // TODO: Should we call QueueRemoveMemReference for the constant buffer memory here?
    if (m_commandBuffer) {
        delete m_commandBuffer;
    }
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
    view_info.viewType = XGL_BUFFER_VIEW_RAW;
    view_info.offset = 0;
    view_info.range  = allocationSize;
    m_bufferView.init(*m_device, view_info);

    this->m_bufferViewInfo.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO;
    this->m_bufferViewInfo.view = m_bufferView.obj();
}

void XglConstantBufferObj::Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset, uint32_t binding)
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
        m_fence.init(*m_device, xgl_testing::Fence::create_info());

        m_commandBuffer = new XglCommandBufferObj(m_device);
    }
    else
    {
        m_device->wait(m_fence);
        m_commandBuffer->mem_ref_mgr.EmitRemoveMemoryRefs(m_device->m_queue);
    }

    // open the command buffer
    XGL_CMD_BUFFER_BEGIN_INFO cmd_buf_info = {};
    cmd_buf_info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;

    err = m_commandBuffer->BeginCommandBuffer(&cmd_buf_info);
    ASSERT_XGL_SUCCESS(err);

    XGL_BUFFER_MEMORY_BARRIER memory_barrier =
        buffer_memory_barrier(outputMask, inputMask, 0, m_numVertices * m_stride);
    XGL_BUFFER_MEMORY_BARRIER *pmemory_barrier = &memory_barrier;

    XGL_PIPE_EVENT set_events[] = { XGL_PIPE_EVENT_GPU_COMMANDS_COMPLETE };
    XGL_PIPELINE_BARRIER pipeline_barrier = {};
    pipeline_barrier.sType = XGL_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = set_events;
    pipeline_barrier.waitEvent = XGL_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.ppMemBarriers = (const void **)&pmemory_barrier;

    // write barrier to the command buffer
    m_commandBuffer->PipelineBarrier(&pipeline_barrier);

    // finish recording the command buffer
    err = m_commandBuffer->EndCommandBuffer();
    ASSERT_XGL_SUCCESS(err);

    /*
     * Tell driver about memory references made in this command buffer
     * Note: Since this command buffer only has a PipelineBarrier
     * command there really aren't any memory refs to worry about.
     */
    m_commandBuffer->mem_ref_mgr.EmitAddMemoryRefs(m_device->m_queue);

    // submit the command buffer to the universal queue
    XGL_CMD_BUFFER bufferArray[1];
    bufferArray[0] = m_commandBuffer->GetBufferHandle();
    err = xglQueueSubmit( m_device->m_queue, 1, bufferArray, m_fence.obj() );
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
    switch (indexType) {
    case XGL_INDEX_8:
        m_stride = 1;
        viewFormat = XGL_FMT_R8_UINT;
        break;
    case XGL_INDEX_16:
        m_stride = 2;
        viewFormat = XGL_FMT_R16_UINT;
        break;
    case XGL_INDEX_32:
        m_stride = 4;
        viewFormat = XGL_FMT_R32_UINT;
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
    view_info.format = viewFormat;
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

XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* XglShaderObj::GetStageCreateInfo()
{
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO *stageInfo = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) calloc( 1,sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO) );
    stageInfo->sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo->shader.stage = m_stage;
    stageInfo->shader.shader = obj();
    stageInfo->shader.linkConstBufferCount = 0;
    stageInfo->shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;

    return stageInfo;
}

XglShaderObj::XglShaderObj(XglDevice *device, const char * shader_code, XGL_PIPELINE_SHADER_STAGE stage, XglRenderFramework *framework)
{
    XGL_RESULT err = XGL_SUCCESS;
    std::vector<unsigned int> spv;
    XGL_SHADER_CREATE_INFO createInfo;
    size_t shader_len;

    m_stage = stage;
    m_device = device;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    if (!framework->m_use_spv) {

        shader_len = strlen(shader_code);
        createInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
        createInfo.pCode = malloc(createInfo.codeSize);
        createInfo.flags = 0;

        /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
        ((uint32_t *) createInfo.pCode)[0] = ICD_SPV_MAGIC;
        ((uint32_t *) createInfo.pCode)[1] = 0;
        ((uint32_t *) createInfo.pCode)[2] = stage;
        memcpy(((uint32_t *) createInfo.pCode + 3), shader_code, shader_len + 1);

        err = init_try(*m_device, createInfo);
    }

    if (framework->m_use_spv || err) {
        std::vector<unsigned int> spv;
        err = XGL_SUCCESS;

        // Use Reference GLSL to SPV compiler
        framework->GLSLtoSPV(stage, shader_code, spv);
        createInfo.pCode = spv.data();
        createInfo.codeSize = spv.size() * sizeof(unsigned int);
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
    m_rs_state.programPointSize = XGL_FALSE;
    m_rs_state.pointOrigin = XGL_COORDINATE_ORIGIN_UPPER_LEFT;
    m_rs_state.provokingVertex = XGL_PROVOKING_VERTEX_LAST;
    m_rs_state.fillMode = XGL_FILL_SOLID;
    m_rs_state.cullMode = XGL_CULL_NONE;
    m_rs_state.frontFace = XGL_FRONT_FACE_CCW;

    memset(&m_cb_state,0,sizeof(m_cb_state));
    m_cb_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    m_cb_state.pNext = &m_rs_state;
    m_cb_state.alphaToCoverageEnable = XGL_FALSE;
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
    m_ds_state.format = XGL_FMT_D32_SFLOAT;
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
    att.format = XGL_FMT_B8G8R8A8_UNORM;
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

void XglPipelineObj::AddColorAttachment(uint32_t binding, const XGL_PIPELINE_CB_ATTACHMENT_STATE *att)
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

void XglPipelineObj::CreateXGLPipeline(XglDescriptorSetObj &descriptorSet)
{
    void* head_ptr = &m_ds_state;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};

    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* shaderCreateInfo;

    for (int i=0; i<m_shaderObjs.size(); i++)
    {
        shaderCreateInfo = m_shaderObjs[i]->GetStageCreateInfo();
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
    info.pSetLayoutChain = descriptorSet.GetLayoutChain();

    m_cb_state.attachmentCount = m_colorAttachments.size();
    m_cb_state.pAttachments = &m_colorAttachments[0];

    init(*m_device, info);
}

vector<XGL_GPU_MEMORY> XglMemoryRefManager::mem_refs() const
{
    std::vector<XGL_GPU_MEMORY> mems;
    if (this->mem_refs_.size()) {
        mems.reserve(this->mem_refs_.size());
        for (uint32_t i = 0; i < this->mem_refs_.size(); i++)
            mems.push_back(this->mem_refs_[i]);
    }

    return mems;
}

void XglMemoryRefManager::AddMemoryRefs(xgl_testing::Object &xglObject)
{
    const std::vector<XGL_GPU_MEMORY> mems = xglObject.memories();
    AddMemoryRefs(mems);
}

void XglMemoryRefManager::AddMemoryRefs(vector<XGL_GPU_MEMORY> mem)
{
    for (size_t i = 0; i < mem.size(); i++) {
        if (mem[i] != NULL) {
            this->mem_refs_.push_back(mem[i]);
        }
    }
}

void XglMemoryRefManager::EmitAddMemoryRefs(XGL_QUEUE queue)
{
    for (uint32_t i = 0; i < mem_refs_.size(); i++) {
        xglQueueAddMemReference(queue, mem_refs_[i]);
    }
}

void XglMemoryRefManager::EmitRemoveMemoryRefs(XGL_QUEUE queue)
{
    for (uint32_t i = 0; i < mem_refs_.size(); i++) {
        xglQueueRemoveMemReference(queue, mem_refs_[i]);
    }
}

XglCommandBufferObj::XglCommandBufferObj(XglDevice *device)
    : xgl_testing::CmdBuffer(*device, xgl_testing::CmdBuffer::create_info(device->graphics_queue_node_index_))
{
    m_device = device;
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

XGL_RESULT XglCommandBufferObj::BeginCommandBuffer(XGL_RENDER_PASS renderpass_obj, XGL_FRAMEBUFFER framebuffer_obj)
{
    begin(renderpass_obj, framebuffer_obj);
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

void XglCommandBufferObj::ClearAllBuffers(XGL_CLEAR_COLOR clear_color, float depth_clear_color, uint32_t stencil_clear_color,
                                          XglDepthStencilObj *depthStencilObj)
{
    uint32_t i;
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
    XGL_IMAGE_MEMORY_BARRIER *pmemory_barrier = &memory_barrier;

    XGL_PIPE_EVENT set_events[] = { XGL_PIPE_EVENT_GPU_COMMANDS_COMPLETE };
    XGL_PIPELINE_BARRIER pipeline_barrier = {};
    pipeline_barrier.sType = XGL_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = set_events;
    pipeline_barrier.waitEvent = XGL_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.ppMemBarriers = (const void **)&pmemory_barrier;

    for (i = 0; i < m_renderTargets.size(); i++) {
        memory_barrier.image = m_renderTargets[i]->image();
        memory_barrier.oldLayout = m_renderTargets[i]->layout();
        xglCmdPipelineBarrier( obj(), &pipeline_barrier);
        m_renderTargets[i]->layout(memory_barrier.newLayout);

        xglCmdClearColorImage(obj(),
               m_renderTargets[i]->image(), XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL,
               clear_color, 1, &srRange );

        mem_ref_mgr.AddMemoryRefs(*m_renderTargets[i]);
    }

    if (depthStencilObj)
    {
        XGL_IMAGE_SUBRESOURCE_RANGE dsRange = {};
        dsRange.aspect = XGL_IMAGE_ASPECT_DEPTH;
        dsRange.baseMipLevel = 0;
        dsRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
        dsRange.baseArraySlice = 0;
        dsRange.arraySize = XGL_LAST_MIP_OR_SLICE;

        // prepare the depth buffer for clear

        memory_barrier.oldLayout = depthStencilObj->BindInfo()->layout;
        memory_barrier.newLayout = XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL;
        memory_barrier.image = depthStencilObj->obj();
        memory_barrier.subresourceRange = dsRange;

        xglCmdPipelineBarrier( obj(), &pipeline_barrier);

        xglCmdClearDepthStencil(obj(),
                                depthStencilObj->obj(), XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL,
                                depth_clear_color,  stencil_clear_color,
                                1, &dsRange);
        mem_ref_mgr.AddMemoryRefs(*depthStencilObj);

        // prepare depth buffer for rendering
        memory_barrier.image = depthStencilObj->obj();
        memory_barrier.oldLayout = XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL;
        memory_barrier.newLayout = depthStencilObj->BindInfo()->layout;
        memory_barrier.subresourceRange = dsRange;
        xglCmdPipelineBarrier( obj(), &pipeline_barrier);
    }
}

void XglCommandBufferObj::PrepareAttachments()
{
    uint32_t i;
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
    XGL_IMAGE_MEMORY_BARRIER *pmemory_barrier = &memory_barrier;

    XGL_PIPE_EVENT set_events[] = { XGL_PIPE_EVENT_GPU_COMMANDS_COMPLETE };
    XGL_PIPELINE_BARRIER pipeline_barrier = {};
    pipeline_barrier.sType = XGL_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = set_events;
    pipeline_barrier.waitEvent = XGL_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.ppMemBarriers = (const void **)&pmemory_barrier;

    for(i=0; i<m_renderTargets.size(); i++)
    {
        memory_barrier.image = m_renderTargets[i]->image();
        memory_barrier.oldLayout = m_renderTargets[i]->layout();
        xglCmdPipelineBarrier( obj(), &pipeline_barrier);
        m_renderTargets[i]->layout(memory_barrier.newLayout);
    }
}

void XglCommandBufferObj::BeginRenderPass(XGL_RENDER_PASS renderpass, XGL_FRAMEBUFFER framebuffer)
{
    XGL_RENDER_PASS_BEGIN rp_begin = {
        renderpass,
        framebuffer,
    };

    xglCmdBeginRenderPass( obj(), &rp_begin);
}

void XglCommandBufferObj::EndRenderPass(XGL_RENDER_PASS renderpass)
{
    xglCmdEndRenderPass( obj(), renderpass);
}

void XglCommandBufferObj::BindStateObject(XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT stateObject)
{
    xglCmdBindDynamicStateObject( obj(), stateBindPoint, stateObject);
}

void XglCommandBufferObj::AddRenderTarget(XglImage *renderTarget)
{
    m_renderTargets.push_back(renderTarget);
}

void XglCommandBufferObj::DrawIndexed(uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{
    xglCmdDrawIndexed(obj(), firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

void XglCommandBufferObj::Draw(uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    xglCmdDraw(obj(), firstVertex, vertexCount, firstInstance, instanceCount);
}

void XglCommandBufferObj::QueueCommandBuffer()
{
    QueueCommandBuffer(NULL);
}

void XglCommandBufferObj::QueueCommandBuffer(XGL_FENCE fence)
{
    XGL_RESULT err = XGL_SUCCESS;

    mem_ref_mgr.EmitAddMemoryRefs(m_device->m_queue);

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &obj(), fence );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

    /*
     * Now that processing on this command buffer is complete
     * we can remove the memory references.
     */
    mem_ref_mgr.EmitRemoveMemoryRefs(m_device->m_queue);
}

void XglCommandBufferObj::BindPipeline(XglPipelineObj &pipeline)
{
    xglCmdBindPipeline( obj(), XGL_PIPELINE_BIND_POINT_GRAPHICS, pipeline.obj() );
    mem_ref_mgr.AddMemoryRefs(pipeline);
}

void XglCommandBufferObj::BindDescriptorSet(XglDescriptorSetObj &descriptorSet)
{
    XGL_DESCRIPTOR_SET set_obj = descriptorSet.GetDescriptorSetHandle();

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic buffer view)
    xglCmdBindDescriptorSets(obj(), XGL_PIPELINE_BIND_POINT_GRAPHICS,
           descriptorSet.GetLayoutChain(), 0, 1, &set_obj, NULL );

    // Add descriptor set mem refs to command buffer's list
    mem_ref_mgr.AddMemoryRefs(descriptorSet.memories());
    mem_ref_mgr.AddMemoryRefs(descriptorSet.mem_ref_mgr.mem_refs());
}

void XglCommandBufferObj::BindIndexBuffer(XglIndexBufferObj *indexBuffer, uint32_t offset)
{
    xglCmdBindIndexBuffer(obj(), indexBuffer->obj(), offset, indexBuffer->GetIndexType());
    mem_ref_mgr.AddMemoryRefs(*indexBuffer);
}

void XglCommandBufferObj::BindVertexBuffer(XglConstantBufferObj *vertexBuffer, uint32_t offset, uint32_t binding)
{
    xglCmdBindVertexBuffer(obj(), vertexBuffer->obj(), offset, binding);
    mem_ref_mgr.AddMemoryRefs(*vertexBuffer);
}

XglDepthStencilObj::XglDepthStencilObj()
{
    m_initialized = false;
}
bool XglDepthStencilObj::Initialized()
{
    return m_initialized;
}

XGL_DEPTH_STENCIL_BIND_INFO* XglDepthStencilObj::BindInfo()
{
    return &m_depthStencilBindInfo;
}

void XglDepthStencilObj::Init(XglDevice *device, int32_t width, int32_t height)
{
    XGL_IMAGE_CREATE_INFO image_info;
    XGL_DEPTH_STENCIL_VIEW_CREATE_INFO view_info;

    m_device = device;
    m_initialized = true;
    m_depth_stencil_fmt = XGL_FMT_D16_UNORM;

    image_info.sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.imageType = XGL_IMAGE_2D;
    image_info.format = m_depth_stencil_fmt;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arraySize = 1;
    image_info.samples = 1;
    image_info.tiling = XGL_OPTIMAL_TILING;
    image_info.usage = XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT;
    image_info.flags = 0;
    init(*m_device, image_info);

    view_info.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.image = XGL_NULL_HANDLE;
    view_info.mipLevel = 0;
    view_info.baseArraySlice = 0;
    view_info.arraySize = 1;
    view_info.flags = 0;
    view_info.image = obj();
    m_depthStencilView.init(*m_device, view_info);

    m_depthStencilBindInfo.view = m_depthStencilView.obj();
    m_depthStencilBindInfo.layout = XGL_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}
