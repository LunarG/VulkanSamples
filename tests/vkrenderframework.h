/*
 * Vulkan Tests
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

#ifndef VKRENDERFRAMEWORK_H
#define VKRENDERFRAMEWORK_H

#include "vktestframework.h"
#include "vk_debug_report_lunarg.h"
#include "vk_debug_marker_lunarg.h"


class VkDeviceObj : public vk_testing::Device
{
public:
    VkDeviceObj(uint32_t id, VkPhysicalDevice obj);
    VkDeviceObj(uint32_t id, VkPhysicalDevice obj,
                std::vector<const char *> &layers,
                std::vector<const char *> &extension_names);

    VkDevice device() { return handle(); }
    void get_device_queue();

    uint32_t                               id;
    VkPhysicalDeviceProperties             props;
    const VkQueueFamilyProperties         *queue_props;

    VkQueue m_queue;
};

class VkDepthStencilObj : public vk_testing::Image
{
public:
    VkDepthStencilObj();
    void Init(VkDeviceObj *device, int32_t width, int32_t height, VkFormat format);
    bool Initialized();
    VkImageView* BindInfo();

protected:
    VkDeviceObj                        *m_device;
    bool                                m_initialized;
    vk_testing::ImageView               m_imageView;
    VkFormat                            m_depth_stencil_fmt;
    VkImageView                         m_attachmentBindInfo;
};

class VkCommandBufferObj;

class VkRenderFramework : public VkTestFramework
{
public:
    VkRenderFramework();
    ~VkRenderFramework();

    VkDevice device() {return m_device->device();}
    VkPhysicalDevice gpu() {return objs[0];}
    VkRenderPass renderPass() {return m_renderPass;}
    VkFramebuffer framebuffer() {return m_framebuffer;}
    std::vector<vk_testing::ShaderModule*> m_shader_modules;
    void InitViewport(float width, float height);
    void InitViewport();
    void InitRenderTarget();
    void InitRenderTarget(uint32_t targets);
    void InitRenderTarget(VkImageView *dsBinding);
    void InitRenderTarget(uint32_t targets, VkImageView *dsBinding);
    void InitFramework();
    void InitFramework(
            std::vector<const char *> instance_layer_names,
            std::vector<const char *> device_layer_names,
            std::vector<const char *> instance_extension_names,
            std::vector<const char *> device_extension_names,
            PFN_vkDbgMsgCallback=NULL,
            void *userData=NULL);

    void ShutdownFramework();
    void InitState();

    const VkRenderPassBeginInfo &renderPassBeginInfo() const { return m_renderPassBeginInfo; }

protected:
    VkApplicationInfo                   app_info;
    VkInstance                          inst;
    VkPhysicalDevice                    objs[16];
    uint32_t                            gpu_count;
    VkDeviceObj                        *m_device;
    VkCmdPool                           m_cmdPool;
    VkCommandBufferObj                 *m_cmdBuffer;
    VkRenderPass                        m_renderPass;
    VkFramebuffer                       m_framebuffer;
    std::vector<VkViewport>             m_viewports;
    std::vector<VkRect2D>               m_scissors;
    float                               m_lineWidth;
    float                               m_depthBias;
    float                               m_depthBiasClamp;
    float                               m_slopeScaledDepthBias;
    float                               m_blendConst[4];
    float                               m_minDepthBounds;
    float                               m_maxDepthBounds;
    uint32_t                            m_stencilCompareMask;
    uint32_t                            m_stencilWriteMask;
    uint32_t                            m_stencilReference;
    std::vector<VkClearValue>           m_renderPassClearValues;
    VkRenderPassBeginInfo               m_renderPassBeginInfo;
    vector<VkImageObj*>                 m_renderTargets;
    float                               m_width, m_height;
    VkFormat                            m_render_target_fmt;
    VkFormat                            m_depth_stencil_fmt;
    VkClearColorValue                   m_clear_color;
    bool                                m_clear_via_load_op;
    float                               m_depth_clear_color;
    uint32_t                            m_stencil_clear_color;
    VkDepthStencilObj                  *m_depthStencil;
    PFN_vkDbgCreateMsgCallback          m_dbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback         m_dbgDestroyMsgCallback;
    VkDbgMsgCallback                    m_globalMsgCallback;
    VkDbgMsgCallback                    m_devMsgCallback;

    /*
     * SetUp and TearDown are called by the Google Test framework
     * to initialize a test framework based on this class.
     */
    virtual void SetUp() {
        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION;

        InitFramework();
    }

    virtual void TearDown() {
        ShutdownFramework();
    }
};

class VkDescriptorSetObj;
class VkIndexBufferObj;
class VkConstantBufferObj;
class VkPipelineObj;
class VkDescriptorSetObj;

