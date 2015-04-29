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
#include <vkDbg.h>


class VkDeviceObj : public vk_testing::Device
{
public:
    VkDeviceObj(uint32_t id, VkPhysicalDevice obj);

    VkDevice device() { return obj(); }
    void get_device_queue();

    uint32_t                               id;
    VkPhysicalDeviceProperties             props;
    const VkPhysicalDeviceQueueProperties *queue_props;

    VkQueue m_queue;
};

class VkMemoryRefManager
{
public:
    void AddMemoryRefs(vk_testing::Object &vkObject);
    void AddMemoryRefs(vector<VkDeviceMemory> mem);
    void EmitAddMemoryRefs(VkQueue queue);
    void EmitRemoveMemoryRefs(VkQueue queue);
    vector<VkDeviceMemory> mem_refs() const;

protected:
    vector<VkDeviceMemory>              mem_refs_;

};

class VkDepthStencilObj : public vk_testing::Image
{
public:
    VkDepthStencilObj();
    void Init(VkDeviceObj *device, int32_t width, int32_t height);
    bool Initialized();
    VkDepthStencilBindInfo* BindInfo();

protected:
    VkDeviceObj                        *m_device;
    bool                                m_initialized;
    vk_testing::DepthStencilView        m_depthStencilView;
    VkFormat                            m_depth_stencil_fmt;
    VkDepthStencilBindInfo              m_depthStencilBindInfo;
};

class VkRenderFramework : public VkTestFramework
{
public:
    VkRenderFramework();
    ~VkRenderFramework();

    VkDevice device() {return m_device->device();}
    VkPhysicalDevice gpu() {return objs[0];}
    VkRenderPass renderPass() {return m_renderPass;}
    VkFramebuffer framebuffer() {return m_framebuffer;}
    void InitViewport(float width, float height);
    void InitViewport();
    void InitRenderTarget();
    void InitRenderTarget(uint32_t targets);
    void InitRenderTarget(VkDepthStencilBindInfo *dsBinding);
    void InitRenderTarget(uint32_t targets, VkDepthStencilBindInfo *dsBinding);
    void InitFramework();
    void InitFramework(const std::vector<const char *> &layers, VK_DBG_MSG_CALLBACK_FUNCTION=NULL, void *userData=NULL);
    void ShutdownFramework();
    void InitState();


protected:
    VkApplicationInfo                   app_info;
    VkInstance                          inst;
    VkPhysicalDevice                    objs[16];
    uint32_t                            gpu_count;
    VkDeviceObj                        *m_device;
    VkCmdBuffer                         m_cmdBuffer;
    VkRenderPass                        m_renderPass;
    VkFramebuffer                       m_framebuffer;
    VkDynamicRsState                    m_stateRaster;
    VkDynamicCbState                    m_colorBlend;
    VkDynamicVpState                    m_stateViewport;
    VkDynamicDsState                    m_stateDepthStencil;
    vector<VkImageObj*>                 m_renderTargets;
    float                               m_width, m_height;
    VkFormat                            m_render_target_fmt;
    VkFormat                            m_depth_stencil_fmt;
    VkColorAttachmentBindInfo           m_colorBindings[8];
    VkClearColor                        m_clear_color;
    float                               m_depth_clear_color;
    uint32_t                            m_stencil_clear_color;
    VkDepthStencilObj                  *m_depthStencil;
    VkMemoryRefManager                  m_mem_ref_mgr;

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
    VkCommandBufferObj(VkDeviceObj *device);
    VkCmdBuffer GetBufferHandle();
    VkResult BeginCommandBuffer();
    VkResult BeginCommandBuffer(VkCmdBufferBeginInfo *pInfo);
    VkResult BeginCommandBuffer(VkRenderPass renderpass_obj, VkFramebuffer framebuffer_obj);
    VkResult EndCommandBuffer();
    void PipelineBarrier(VkWaitEvent waitEvent, uint32_t pipeEventCount, const VkPipeEvent* pPipeEvents, uint32_t memBarrierCount, const void** ppMemBarriers);
    void AddRenderTarget(VkImageObj *renderTarget);
    void AddDepthStencil();
    void ClearAllBuffers(VkClearColor clear_color, float depth_clear_color, uint32_t stencil_clear_color, VkDepthStencilObj *depthStencilObj);
    void PrepareAttachments();
    void AddMemoryRefs(vk_testing::Object &vkObject);
    void AddMemoryRefs(uint32_t ref_count, const VkDeviceMemory *mem);
    void AddMemoryRefs(vector<vk_testing::Object *> images);
    void BindPipeline(VkPipelineObj &pipeline);
    void BindDescriptorSet(VkDescriptorSetObj &descriptorSet);
    void BindVertexBuffer(VkConstantBufferObj *vertexBuffer, VkDeviceSize offset, uint32_t binding);
    void BindIndexBuffer(VkIndexBufferObj *indexBuffer, uint32_t offset);
    void BindStateObject(VkStateBindPoint stateBindPoint, VkDynamicStateObject stateObject);
    void BeginRenderPass(VkRenderPass renderpass, VkFramebuffer framebuffer);
    void EndRenderPass(VkRenderPass renderpass);
    void Draw(uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount);
    void DrawIndexed(uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount);
    void QueueCommandBuffer();
    void QueueCommandBuffer(VkFence fence);

