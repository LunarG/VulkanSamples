/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tony Barbour <tony@LunarG.com>
 *
 */

#include "vkrenderframework.h"
#include <vulkan/VK_KHR_surface.h>
#include <vulkan/VK_KHR_swapchain.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
    assert(fp##entrypoint != NULL);                                                                                        \
}

VkRenderFramework::VkRenderFramework() :
    m_commandPool(),
    m_commandBuffer(),
    m_renderPass(VK_NULL_HANDLE),
    m_framebuffer(VK_NULL_HANDLE),
    m_width( 256.0 ),                   // default window width
    m_height( 256.0 ),                  // default window height
    m_render_target_fmt( VK_FORMAT_R8G8B8A8_UNORM ),
    m_depth_stencil_fmt( VK_FORMAT_UNDEFINED ),
    m_clear_via_load_op( true ),
    m_depth_clear_color( 1.0 ),
    m_stencil_clear_color( 0 ),
    m_depthStencil( NULL ),
    m_dbgCreateMsgCallback( VK_NULL_HANDLE ),
    m_dbgDestroyMsgCallback( VK_NULL_HANDLE ),
    m_globalMsgCallback( VK_NULL_HANDLE ),
    m_devMsgCallback( VK_NULL_HANDLE )
{

    memset(&m_renderPassBeginInfo, 0, sizeof(m_renderPassBeginInfo));
    m_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

    // clear the back buffer to dark grey
    m_clear_color.float32[0] = 0.25f;
    m_clear_color.float32[1] = 0.25f;
    m_clear_color.float32[2] = 0.25f;
    m_clear_color.float32[3] = 0.0f;
}

VkRenderFramework::~VkRenderFramework()
{

}

void VkRenderFramework::InitFramework()
{
    std::vector<const char*> instance_layer_names;
    std::vector<const char*> device_layer_names;
    std::vector<const char*> instance_extension_names;
    std::vector<const char*> device_extension_names;
    InitFramework(
                instance_layer_names, device_layer_names,
                instance_extension_names, device_extension_names);
}

void VkRenderFramework::InitFramework(
        std::vector<const char *> instance_layer_names,
        std::vector<const char *> device_layer_names,
        std::vector<const char *> instance_extension_names,
        std::vector<const char *> device_extension_names,
        PFN_vkDbgMsgCallback dbgFunction,
        void *userData)
{
    VkInstanceCreateInfo instInfo = {};
    std::vector<VkExtensionProperties> instance_extensions;
    std::vector<VkExtensionProperties> device_extensions;
    VkResult U_ASSERT_ONLY err;

    /* TODO: Verify requested extensions are available */

    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = NULL;
    instInfo.pApplicationInfo = &app_info;
    instInfo.enabledLayerNameCount = instance_layer_names.size();
    instInfo.ppEnabledLayerNames = instance_layer_names.data();
    instInfo.enabledExtensionNameCount = instance_extension_names.size();
    instInfo.ppEnabledExtensionNames = instance_extension_names.data();
    err = vkCreateInstance(&instInfo, NULL, &this->inst);
    ASSERT_VK_SUCCESS(err);

    err = vkEnumeratePhysicalDevices(inst, &this->gpu_count, NULL);
    ASSERT_LE(this->gpu_count, ARRAY_SIZE(objs)) << "Too many gpus";
    ASSERT_VK_SUCCESS(err);
    err = vkEnumeratePhysicalDevices(inst, &this->gpu_count, objs);
    ASSERT_VK_SUCCESS(err);
    ASSERT_GE(this->gpu_count, (uint32_t) 1) << "No GPU available";
    if (dbgFunction) {
        m_dbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) vkGetInstanceProcAddr(this->inst, "vkDbgCreateMsgCallback");
        ASSERT_NE(m_dbgCreateMsgCallback, (PFN_vkDbgCreateMsgCallback) NULL) << "Did not get function pointer for DbgCreateMsgCallback";
        if (m_dbgCreateMsgCallback) {
            err = m_dbgCreateMsgCallback(this->inst,
                                         VK_DBG_REPORT_ERROR_BIT | VK_DBG_REPORT_WARN_BIT | VK_DBG_REPORT_PERF_WARN_BIT,
                                         dbgFunction,
                                         userData,
                                         &m_globalMsgCallback);
            ASSERT_VK_SUCCESS(err);

            m_dbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) vkGetInstanceProcAddr(this->inst, "vkDbgDestroyMsgCallback");
            ASSERT_NE(m_dbgDestroyMsgCallback, (PFN_vkDbgDestroyMsgCallback) NULL) << "Did not get function pointer for DbgDestroyMsgCallback";
        }
    }

    /* TODO: Verify requested physical device extensions are available */
    m_device = new VkDeviceObj(0, objs[0], device_layer_names, device_extension_names);

    /* Now register callback on device */
    if (0) {
        if (m_dbgCreateMsgCallback) {
            err = m_dbgCreateMsgCallback(this->inst,
                                         VK_DBG_REPORT_ERROR_BIT | VK_DBG_REPORT_WARN_BIT,
                                         dbgFunction,
                                         userData,
                                         &m_devMsgCallback);
            ASSERT_VK_SUCCESS(err);
        }
    }
    m_device->get_device_queue();

    m_depthStencil = new VkDepthStencilObj();
}

void VkRenderFramework::ShutdownFramework()
{
    if (m_commandBuffer)
        delete m_commandBuffer;
    if (m_commandPool) vkDestroyCommandPool(device(), m_commandPool, NULL);
    if (m_framebuffer) vkDestroyFramebuffer(device(), m_framebuffer, NULL);
    if (m_renderPass) vkDestroyRenderPass(device(), m_renderPass, NULL);

    if (m_globalMsgCallback) m_dbgDestroyMsgCallback(this->inst, m_globalMsgCallback);
    if (m_devMsgCallback) m_dbgDestroyMsgCallback(this->inst, m_devMsgCallback);

    while (!m_renderTargets.empty()) {
        vkDestroyImageView(device(), m_renderTargets.back()->targetView(m_render_target_fmt), NULL);
        vkDestroyImage(device(), m_renderTargets.back()->image(), NULL);
        vkFreeMemory(device(), m_renderTargets.back()->memory(), NULL);
        m_renderTargets.pop_back();
    }

    delete m_depthStencil;

    // reset the driver
    delete m_device;
    if (this->inst) vkDestroyInstance(this->inst, NULL);
}