class VkCommandBufferObj : public vk_testing::CmdBuffer
{
public:
    VkCommandBufferObj(VkDeviceObj *device, VkCmdPool pool);
    VkCmdBuffer GetBufferHandle();
    VkResult BeginCommandBuffer();
    VkResult BeginCommandBuffer(VkCmdBufferBeginInfo *pInfo);
    VkResult EndCommandBuffer();
    void PipelineBarrier(VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages, VkBool32 byRegion, uint32_t memBarrierCount, const void* const* ppMemBarriers);
    void AddRenderTarget(VkImageObj *renderTarget);
    void AddDepthStencil();
    void ClearAllBuffers(VkClearColorValue clear_color, float depth_clear_color, uint32_t stencil_clear_color, VkDepthStencilObj *depthStencilObj);
    void PrepareAttachments();
    void BindPipeline(VkPipelineObj &pipeline);
    void BindDescriptorSet(VkDescriptorSetObj &descriptorSet);
    void BindVertexBuffer(VkConstantBufferObj *vertexBuffer, VkDeviceSize offset, uint32_t binding);
    void BindIndexBuffer(VkIndexBufferObj *indexBuffer, VkDeviceSize offset);
    void BeginRenderPass(const VkRenderPassBeginInfo &info);
    void EndRenderPass();
    void FillBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize fill_size, uint32_t data);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    void QueueCommandBuffer();
    void QueueCommandBuffer(VkFence fence);
    void SetViewport(uint32_t viewportCount, const VkViewport* pViewports);
    void SetScissor(uint32_t scissorCount, const VkRect2D* pScissors);
    void SetLineWidth(float lineWidth);
    void SetDepthBias(float depthBias, float depthBiasClamp, float slopeScaledDepthBias);
    void SetBlendConstants(const float blendConst[4]);
    void SetDepthBounds(float minDepthBounds, float maxDepthBounds);
    void SetStencilReadMask(VkStencilFaceFlags faceMask, uint32_t stencilCompareMask);
    void SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t stencilWriteMask);
    void SetStencilReference(VkStencilFaceFlags faceMask, uint32_t stencilReference);

protected:
    VkDeviceObj                        *m_device;
    vector<VkImageObj*>                 m_renderTargets;
};

class VkConstantBufferObj : public vk_testing::Buffer
{
public:
    VkConstantBufferObj(VkDeviceObj *device);
    VkConstantBufferObj(VkDeviceObj *device, int constantCount, int constantSize, const void* data);
    ~VkConstantBufferObj();
    void BufferMemoryBarrier(
        VkFlags outputMask =
            VK_MEMORY_OUTPUT_HOST_WRITE_BIT |
            VK_MEMORY_OUTPUT_SHADER_WRITE_BIT |
            VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
            VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_MEMORY_OUTPUT_TRANSFER_BIT,
        VkFlags inputMask =
            VK_MEMORY_INPUT_HOST_READ_BIT |
            VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
            VK_MEMORY_INPUT_INDEX_FETCH_BIT |
            VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
            VK_MEMORY_INPUT_UNIFORM_READ_BIT |
            VK_MEMORY_INPUT_SHADER_READ_BIT |
            VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
            VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_MEMORY_INPUT_TRANSFER_BIT);

    void Bind(VkCmdBuffer cmdBuffer, VkDeviceSize offset, uint32_t binding);

    VkDescriptorInfo                    m_descriptorInfo;

protected:
    VkDeviceObj                        *m_device;
    vk_testing::BufferView              m_bufferView;
    int                                 m_numVertices;
    int                                 m_stride;
    vk_testing::CmdPool                *m_cmdPool;
    VkCommandBufferObj                 *m_commandBuffer;
    vk_testing::Fence                   m_fence;
};

class VkIndexBufferObj : public VkConstantBufferObj
{
public:
    VkIndexBufferObj(VkDeviceObj *device);
    void CreateAndInitBuffer(int numIndexes, VkIndexType dataFormat, const void* data);
    void Bind(VkCmdBuffer cmdBuffer, VkDeviceSize offset);
    VkIndexType GetIndexType();

protected:
    VkIndexType                         m_indexType;
};

class VkImageObj : public vk_testing::Image
{
public:
    VkImageObj(VkDeviceObj *dev);
    bool IsCompatible(VkFlags usage, VkFlags features);

public:
    void init(uint32_t w, uint32_t h,
              VkFormat fmt, VkFlags usage,
              VkImageTiling tiling=VK_IMAGE_TILING_LINEAR,
              VkMemoryPropertyFlags reqs=0);

    //    void clear( CommandBuffer*, uint32_t[4] );

    void layout( VkImageLayout layout )
    {
        m_descriptorInfo.imageLayout = layout;
    }

    VkDeviceMemory memory() const
    {
        return Image::memory().handle();
    }

    void *MapMemory()
    {
        return Image::memory().map();
    }

    void UnmapMemory()
    {
        Image::memory().unmap();
    }

    void ImageMemoryBarrier(VkCommandBufferObj *cmd,
                            VkImageAspect aspect,
                            VkFlags output_mask,
                            VkFlags input_mask,
                            VkImageLayout image_layout);

    VkResult CopyImage(VkImageObj &src_image);

    VkImage image() const
    {
        return handle();
    }