    VkMemoryRefManager                  mem_ref_mgr;

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
            VK_MEMORY_OUTPUT_CPU_WRITE_BIT |
            VK_MEMORY_OUTPUT_SHADER_WRITE_BIT |
            VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
            VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_MEMORY_OUTPUT_TRANSFER_BIT,
        VkFlags inputMask =
            VK_MEMORY_INPUT_CPU_READ_BIT |
            VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
            VK_MEMORY_INPUT_INDEX_FETCH_BIT |
            VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
            VK_MEMORY_INPUT_UNIFORM_READ_BIT |
            VK_MEMORY_INPUT_SHADER_READ_BIT |
            VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
            VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_MEMORY_INPUT_TRANSFER_BIT);

    void Bind(VkCmdBuffer cmdBuffer, VkDeviceSize offset, uint32_t binding);

    VkBufferViewAttachInfo     m_bufferViewInfo;

protected:
    VkDeviceObj                        *m_device;
    vk_testing::BufferView              m_bufferView;
    int                                 m_numVertices;
    int                                 m_stride;
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
        m_imageInfo.layout = layout;
    }

    VkDeviceMemory memory() const
    {
        const std::vector<VkDeviceMemory> mems = memories();
        return mems.empty() ? VK_NULL_HANDLE : mems[0];
    }

    void ImageMemoryBarrier(VkCommandBufferObj *cmd,
                            VkImageAspect aspect,
                            VkFlags output_mask,
                            VkFlags input_mask,
                            VkImageLayout image_layout);

    VkResult CopyImage(VkImageObj &src_image);

    VkImage image() const
    {
        return obj();
    }

    VkColorAttachmentView targetView()
    {
        if (!m_targetView.initialized())
        {
            VkColorAttachmentViewCreateInfo createView = {
                VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
                VK_NULL_HANDLE,
                obj(),
                VK_FORMAT_B8G8R8A8_UNORM,
                0,
                0,
                1
            };
            m_targetView.init(*m_device, createView);
        }
        return m_targetView.obj();
    }

    void SetLayout(VkCommandBufferObj *cmd_buf, VkImageAspect aspect, VkImageLayout image_layout);
    void SetLayout(VkImageAspect aspect, VkImageLayout image_layout);

    VkImageLayout layout() const
    {
        return ( VkImageLayout )m_imageInfo.layout;
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

    VkResult MapMemory(void** ptr);
    VkResult UnmapMemory();

protected:
    VkDeviceObj                        *m_device;

    vk_testing::ColorAttachmentView     m_targetView;
    VkImageViewAttachInfo               m_imageInfo;
};

class VkTextureObj : public VkImageObj
{
public:
    VkTextureObj(VkDeviceObj *device, uint32_t *colors = NULL);
    VkImageViewAttachInfo m_textureViewInfo;


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

    VkMemoryRefManager                  mem_ref_mgr;

protected:
    VkDeviceObj                        *m_device;
    vector<VkDescriptorTypeCount>       m_type_counts;
    int                                 m_nextSlot;

    vector<VkUpdateBuffers>             m_updateBuffers;

    vector<VkSamplerImageViewInfo>      m_samplerTextureInfo;
    vector<VkUpdateSamplerTextures>     m_updateSamplerTextures;

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
    void AddVertexDataBuffer(VkConstantBufferObj* vertexDataBuffer, int binding);
    void AddColorAttachment(uint32_t binding, const VkPipelineCbAttachmentState *att);
    void SetDepthStencil(VkPipelineDsStateCreateInfo *);
    void CreateVKPipeline(VkDescriptorSetObj &descriptorSet);

protected:
    VkPipelineVertexInputCreateInfo     m_vi_state;
    VkPipelineIaStateCreateInfo         m_ia_state;
    VkPipelineRsStateCreateInfo         m_rs_state;
    VkPipelineCbStateCreateInfo         m_cb_state;
    VkPipelineDsStateCreateInfo         m_ds_state;
    VkPipelineMsStateCreateInfo         m_ms_state;
    VkDeviceObj                        *m_device;
    vector<VkShaderObj*>                m_shaderObjs;
    vector<VkConstantBufferObj*>        m_vertexBufferObjs;
    vector<int>                         m_vertexBufferBindings;
    vector<VkPipelineCbAttachmentState> m_colorAttachments;
    int                                 m_vertexBufferCount;

};

#endif // VKRENDERFRAMEWORK_H