void VkRenderFramework::InitState()
{
    VkResult err;

    // Get the list of VkFormat's that are supported:
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    uint32_t formatCount;
    VkSurfaceDescriptionKHR surface_description;
    surface_description.sType = VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_KHR;
    surface_description.pNext = NULL;
    GET_DEVICE_PROC_ADDR(device(), GetPhysicalDeviceSurfaceFormatsKHR);
    err = fpGetPhysicalDeviceSurfaceFormatsKHR(device(),
                                    (VkSurfaceDescriptionKHR *) &surface_description,
                                    &formatCount, NULL);
    ASSERT_VK_SUCCESS(err);
    VkSurfaceFormatKHR *surfFormats =
        (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    err = fpGetPhysicalDeviceSurfaceFormatsKHR(device(),
                                    (VkSurfaceDescriptionKHR *) &surface_description,
                                    &formatCount, surfFormats);
    ASSERT_VK_SUCCESS(err);
    m_render_target_fmt = surfFormats[0].format;
    free(surfFormats);

    m_lineWidth = 1.0f;

    m_depthBiasConstantFactor = 0.0f;
    m_depthBiasClamp = 0.0f;
    m_depthBiasSlopeFactor = 0.0f;

    m_blendConstants[0] = 1.0f;
    m_blendConstants[1] = 1.0f;
    m_blendConstants[2] = 1.0f;
    m_blendConstants[3] = 1.0f;

    m_minDepthBounds = 0.f;
    m_maxDepthBounds = 1.f;

    m_compareMask = 0xff;
    m_writeMask = 0xff;
    m_reference = 0;

    VkCommandPoolCreateInfo cmd_pool_info;
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    cmd_pool_info.pNext = NULL,
    cmd_pool_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    cmd_pool_info.flags = 0,
    err = vkCreateCommandPool(device(), &cmd_pool_info, NULL, &m_commandPool);
    assert(!err);

    m_commandBuffer = new VkCommandBufferObj(m_device, m_commandPool);
}

void VkRenderFramework::InitViewport(float width, float height)
{
    VkViewport viewport;
    VkRect2D scissor;
    viewport.x  = 0;
    viewport.y  = 0;
    viewport.width    = 1.f * width;
    viewport.height   = 1.f * height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    m_viewports.push_back(viewport);

    scissor.extent.width = (int32_t) width;
    scissor.extent.height = (int32_t) height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    m_scissors.push_back(scissor);

    m_width = width;
    m_height = height;
}

void VkRenderFramework::InitViewport()
{
    InitViewport(m_width, m_height);
}
void VkRenderFramework::InitRenderTarget()
{
    InitRenderTarget(1);
}

void VkRenderFramework::InitRenderTarget(uint32_t targets)
{
    InitRenderTarget(targets, NULL);
}

void VkRenderFramework::InitRenderTarget(VkImageView *dsBinding)
{
    InitRenderTarget(1, dsBinding);
}

void VkRenderFramework::InitRenderTarget(uint32_t targets, VkImageView *dsBinding)
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> color_references;
    std::vector<VkImageView> bindings;
    attachments.reserve(targets + 1); // +1 for dsBinding
    color_references.reserve(targets);
    bindings.reserve(targets + 1);     // +1 for dsBinding

    VkAttachmentDescription att = {};
    att.format = m_render_target_fmt;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = (m_clear_via_load_op) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ref = {};
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    m_renderPassClearValues.clear();
    VkClearValue clear = {};
    clear.color = m_clear_color;

    VkImageView bind = {};

    for (uint32_t i = 0; i < targets; i++) {
        attachments.push_back(att);

        ref.attachment = i;
        color_references.push_back(ref);

        m_renderPassClearValues.push_back(clear);

        VkImageObj *img = new VkImageObj(m_device);

        VkFormatProperties props;

        vkGetPhysicalDeviceFormatProperties(m_device->phy().handle(), m_render_target_fmt, &props);

        if (props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
            img->init((uint32_t)m_width, (uint32_t)m_height, m_render_target_fmt,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_IMAGE_TILING_LINEAR);
        }
        else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
            img->init((uint32_t)m_width, (uint32_t)m_height, m_render_target_fmt,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_IMAGE_TILING_OPTIMAL);
        }
        else {
            FAIL() << "Neither Linear nor Optimal allowed for render target";
        }

        m_renderTargets.push_back(img);
        bind = img->targetView(m_render_target_fmt);
        bindings.push_back(bind);
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = targets;
    subpass.pColorAttachments = color_references.data();
    subpass.pResolveAttachments = NULL;

    VkAttachmentReference ds_reference;
    if (dsBinding) {
        att.format = m_depth_stencil_fmt;
        att.loadOp = (m_clear_via_load_op) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;;
        att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        att.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(att);

        clear.depthStencil.depth = m_depth_clear_color;
        clear.depthStencil.stencil = m_stencil_clear_color;
        m_renderPassClearValues.push_back(clear);

        bindings.push_back(*dsBinding);

        ds_reference.attachment = targets;
        ds_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &ds_reference;
    } else {
        subpass.pDepthStencilAttachment = NULL;
    }

    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = attachments.size();
    rp_info.pAttachments = attachments.data();
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;

    vkCreateRenderPass(device(), &rp_info, NULL, &m_renderPass);

    // Create Framebuffer and RenderPass with color attachments and any depth/stencil attachment
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = m_renderPass;
    fb_info.attachmentCount = bindings.size();
    fb_info.pAttachments = bindings.data();
    fb_info.width = (uint32_t)m_width;
    fb_info.height = (uint32_t)m_height;
    fb_info.layers = 1;

    vkCreateFramebuffer(device(), &fb_info, NULL, &m_framebuffer);

    m_renderPassBeginInfo.renderPass = m_renderPass;
    m_renderPassBeginInfo.framebuffer = m_framebuffer;
    m_renderPassBeginInfo.renderArea.extent.width = (int32_t) m_width;
    m_renderPassBeginInfo.renderArea.extent.height = (int32_t) m_height;
    m_renderPassBeginInfo.clearValueCount = m_renderPassClearValues.size();
    m_renderPassBeginInfo.pClearValues = m_renderPassClearValues.data();
}



VkDeviceObj::VkDeviceObj(uint32_t id, VkPhysicalDevice obj) :
    vk_testing::Device(obj), id(id)
{
    init();

    props = phy().properties();
    queue_props = phy().queue_properties().data();
}

VkDeviceObj::VkDeviceObj(uint32_t id,
        VkPhysicalDevice obj, std::vector<const char *> &layer_names,
        std::vector<const char *> &extension_names) :
    vk_testing::Device(obj), id(id)
{
    init(layer_names, extension_names);

    props = phy().properties();
    queue_props = phy().queue_properties().data();
}

void VkDeviceObj::get_device_queue()
{
    ASSERT_NE(true, graphics_queues().empty());
    m_queue = graphics_queues()[0]->handle();
}