    VkImageView targetView(VkFormat format)
    {
        if (!m_targetView.initialized())
        {
            VkImageViewCreateInfo createView = {};
            createView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createView.image = handle();
            createView.viewType =  VK_IMAGE_VIEW_TYPE_2D;
            createView.format = format;
            createView.channels.r = VK_CHANNEL_SWIZZLE_R;
            createView.channels.g = VK_CHANNEL_SWIZZLE_G;
            createView.channels.b = VK_CHANNEL_SWIZZLE_B;
            createView.channels.a = VK_CHANNEL_SWIZZLE_A;
            createView.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            createView.flags = 0;
            m_targetView.init(*m_device, createView);
        }
        return m_targetView.handle();
    }

    void SetLayout(VkCommandBufferObj *cmd_buf, VkImageAspect aspect, VkImageLayout image_layout);
    void SetLayout(VkImageAspect aspect, VkImageLayout image_layout);

    VkImageLayout layout() const
    {
        return m_descriptorInfo.imageLayout;
    }
    uint32_t width() const
    {
        return extent().width;
    }
    uint32_t height() const
    {
        return extent().height;
    }
    VkDeviceObj* device() const
    {
        return m_device;
    }

protected:
    VkDeviceObj                        *m_device;

    vk_testing::ImageView               m_targetView;
    VkDescriptorInfo                    m_descriptorInfo;
};

class VkTextureObj : public VkImageObj
{
public:
    VkTextureObj(VkDeviceObj *device, uint32_t *colors = NULL);

    VkDescriptorInfo                    m_descriptorInfo;

protected:
    VkDeviceObj                        *m_device;
    vk_testing::ImageView               m_textureView;
    VkDeviceSize                        m_rowPitch;
};

class VkSamplerObj : public vk_testing::Sampler
{
public:
    VkSamplerObj(VkDeviceObj *device);

protected:
     VkDeviceObj                       *m_device;

};

class VkDescriptorSetObj : public vk_testing::DescriptorPool
{
public:
    VkDescriptorSetObj(VkDeviceObj *device);
    ~VkDescriptorSetObj();

    int AppendDummy();
    int AppendBuffer(VkDescriptorType type, VkConstantBufferObj &constantBuffer);
    int AppendSamplerTexture(VkSamplerObj* sampler, VkTextureObj* texture);
    void CreateVKDescriptorSet(VkCommandBufferObj *cmdBuffer);

    VkDescriptorSet GetDescriptorSetHandle() const;
    VkPipelineLayout GetPipelineLayout() const;

protected:
    VkDeviceObj                        *m_device;
    vector<VkDescriptorTypeCount>       m_type_counts;
    int                                 m_nextSlot;

    vector<VkDescriptorInfo>            m_imageSamplerDescriptors;
    vector<VkWriteDescriptorSet>        m_writes;

    vk_testing::DescriptorSetLayout     m_layout;
    vk_testing::PipelineLayout          m_pipeline_layout;
    vk_testing::DescriptorSet          *m_set;
};


class VkShaderObj : public vk_testing::Shader
{
public:
    VkShaderObj(VkDeviceObj *device, const char * shaderText, VkShaderStage stage, VkRenderFramework *framework);
    VkPipelineShaderStageCreateInfo* GetStageCreateInfo();

protected:
    VkPipelineShaderStageCreateInfo     stage_info;
    VkShaderStage                       m_stage;
    VkDeviceObj                        *m_device;

};

class VkPipelineObj : public vk_testing::Pipeline
{
public:
    VkPipelineObj(VkDeviceObj *device);
    void AddShader(VkShaderObj* shaderObj);
    void AddVertexInputAttribs(VkVertexInputAttributeDescription* vi_attrib, int count);
    void AddVertexInputBindings(VkVertexInputBindingDescription* vi_binding, int count);
    void AddColorAttachment(uint32_t binding, const VkPipelineColorBlendAttachmentState *att);

    void AddColorAttachment()
    {
        VkPipelineColorBlendAttachmentState att = {};
        att.blendEnable = VK_FALSE;
        att.channelWriteMask = 0xf;
        AddColorAttachment(0, &att);
    }

    void SetDepthStencil(VkPipelineDepthStencilStateCreateInfo *);
    void SetMSAA(VkPipelineMultisampleStateCreateInfo *ms_state);
    VkResult CreateVKPipeline(VkPipelineLayout layout, VkRenderPass render_pass);

protected:
    VkPipelineVertexInputStateCreateInfo          m_vi_state;
    VkPipelineInputAssemblyStateCreateInfo        m_ia_state;
    VkPipelineRasterStateCreateInfo               m_rs_state;
    VkPipelineColorBlendStateCreateInfo           m_cb_state;
    VkPipelineDepthStencilStateCreateInfo         m_ds_state;
    VkPipelineViewportStateCreateInfo             m_vp_state;
    VkPipelineMultisampleStateCreateInfo          m_ms_state;
    VkDeviceObj                                  *m_device;
    vector<VkShaderObj*>                          m_shaderObjs;
    vector<int>                                   m_vertexBufferBindings;
    vector<VkPipelineColorBlendAttachmentState>   m_colorAttachments;
    int                                           m_vertexBufferCount;
};

#endif // VKRENDERFRAMEWORK_H