VkDescriptorSetObj::VkDescriptorSetObj(VkDeviceObj *device) :
        m_device(device), m_nextSlot(0)
{

}

VkDescriptorSetObj::~VkDescriptorSetObj()
{
    delete m_set;
}

int VkDescriptorSetObj::AppendDummy()
{
    /* request a descriptor but do not update it */
    VkDescriptorPoolSize tc = {};
    tc.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tc.descriptorCount = 1;
    m_type_counts.push_back(tc);

    return m_nextSlot++;
}

int VkDescriptorSetObj::AppendBuffer(VkDescriptorType type, VkConstantBufferObj &constantBuffer)
{
    assert(type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
           type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
    VkDescriptorPoolSize tc = {};
    tc.type = type;
    tc.descriptorCount = 1;
    m_type_counts.push_back(tc);

    m_writes.push_back(vk_testing::Device::write_descriptor_set(vk_testing::DescriptorSet(),
                m_nextSlot, 0, type, 1, &constantBuffer.m_descriptorBufferInfo));

    return m_nextSlot++;
}

int VkDescriptorSetObj::AppendSamplerTexture( VkSamplerObj* sampler, VkTextureObj* texture)
{
    VkDescriptorPoolSize tc = {};
    tc.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tc.descriptorCount = 1;
    m_type_counts.push_back(tc);

    VkDescriptorImageInfo tmp = texture->m_imageInfo;
    tmp.sampler = sampler->handle();
    m_imageSamplerDescriptors.push_back(tmp);

    m_writes.push_back(vk_testing::Device::write_descriptor_set(vk_testing::DescriptorSet(),
                m_nextSlot, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &tmp));

    return m_nextSlot++;
}

VkPipelineLayout VkDescriptorSetObj::GetPipelineLayout() const
{
    return m_pipeline_layout.handle();
}

VkDescriptorSet VkDescriptorSetObj::GetDescriptorSetHandle() const
{
    return m_set->handle();
}

void VkDescriptorSetObj::CreateVKDescriptorSet(VkCommandBufferObj *commandBuffer)
{
    // create VkDescriptorPool
    VkDescriptorPoolCreateInfo pool = {};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.poolSizeCount = m_type_counts.size();
    pool.maxSets = 1;
    pool.pPoolSizes = m_type_counts.data();
    init(*m_device, pool);

    // create VkDescriptorSetLayout
    vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.resize(m_type_counts.size());
    for (int i = 0; i < m_type_counts.size(); i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = m_type_counts[i].type;
        bindings[i].descriptorCount = m_type_counts[i].descriptorCount;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        bindings[i].pImmutableSamplers = NULL;
    }

    // create VkDescriptorSetLayout
    VkDescriptorSetLayoutCreateInfo layout = {};
    layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout.bindingCount = bindings.size();
    layout.pBinding = bindings.data();

    m_layout.init(*m_device, layout);
    vector<const vk_testing::DescriptorSetLayout *> layouts;
    layouts.push_back(&m_layout);

    // create VkPipelineLayout
    VkPipelineLayoutCreateInfo pipeline_layout = {};
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = layouts.size();
    pipeline_layout.pSetLayouts = NULL;

    m_pipeline_layout.init(*m_device, pipeline_layout, layouts);

    // create VkDescriptorSet
    m_set = alloc_sets(*m_device, m_layout);

    // build the update array
    size_t imageSamplerCount = 0;
    for (std::vector<VkWriteDescriptorSet>::iterator it = m_writes.begin();
         it != m_writes.end(); it++) {
        it->dstSet = m_set->handle();
        if (it->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            it->pImageInfo = &m_imageSamplerDescriptors[imageSamplerCount++];
    }

    // do the updates
    m_device->update_descriptor_sets(m_writes);
}

VkImageObj::VkImageObj(VkDeviceObj *dev)
{
    m_device = dev;
    m_descriptorImageInfo.imageView = VK_NULL_HANDLE;
    m_descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
}

void VkImageObj::ImageMemoryBarrier(
        VkCommandBufferObj *cmd_buf,
        VkImageAspectFlags aspect,
        VkFlags output_mask /*=
            VK_ACCESS_HOST_WRITE_BIT |
            VK_ACCESS_SHADER_WRITE_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
            VK_MEMORY_OUTPUT_COPY_BIT*/,
        VkFlags input_mask /*=
            VK_ACCESS_HOST_READ_BIT |
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT |
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_UNIFORM_READ_BIT |
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_MEMORY_INPUT_COPY_BIT*/,
        VkImageLayout image_layout)
{
    const VkImageSubresourceRange subresourceRange = subresource_range(aspect, 0, 1, 0, 1);
    VkImageMemoryBarrier barrier;
    barrier = image_memory_barrier(output_mask, input_mask, layout(), image_layout,
                                   subresourceRange);

    VkImageMemoryBarrier *pmemory_barrier = &barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    // write barrier to the command buffer
    vkCmdPipelineBarrier(cmd_buf->handle(), src_stages, dest_stages, 0, 1, (const void * const*)&pmemory_barrier);
}

void VkImageObj::SetLayout(VkCommandBufferObj *cmd_buf,
                         VkImageAspectFlagBits aspect,
                         VkImageLayout image_layout)
{
    VkFlags src_mask, dst_mask;
    const VkFlags all_cache_outputs =
            VK_ACCESS_HOST_WRITE_BIT |
            VK_ACCESS_SHADER_WRITE_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_TRANSFER_WRITE_BIT;
    const VkFlags all_cache_inputs =
            VK_ACCESS_HOST_READ_BIT |
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT |
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_UNIFORM_READ_BIT |
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_MEMORY_READ_BIT;

    if (image_layout == m_descriptorImageInfo.imageLayout) {
        return;
    }

    switch (image_layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        if (m_descriptorImageInfo.imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            src_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        else
            src_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        if (m_descriptorImageInfo.imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            src_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        else
            src_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        src_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
        break;

    default:
        src_mask =  all_cache_outputs;
        dst_mask = all_cache_inputs;
        break;
    }

    ImageMemoryBarrier(cmd_buf, aspect, src_mask, dst_mask, image_layout);
    m_descriptorImageInfo.imageLayout = image_layout;
}

void VkImageObj::SetLayout(VkImageAspectFlagBits aspect,
                           VkImageLayout image_layout)
{
    VkResult U_ASSERT_ONLY err;

    if (image_layout == m_descriptorImageInfo.imageLayout) {
        return;
    }

    VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = NULL;
        cmd_pool_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
        cmd_pool_info.flags = 0;
    vk_testing::CommandPool pool(*m_device, cmd_pool_info);
    VkCommandBufferObj cmd_buf(m_device, pool.handle());

    /* Build command buffer to set image layout in the driver */
    err = cmd_buf.BeginCommandBuffer();
    assert(!err);

    SetLayout(&cmd_buf, aspect, image_layout);

    err = cmd_buf.EndCommandBuffer();
    assert(!err);

    cmd_buf.QueueCommandBuffer();
}

bool VkImageObj::IsCompatible(VkFlags usage, VkFlags features)
{
    if ((usage & VK_IMAGE_USAGE_SAMPLED_BIT) &&
            !(features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
        return false;

    return true;
}

void VkImageObj::init(uint32_t w, uint32_t h,
                      VkFormat fmt, VkFlags usage,
                      VkImageTiling requested_tiling,
                      VkMemoryPropertyFlags reqs)
{
    uint32_t mipCount;
    VkFormatProperties image_fmt;
    VkImageTiling tiling;

    mipCount = 0;

    uint32_t _w = w;
    uint32_t _h = h;
    while( ( _w > 0 ) || ( _h > 0 ) )
    {
        _w >>= 1;
        _h >>= 1;
        mipCount++;
    }

    vkGetPhysicalDeviceFormatProperties(m_device->phy().handle(), fmt, &image_fmt);

    if (requested_tiling == VK_IMAGE_TILING_LINEAR) {
        if (IsCompatible(usage, image_fmt.linearTilingFeatures)) {
            tiling = VK_IMAGE_TILING_LINEAR;
        } else if (IsCompatible(usage, image_fmt.optimalTilingFeatures)) {
            tiling = VK_IMAGE_TILING_OPTIMAL;
        } else {
            ASSERT_TRUE(false) << "Error: Cannot find requested tiling configuration";
        }
    } else if (IsCompatible(usage, image_fmt.optimalTilingFeatures)) {
        tiling = VK_IMAGE_TILING_OPTIMAL;
    } else if (IsCompatible(usage, image_fmt.linearTilingFeatures)) {
        tiling = VK_IMAGE_TILING_LINEAR;
    } else {
         ASSERT_TRUE(false) << "Error: Cannot find requested tiling configuration";
    }

    VkImageCreateInfo imageCreateInfo = vk_testing::Image::create_info();
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = fmt;
    imageCreateInfo.extent.width = w;
    imageCreateInfo.extent.height = h;
    imageCreateInfo.mipLevels = mipCount;
    imageCreateInfo.tiling = tiling;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    else if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    else
        imageCreateInfo.initialLayout = m_descriptorImageInfo.imageLayout;

    layout(imageCreateInfo.initialLayout);
    imageCreateInfo.usage = usage;

    vk_testing::Image::init(*m_device, imageCreateInfo, reqs);
}

VkResult VkImageObj::CopyImage(VkImageObj &src_image)
{
    VkResult U_ASSERT_ONLY err;
    VkImageLayout src_image_layout, dest_image_layout;

    VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = NULL;
        cmd_pool_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
        cmd_pool_info.flags = 0;
    vk_testing::CommandPool pool(*m_device, cmd_pool_info);
    VkCommandBufferObj cmd_buf(m_device, pool.handle());

    /* Build command buffer to copy staging texture to usable texture */
    err = cmd_buf.BeginCommandBuffer();
    assert(!err);

    /* TODO: Can we determine image aspect from image object? */
    src_image_layout = src_image.layout();
    src_image.SetLayout(&cmd_buf, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    dest_image_layout = this->layout();
    this->SetLayout(&cmd_buf, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageCopy copy_region = {};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.baseArrayLayer = 0;
    copy_region.srcSubresource.mipLevel = 0;
    copy_region.srcOffset.x = 0;
    copy_region.srcOffset.y = 0;
    copy_region.srcOffset.z = 0;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.baseArrayLayer = 0;
    copy_region.dstSubresource.mipLevel = 0;
    copy_region.dstOffset.x = 0;
    copy_region.dstOffset.y = 0;
    copy_region.dstOffset.z = 0;
    copy_region.extent = src_image.extent();

    vkCmdCopyImage(cmd_buf.handle(),
                    src_image.handle(), src_image.layout(),
                    handle(), layout(),
                    1, &copy_region);

    src_image.SetLayout(&cmd_buf, VK_IMAGE_ASPECT_COLOR_BIT, src_image_layout);

    this->SetLayout(&cmd_buf, VK_IMAGE_ASPECT_COLOR_BIT, dest_image_layout);

    err = cmd_buf.EndCommandBuffer();
    assert(!err);

    cmd_buf.QueueCommandBuffer();

    return VK_SUCCESS;
}

VkTextureObj::VkTextureObj(VkDeviceObj *device, uint32_t *colors)
    :VkImageObj(device)
{
    m_device = device;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    uint32_t tex_colors[2] = { 0xffff0000, 0xff00ff00 };
    void *data;
    int32_t x, y;
    VkImageObj stagingImage(device);
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    stagingImage.init(16, 16, tex_format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_LINEAR, reqs);
    VkSubresourceLayout layout = stagingImage.subresource_layout(subresource(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0));

    if (colors == NULL)
        colors = tex_colors;

    memset(&m_imageInfo,0,sizeof(m_imageInfo));

    VkImageViewCreateInfo view = {};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.pNext = NULL;
    view.image = VK_NULL_HANDLE;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = tex_format;
    view.components.r = VK_COMPONENT_SWIZZLE_R;
    view.components.g = VK_COMPONENT_SWIZZLE_G;
    view.components.b = VK_COMPONENT_SWIZZLE_B;
    view.components.a = VK_COMPONENT_SWIZZLE_A;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;

    /* create image */
    init(16, 16, tex_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_OPTIMAL);

    /* create image view */
    view.image = handle();
    m_textureView.init(*m_device, view);
    m_imageInfo.imageView = m_textureView.handle();

    data = stagingImage.MapMemory();

    for (y = 0; y < extent().height; y++) {
        uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
        for (x = 0; x < extent().width; x++)
            row[x] = colors[(x & 1) ^ (y & 1)];
    }
    stagingImage.UnmapMemory();
    VkImageObj::CopyImage(stagingImage);
}

VkSamplerObj::VkSamplerObj(VkDeviceObj *device)
{
    m_device = device;

    VkSamplerCreateInfo samplerCreateInfo;
    memset(&samplerCreateInfo,0,sizeof(samplerCreateInfo));
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_BASE;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.maxAnisotropy = 0;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod = 0.0;
    samplerCreateInfo.maxLod = 0.0;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    init(*m_device, samplerCreateInfo);
}

/*
 * Basic ConstantBuffer constructor. Then use create methods to fill in the details.
 */
VkConstantBufferObj::VkConstantBufferObj(VkDeviceObj *device)
{
    m_device = device;
    m_commandBuffer = 0;

    memset(&m_descriptorBufferInfo,0,sizeof(m_descriptorBufferInfo));
}

VkConstantBufferObj::~VkConstantBufferObj()
{
    // TODO: Should we call QueueRemoveMemReference for the constant buffer memory here?
    if (m_commandBuffer) {
        delete m_commandBuffer;
        delete m_commandPool;
    }
}

VkConstantBufferObj::VkConstantBufferObj(VkDeviceObj *device, int constantCount, int constantSize, const void* data)
{
    m_device = device;
    m_commandBuffer = 0;

    memset(&m_descriptorBufferInfo,0,sizeof(m_descriptorBufferInfo));
    m_numVertices = constantCount;
    m_stride = constantSize;

    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const size_t allocationSize = constantCount * constantSize;
    init_as_src_and_dst(*m_device, allocationSize, reqs);

    void *pData = memory().map();
    memcpy(pData, data, allocationSize);
    memory().unmap();

    /*
     * Constant buffers are going to be used as vertex input buffers
     * or as shader uniform buffers. So, we'll create the shaderbuffer
     * descriptor here so it's ready if needed.
     */
    this->m_descriptorBufferInfo.buffer = handle();
    this->m_descriptorBufferInfo.offset = 0;
    this->m_descriptorBufferInfo.range = allocationSize;
}

void VkConstantBufferObj::Bind(VkCommandBuffer commandBuffer, VkDeviceSize offset, uint32_t binding)
{
    vkCmdBindVertexBuffers(commandBuffer, binding, 1, &handle(), &offset);
}


void VkConstantBufferObj::BufferMemoryBarrier(
        VkFlags srcAccessMask /*=
            VK_ACCESS_HOST_WRITE_BIT |
            VK_ACCESS_SHADER_WRITE_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
            VK_MEMORY_OUTPUT_COPY_BIT*/,
        VkFlags dstAccessMask /*=
            VK_ACCESS_HOST_READ_BIT |
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT |
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_UNIFORM_READ_BIT |
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_MEMORY_INPUT_COPY_BIT*/)
{
    VkResult err = VK_SUCCESS;

    if (!m_commandBuffer)
    {
        m_fence.init(*m_device, vk_testing::Fence::create_info());
        VkCommandPoolCreateInfo cmd_pool_info = {};
            cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmd_pool_info.pNext = NULL;
            cmd_pool_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
            cmd_pool_info.flags = 0;
        m_commandPool = new vk_testing::CommandPool(*m_device, cmd_pool_info);
        m_commandBuffer = new VkCommandBufferObj(m_device, m_commandPool->handle());
    }
    else
    {
        m_device->wait(m_fence);
    }

    // open the command buffer
    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;
    cmd_buf_info.renderPass = VK_NULL_HANDLE;
    cmd_buf_info.subpass = 0;
    cmd_buf_info.framebuffer = VK_NULL_HANDLE;
    cmd_buf_info.occlusionQueryEnable = VK_FALSE;
    cmd_buf_info.queryFlags = 0;
    cmd_buf_info.pipelineStatistics = 0;

    err = m_commandBuffer->BeginCommandBuffer(&cmd_buf_info);
    ASSERT_VK_SUCCESS(err);

    VkBufferMemoryBarrier memory_barrier =
        buffer_memory_barrier(srcAccessMask, dstAccessMask, 0, m_numVertices * m_stride);
    VkBufferMemoryBarrier *pmemory_barrier = &memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    // write barrier to the command buffer
    m_commandBuffer->PipelineBarrier(src_stages, dest_stages, 0, 1, (const void **)&pmemory_barrier);

    // finish recording the command buffer
    err = m_commandBuffer->EndCommandBuffer();
    ASSERT_VK_SUCCESS(err);

    // submit the command buffer to the universal queue
    VkCommandBuffer bufferArray[1];
    bufferArray[0] = m_commandBuffer->GetBufferHandle();
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = bufferArray;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit(m_device->m_queue, 1, &submit_info, m_fence.handle());
    ASSERT_VK_SUCCESS(err);
}

VkIndexBufferObj::VkIndexBufferObj(VkDeviceObj *device)
    : VkConstantBufferObj(device)
{

}

void VkIndexBufferObj::CreateAndInitBuffer(int numIndexes, VkIndexType indexType, const void* data)
{
    m_numVertices = numIndexes;
    m_indexType = indexType;
    switch (indexType) {
    case VK_INDEX_TYPE_UINT16:
        m_stride = 2;
        break;
    case VK_INDEX_TYPE_UINT32:
        m_stride = 4;
        break;
    default:
        assert(!"unknown index type");
        m_stride = 2;
        break;
    }

    const size_t allocationSize = numIndexes * m_stride;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    init_as_src_and_dst(*m_device, allocationSize, reqs);

    void *pData = memory().map();
    memcpy(pData, data, allocationSize);
    memory().unmap();

    // set up the descriptor for the constant buffer
    this->m_descriptorBufferInfo.buffer = handle();
    this->m_descriptorBufferInfo.offset = 0;
    this->m_descriptorBufferInfo.range = allocationSize;
}

void VkIndexBufferObj::Bind(VkCommandBuffer commandBuffer, VkDeviceSize offset)
{
    vkCmdBindIndexBuffer(commandBuffer, handle(), offset, m_indexType);
}

VkIndexType VkIndexBufferObj::GetIndexType()
{
    return m_indexType;
}

VkPipelineShaderStageCreateInfo VkShaderObj::GetStageCreateInfo() const
{
    VkPipelineShaderStageCreateInfo stageInfo = {};

    stageInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage                = m_stage;
    stageInfo.module               = handle();
    stageInfo.pName                = "main";

    return stageInfo;
}

VkShaderObj::VkShaderObj(VkDeviceObj *device, const char * shader_code, VkShaderStageFlagBits stage, VkRenderFramework *framework)
{
    VkResult U_ASSERT_ONLY err = VK_SUCCESS;
    std::vector<unsigned int> spv;
    VkShaderModuleCreateInfo moduleCreateInfo;
    size_t shader_len;

    m_stage = stage;
    m_device = device;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;

    if (framework->m_use_glsl) {

        shader_len = strlen(shader_code);
        moduleCreateInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
        moduleCreateInfo.pCode = (uint32_t *) malloc(moduleCreateInfo.codeSize);
        moduleCreateInfo.flags = 0;

        /* try version 0 first: VkShaderStage followed by GLSL */
        ((uint32_t *) moduleCreateInfo.pCode)[0] = ICD_SPV_MAGIC;
        ((uint32_t *) moduleCreateInfo.pCode)[1] = 0;
        ((uint32_t *) moduleCreateInfo.pCode)[2] = stage;
        memcpy(((uint32_t *) moduleCreateInfo.pCode + 3), shader_code, shader_len + 1);

    } else {

        // Use Reference GLSL to SPV compiler
        framework->GLSLtoSPV(stage, shader_code, spv);
        moduleCreateInfo.pCode = spv.data();
        moduleCreateInfo.codeSize = spv.size() * sizeof(unsigned int);
        moduleCreateInfo.flags = 0;
    }

    err = init_try(*m_device, moduleCreateInfo);
    assert(VK_SUCCESS == err);
}

VkPipelineObj::VkPipelineObj(VkDeviceObj *device)
{
    m_device = device;

    m_vi_state.pNext                        = VK_NULL_HANDLE;
    m_vi_state.vertexBindingDescriptionCount = 0;
    m_vi_state.pVertexBindingDescriptions   = VK_NULL_HANDLE;
    m_vi_state.vertexAttributeDescriptionCount = 0;
    m_vi_state.pVertexAttributeDescriptions = VK_NULL_HANDLE;

    m_vertexBufferCount = 0;

    m_ia_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_ia_state.pNext = VK_NULL_HANDLE;
    m_ia_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_ia_state.primitiveRestartEnable = VK_FALSE;

    m_rs_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rs_state.pNext = VK_NULL_HANDLE;
    m_rs_state.depthClampEnable = VK_TRUE;
    m_rs_state.rasterizerDiscardEnable = VK_FALSE;
    m_rs_state.polygonMode = VK_POLYGON_MODE_FILL;
    m_rs_state.cullMode = VK_CULL_MODE_BACK_BIT;
    m_rs_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    m_rs_state.depthBiasEnable = VK_FALSE;
    m_rs_state.lineWidth = 1.0f;
    m_rs_state.depthBiasConstantFactor = 0.0f;
    m_rs_state.depthBiasClamp = 0.0f;
    m_rs_state.depthBiasSlopeFactor = 0.0f;

    memset(&m_cb_state,0,sizeof(m_cb_state));
    m_cb_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_cb_state.pNext = VK_NULL_HANDLE;
    m_cb_state.logicOp = VK_LOGIC_OP_COPY;
    m_cb_state.blendConstants[0] = 1.0f;
    m_cb_state.blendConstants[1] = 1.0f;
    m_cb_state.blendConstants[2] = 1.0f;
    m_cb_state.blendConstants[3] = 1.0f;

    m_ms_state.pNext = VK_NULL_HANDLE;
    m_ms_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_ms_state.pSampleMask = NULL;
    m_ms_state.alphaToCoverageEnable = VK_FALSE;
    m_ms_state.alphaToOneEnable = VK_FALSE;
    m_ms_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_ms_state.minSampleShading = 0;
    m_ms_state.sampleShadingEnable = 0;

    m_vp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_vp_state.pNext = VK_NULL_HANDLE;
    m_vp_state.viewportCount = 1;
    m_vp_state.scissorCount = 1;
    m_vp_state.pViewports = NULL;
    m_vp_state.pScissors = NULL;

    m_ds_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_ds_state.pNext = VK_NULL_HANDLE,
    m_ds_state.depthTestEnable      = VK_FALSE;
    m_ds_state.depthWriteEnable     = VK_FALSE;
    m_ds_state.depthBoundsTestEnable = VK_FALSE;
    m_ds_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    m_ds_state.back.depthFailOp = VK_STENCIL_OP_KEEP;
    m_ds_state.back.failOp = VK_STENCIL_OP_KEEP;
    m_ds_state.back.passOp = VK_STENCIL_OP_KEEP;
    m_ds_state.back.compareOp = VK_COMPARE_OP_ALWAYS;
    m_ds_state.stencilTestEnable = VK_FALSE;
    m_ds_state.back.compareMask = 0xff;
    m_ds_state.back.writeMask = 0xff;
    m_ds_state.back.reference = 0;
    m_ds_state.minDepthBounds = 0.f;
    m_ds_state.maxDepthBounds = 1.f;

    m_ds_state.front = m_ds_state.back;
};

void VkPipelineObj::AddShader(VkShaderObj* shader)
{
    m_shaderObjs.push_back(shader);
}

void VkPipelineObj::AddVertexInputAttribs(VkVertexInputAttributeDescription* vi_attrib, int count)
{
    m_vi_state.pVertexAttributeDescriptions = vi_attrib;
    m_vi_state.vertexAttributeDescriptionCount = count;
}

void VkPipelineObj::AddVertexInputBindings(VkVertexInputBindingDescription* vi_binding, int count)
{
    m_vi_state.pVertexBindingDescriptions = vi_binding;
    m_vi_state.vertexBindingDescriptionCount = count;
}

void VkPipelineObj::AddColorAttachment(uint32_t binding, const VkPipelineColorBlendAttachmentState *att)
{
    if (binding+1 > m_colorAttachments.size())
    {
        m_colorAttachments.resize(binding+1);
    }
    m_colorAttachments[binding] = *att;
}

void VkPipelineObj::SetDepthStencil(VkPipelineDepthStencilStateCreateInfo *ds_state)
{
    m_ds_state.depthTestEnable = ds_state->depthTestEnable;
    m_ds_state.depthWriteEnable = ds_state->depthWriteEnable;
    m_ds_state.depthBoundsTestEnable = ds_state->depthBoundsTestEnable;
    m_ds_state.depthCompareOp = ds_state->depthCompareOp;
    m_ds_state.stencilTestEnable = ds_state->stencilTestEnable;
    m_ds_state.back = ds_state->back;
    m_ds_state.front = ds_state->front;
}

void VkPipelineObj::SetViewport(vector<VkViewport> viewports)
{
    m_viewports = viewports;
    // If we explicitly set a null viewport, pass it through to create info
    // but preserve viewportCount because it musn't change
    if (m_viewports.size() == 0) {
        m_vp_state.pViewports = nullptr;
    }
}

void VkPipelineObj::SetScissor(vector<VkRect2D> scissors)
{
    m_scissors = scissors;
    // If we explicitly set a null scissors, pass it through to create info
    // but preserve viewportCount because it musn't change
    if (m_scissors.size() == 0) {
        m_vp_state.pScissors = nullptr;
    }
}

void VkPipelineObj::MakeDynamic(VkDynamicState state)
{
    /* Only add a state once */
    for (auto it = m_dynamic_state_enables.begin(); it != m_dynamic_state_enables.end(); it++) {
        if ((*it) == state) return;
    }
    m_dynamic_state_enables.push_back(state);
}

void VkPipelineObj::SetMSAA(VkPipelineMultisampleStateCreateInfo *ms_state)
{
    memcpy(&m_ms_state, ms_state, sizeof(VkPipelineMultisampleStateCreateInfo));
}

VkResult VkPipelineObj::CreateVKPipeline(VkPipelineLayout layout, VkRenderPass render_pass)
{
    VkGraphicsPipelineCreateInfo info = {};
    VkPipelineDynamicStateCreateInfo dsci = {};

    info.stageCount = m_shaderObjs.size();
    info.pStages = new VkPipelineShaderStageCreateInfo[info.stageCount];

    for (int i=0; i<m_shaderObjs.size(); i++)
    {
        ((VkPipelineShaderStageCreateInfo *) info.pStages)[i] =
            m_shaderObjs[i]->GetStageCreateInfo();
    }

    if (m_vi_state.vertexAttributeDescriptionCount && m_vi_state.vertexBindingDescriptionCount) {
        m_vi_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.pVertexInputState = &m_vi_state;
    } else {
        info.pVertexInputState = NULL;
    }

    info.sType  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext  = NULL;
    info.flags  = 0;
    info.layout = layout;

    m_cb_state.attachmentCount = m_colorAttachments.size();
    m_cb_state.pAttachments = m_colorAttachments.data();

    if (m_viewports.size() > 0) {
        m_vp_state.viewportCount = m_viewports.size();
        m_vp_state.pViewports = m_viewports.data();
    } else {
        MakeDynamic(VK_DYNAMIC_STATE_VIEWPORT);
    }

    if (m_scissors.size() > 0) {
        m_vp_state.scissorCount = m_scissors.size();
        m_vp_state.pScissors = m_scissors.data();
    } else {
        MakeDynamic(VK_DYNAMIC_STATE_SCISSOR);
    }

    if (m_dynamic_state_enables.size() > 0) {
        dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dsci.dynamicStateCount = m_dynamic_state_enables.size();
        dsci.pDynamicStates = m_dynamic_state_enables.data();
        info.pDynamicState = &dsci;
    }

    info.renderPass        = render_pass;
    info.subpass           = 0;
    info.pTessellationState   = NULL;
    info.pInputAssemblyState  = &m_ia_state;
    info.pViewportState       = &m_vp_state;
    info.pRasterizationState         = &m_rs_state;
    info.pMultisampleState    = &m_ms_state;
    info.pDepthStencilState   = &m_ds_state;
    info.pColorBlendState     = &m_cb_state;

    return init_try(*m_device, info);
}

VkCommandBufferObj::VkCommandBufferObj(VkDeviceObj *device, VkCommandPool pool)
{
    m_device = device;

    init(*device, vk_testing::CommandBuffer::create_info(pool));
}

VkCommandBuffer VkCommandBufferObj::GetBufferHandle()
{
    return handle();
}

VkResult VkCommandBufferObj::BeginCommandBuffer(VkCommandBufferBeginInfo *pInfo)
{
    begin(pInfo);
    return VK_SUCCESS;
}

VkResult VkCommandBufferObj::BeginCommandBuffer()
{
    begin();
    return VK_SUCCESS;
}

VkResult VkCommandBufferObj::EndCommandBuffer()
{
    end();
    return VK_SUCCESS;
}

void VkCommandBufferObj::PipelineBarrier(VkPipelineStageFlags src_stages,  VkPipelineStageFlags dest_stages, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const void* const* ppMemoryBarriers)
{
    vkCmdPipelineBarrier(handle(), src_stages, dest_stages, dependencyFlags, memoryBarrierCount, ppMemoryBarriers);
}

void VkCommandBufferObj::ClearAllBuffers(VkClearColorValue clear_color, float depth_clear_color, uint32_t stencil_clear_color,
                                          VkDepthStencilObj *depthStencilObj)
{
    uint32_t i;
    const VkFlags output_mask =
        VK_ACCESS_HOST_WRITE_BIT |
        VK_ACCESS_SHADER_WRITE_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_TRANSFER_WRITE_BIT;
    const VkFlags input_mask = 0;

    // whatever we want to do, we do it to the whole buffer
    VkImageSubresourceRange srRange = {};
    srRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    srRange.baseMipLevel = 0;
    srRange.levelCount = VK_REMAINING_MIP_LEVELS;
    srRange.baseArrayLayer = 0;
    srRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkImageMemoryBarrier memory_barrier = {};
    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.srcAccessMask = output_mask;
    memory_barrier.dstAccessMask = input_mask;
    memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    memory_barrier.subresourceRange = srRange;
    VkImageMemoryBarrier *pmemory_barrier = &memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    for (i = 0; i < m_renderTargets.size(); i++) {
        memory_barrier.image = m_renderTargets[i]->image();
        memory_barrier.oldLayout = m_renderTargets[i]->layout();
        vkCmdPipelineBarrier( handle(), src_stages, dest_stages, 0, 1, (const void * const*)&pmemory_barrier);
        m_renderTargets[i]->layout(memory_barrier.newLayout);

        vkCmdClearColorImage(handle(),
               m_renderTargets[i]->image(), VK_IMAGE_LAYOUT_GENERAL,
               &clear_color, 1, &srRange );

    }

    if (depthStencilObj)
    {
        VkImageSubresourceRange dsRange = {};
        dsRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        dsRange.baseMipLevel = 0;
        dsRange.levelCount = VK_REMAINING_MIP_LEVELS;
        dsRange.baseArrayLayer = 0;
        dsRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        // prepare the depth buffer for clear

        memory_barrier.oldLayout = memory_barrier.newLayout;
        memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        memory_barrier.image = depthStencilObj->handle();
        memory_barrier.subresourceRange = dsRange;

        vkCmdPipelineBarrier( handle(), src_stages, dest_stages, 0, 1, (const void * const*)&pmemory_barrier);

        VkClearDepthStencilValue clear_value = {
            depth_clear_color,
            stencil_clear_color
        };
        vkCmdClearDepthStencilImage(handle(),
                                    depthStencilObj->handle(), VK_IMAGE_LAYOUT_GENERAL,
                                    &clear_value,
                                    1, &dsRange);

        // prepare depth buffer for rendering
        memory_barrier.image = depthStencilObj->handle();
        memory_barrier.newLayout = memory_barrier.oldLayout;
        memory_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        memory_barrier.subresourceRange = dsRange;
        vkCmdPipelineBarrier( handle(), src_stages, dest_stages, 0, 1, (const void * const*)&pmemory_barrier);
    }
}

void VkCommandBufferObj::FillBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize fill_size, uint32_t data)
{
    vkCmdFillBuffer( handle(), buffer, offset, fill_size, data);
}

void VkCommandBufferObj::UpdateBuffer(VkBuffer buffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t *pData)
{
    vkCmdUpdateBuffer(handle(), buffer, dstOffset, dataSize, pData);
}

void VkCommandBufferObj::CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    vkCmdCopyImage(handle(), srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void VkCommandBufferObj::ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
    vkCmdResolveImage(handle(), srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void VkCommandBufferObj::PrepareAttachments()
{
    uint32_t i;
    const VkFlags output_mask =
        VK_ACCESS_HOST_WRITE_BIT |
        VK_ACCESS_SHADER_WRITE_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_TRANSFER_WRITE_BIT;
    const VkFlags input_mask =
        VK_ACCESS_HOST_READ_BIT |
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
        VK_ACCESS_INDEX_READ_BIT |
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        VK_ACCESS_UNIFORM_READ_BIT |
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_MEMORY_READ_BIT;

    VkImageSubresourceRange srRange = {};
    srRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    srRange.baseMipLevel = 0;
    srRange.levelCount = VK_REMAINING_MIP_LEVELS;
    srRange.baseArrayLayer = 0;
    srRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkImageMemoryBarrier memory_barrier = {};
    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.srcAccessMask = output_mask;
    memory_barrier.dstAccessMask = input_mask;
    memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    memory_barrier.subresourceRange = srRange;
    VkImageMemoryBarrier *pmemory_barrier = &memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    for(i=0; i<m_renderTargets.size(); i++)
    {
        memory_barrier.image = m_renderTargets[i]->image();
        memory_barrier.oldLayout = m_renderTargets[i]->layout();
        vkCmdPipelineBarrier( handle(), src_stages, dest_stages, 0, 1, (const void * const*)&pmemory_barrier);
        m_renderTargets[i]->layout(memory_barrier.newLayout);
    }
}

void VkCommandBufferObj::BeginRenderPass(const VkRenderPassBeginInfo &info)
{
    vkCmdBeginRenderPass( handle(), &info, VK_SUBPASS_CONTENTS_INLINE);
}

void VkCommandBufferObj::EndRenderPass()
{
    vkCmdEndRenderPass(handle());
}

void VkCommandBufferObj::SetViewport(
    uint32_t                            viewportCount,
    const VkViewport*                   pViewports)
{
    vkCmdSetViewport( handle(), viewportCount, pViewports);
}

void VkCommandBufferObj::SetScissor(
    uint32_t                            scissorCount,
    const VkRect2D*                     pScissors)
{
    vkCmdSetScissor( handle(), scissorCount, pScissors);
}

void VkCommandBufferObj::SetLineWidth(float lineWidth)
{
    vkCmdSetLineWidth( handle(), lineWidth);
}

void VkCommandBufferObj::SetDepthBias(
    float                               depthBiasConstantFactor,
    float                               depthBiasClamp,
    float                               depthBiasSlopeFactor)
{
    vkCmdSetDepthBias( handle(), depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void VkCommandBufferObj::SetBlendConstants(
    const float                         blendConstants[4])
{
    vkCmdSetBlendConstants( handle(), blendConstants);
}

void VkCommandBufferObj::SetDepthBounds(
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    vkCmdSetDepthBounds( handle(), minDepthBounds, maxDepthBounds);
}

void VkCommandBufferObj::SetStencilReadMask(
    VkStencilFaceFlags                  faceMask,
    uint32_t                            compareMask)
{
    vkCmdSetStencilCompareMask( handle(), faceMask, compareMask);
}

void VkCommandBufferObj::SetStencilWriteMask(
    VkStencilFaceFlags                  faceMask,
    uint32_t                            writeMask)
{
    vkCmdSetStencilWriteMask( handle(), faceMask, writeMask);
}

void VkCommandBufferObj::SetStencilReference(
    VkStencilFaceFlags                  faceMask,
    uint32_t                            reference)
{
    vkCmdSetStencilReference( handle(), faceMask, reference);
}

void VkCommandBufferObj::AddRenderTarget(VkImageObj *renderTarget)
{
    m_renderTargets.push_back(renderTarget);
}

void VkCommandBufferObj::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(handle(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VkCommandBufferObj::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(handle(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void VkCommandBufferObj::QueueCommandBuffer()
{
    VkFence nullFence = { VK_NULL_HANDLE };
    QueueCommandBuffer(nullFence);
}

void VkCommandBufferObj::QueueCommandBuffer(VkFence fence)
{
    VkResult err = VK_SUCCESS;

    // submit the command buffer to the universal queue
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &handle();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit( m_device->m_queue, 1, &submit_info, fence );
    ASSERT_VK_SUCCESS( err );

    err = vkQueueWaitIdle( m_device->m_queue );
    ASSERT_VK_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    vkDeviceWaitIdle(m_device->device());
}

void VkCommandBufferObj::BindPipeline(VkPipelineObj &pipeline)
{
    vkCmdBindPipeline( handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle() );
}

void VkCommandBufferObj::BindDescriptorSet(VkDescriptorSetObj &descriptorSet)
{
    VkDescriptorSet set_obj = descriptorSet.GetDescriptorSetHandle();

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic buffer view)
    vkCmdBindDescriptorSets(handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSet.GetPipelineLayout(),
           0, 1, &set_obj, 0, NULL );
}

void VkCommandBufferObj::BindIndexBuffer(VkIndexBufferObj *indexBuffer, VkDeviceSize offset)
{
    vkCmdBindIndexBuffer(handle(), indexBuffer->handle(), offset, indexBuffer->GetIndexType());
}

void VkCommandBufferObj::BindVertexBuffer(VkConstantBufferObj *vertexBuffer, VkDeviceSize offset, uint32_t binding)
{
    vkCmdBindVertexBuffers(handle(), binding, 1, &vertexBuffer->handle(), &offset);
}

VkDepthStencilObj::VkDepthStencilObj()
{
    m_initialized = false;
}
bool VkDepthStencilObj::Initialized()
{
    return m_initialized;
}

VkImageView* VkDepthStencilObj::BindInfo()
{
    return &m_attachmentBindInfo;
}

void VkDepthStencilObj::Init(VkDeviceObj *device, int32_t width, int32_t height, VkFormat format)
{
    VkImageCreateInfo image_info = {};
    VkImageViewCreateInfo view_info = {};

    m_device = device;
    m_initialized = true;
    m_depth_stencil_fmt = format;

    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = m_depth_stencil_fmt;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.flags = 0;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    init(*m_device, image_info);

    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.image = VK_NULL_HANDLE;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    view_info.flags = 0;
    view_info.format = m_depth_stencil_fmt;
    view_info.image = handle();
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    m_imageView.init(*m_device, view_info);

    m_attachmentBindInfo = m_imageView.handle();
}
